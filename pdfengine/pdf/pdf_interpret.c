#include "pdf-internal.h"
#include "pdfengine-internal.h"

#define TILE

typedef struct pdf_material_s pdf_material;
typedef struct pdf_gstate_s pdf_gstate;
typedef struct pdf_csi_s pdf_csi;

enum
{
	PDF_FILL,
	PDF_STROKE,
};

enum
{
	PDF_MAT_NONE,
	PDF_MAT_COLOR,
	PDF_MAT_PATTERN,
	PDF_MAT_SHADE,
};

struct pdf_material_s
{
	int kind;
	base_colorspace *colorspace;
	pdf_pattern *pattern;
	base_shade *shade;
	float alpha;
	float v[32];
};

struct pdf_gstate_s
{
	base_matrix ctm;
	int clip_depth;

	
	base_stroke_state *stroke_state;

	
	pdf_material stroke;
	pdf_material fill;

	
	float char_space;
	float word_space;
	float scale;
	float leading;
	pdf_font_desc *font;
	float size;
	int render;
	float rise;

	
	int blendmode;
	pdf_xobject *softmask;
	base_matrix softmask_ctm;
	float softmask_bc[base_MAX_COLORS];
	int luminosity;
};

struct pdf_csi_s
{
	base_device *dev;
	pdf_document *xref;

	
	char *event; 

	
	pdf_obj *obj;
	char name[256];
	unsigned char string[256];
	int string_len;
	float stack[32];
	int top;

	int xbalance;
	int in_text;
	int in_hidden_ocg;

	
	base_path *path;
	int clip;
	int clip_even_odd;

	
	base_text *text;
	base_rect text_bbox;
	base_matrix tlm;
	base_matrix tm;
	int text_mode;
	int accumulate;

	
	base_matrix top_ctm;
	pdf_gstate *gstate;
	int gcap;
	int gtop;

	
	base_cookie *cookie;
};

static void pdf_run_buffer(pdf_csi *csi, pdf_obj *rdb, base_buffer *contents);
static void pdf_run_xobject(pdf_csi *csi, pdf_obj *resources, pdf_xobject *xobj, base_matrix transform);
static void pdf_show_pattern(pdf_csi *csi, pdf_pattern *pat, base_rect area, int what);

static int
ocg_intents_include(pdf_ocg_descriptor *desc, char *name)
{
	int i, len;

	if (strcmp(name, "All") == 0)
		return 1;

	
	if (!desc->intent)
		return (strcmp(name, "View") == 0);

	if (pdf_is_name(desc->intent))
	{
		char *intent = pdf_to_name(desc->intent);
		if (strcmp(intent, "All") == 0)
			return 1;
		return (strcmp(intent, name) == 0);
	}
	if (!pdf_is_array(desc->intent))
		return 0;

	len = pdf_array_len(desc->intent);
	for (i=0; i < len; i++)
	{
		char *intent = pdf_to_name(pdf_array_get(desc->intent, i));
		if (strcmp(intent, "All") == 0)
			return 1;
		if (strcmp(intent, name) == 0)
			return 1;
	}
	return 0;
}

static int
pdf_is_hidden_ocg(pdf_obj *ocg, pdf_csi *csi, pdf_obj *rdb)
{
	char event_state[16];
	pdf_obj *obj, *obj2;
	char *type;
	pdf_ocg_descriptor *desc = csi->xref->ocg;

	
	if (!desc)
		return 0;

	
	if (pdf_is_name(ocg))
	{
		ocg = pdf_dict_gets(pdf_dict_gets(rdb, "Properties"), pdf_to_name(ocg));
	}
	
	if (!ocg)
		return 0;

	base_strlcpy(event_state, csi->event, sizeof event_state);
	base_strlcat(event_state, "State", sizeof event_state);

	type = pdf_to_name(pdf_dict_gets(ocg, "Type"));

	if (strcmp(type, "OCG") == 0)
	{
		
		int num = pdf_to_num(ocg);
		int gen = pdf_to_gen(ocg);
		int len = desc->len;
		int i;

		for (i = 0; i < len; i++)
		{
			if (desc->ocgs[i].num == num && desc->ocgs[i].gen == gen)
			{
				if (desc->ocgs[i].state == 0)
					return 1; 
				break;
			}
		}

		
		obj = pdf_dict_gets(ocg, "Intent");
		if (pdf_is_name(obj))
		{
			
			if (ocg_intents_include(desc, pdf_to_name(obj)) == 0)
				return 1;
		}
		else if (pdf_is_array(obj))
		{
			int match = 0;
			len = pdf_array_len(obj);
			for (i=0; i<len; i++) {
				match |= ocg_intents_include(desc, pdf_to_name(pdf_array_get(obj, i)));
				if (match)
					break;
			}
			
			if (match == 0)
				return 1;
		}
		else
		{
			
			if (ocg_intents_include(desc, "View") == 0)
				return 1;
		}

		
		obj = pdf_dict_gets(ocg, "Usage");
		if (!pdf_is_dict(obj))
			return 0;
		
		
		obj2 = pdf_dict_gets(obj, csi->event);
		if (strcmp(pdf_to_name(pdf_dict_gets(obj2, event_state)), "OFF") == 0)
		{
			return 1;
		}
		return 0;
	}
	else if (strcmp(type, "OCMD") == 0)
	{
		
		char *name;
		int combine, on;

		obj = pdf_dict_gets(ocg, "VE");
		if (pdf_is_array(obj)) {
			
			return 0;
		}
		name = pdf_to_name(pdf_dict_gets(ocg, "P"));
		
		if (strcmp(name, "AllOn") == 0)
		{
			combine = 1;
		}
		else if (strcmp(name, "AnyOff") == 0)
		{
			combine = 2;
		}
		else if (strcmp(name, "AllOff") == 0)
		{
			combine = 3;
		}
		else	
		{
			combine = 0;
		}

		obj = pdf_dict_gets(ocg, "OCGs");
		on = combine & 1;
		if (pdf_is_array(obj)) {
			int i, len;
			len = pdf_array_len(obj);
			for (i = 0; i < len; i++)
			{
				int hidden;
				hidden = pdf_is_hidden_ocg(pdf_array_get(obj, i), csi, rdb);
				if ((combine & 1) == 0)
					hidden = !hidden;
				if (combine & 2)
					on &= hidden;
				else
					on |= hidden;
			}
		}
		else
		{
			on = pdf_is_hidden_ocg(obj, csi, rdb);
			if ((combine & 1) == 0)
				on = !on;
		}
		return !on;
	}
	
	return 0;
}

static void
pdf_begin_group(pdf_csi *csi, base_rect bbox)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;

	if (gstate->softmask)
	{
		pdf_xobject *softmask = gstate->softmask;
		base_rect bbox = base_transform_rect(gstate->softmask_ctm, softmask->bbox);
		base_matrix save_ctm = gstate->ctm;

		gstate->softmask = NULL;
		gstate->ctm = gstate->softmask_ctm;

		base_begin_mask(csi->dev, bbox, gstate->luminosity,
			softmask->colorspace, gstate->softmask_bc);
		pdf_run_xobject(csi, NULL, softmask, base_identity);
		
		base_end_mask(csi->dev);

		gstate->softmask = softmask;
		gstate->ctm = save_ctm;
	}

	if (gstate->blendmode)
		base_begin_group(csi->dev, bbox, 1, 0, gstate->blendmode, 1);
}

static void
pdf_end_group(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;

	if (gstate->blendmode)
		base_end_group(csi->dev);

	if (gstate->softmask)
		base_pop_clip(csi->dev);
}

static void
pdf_show_shade(pdf_csi *csi, base_shade *shd)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	base_rect bbox;

	if (csi->in_hidden_ocg > 0)
		return;

	bbox = base_bound_shade(ctx, shd, gstate->ctm);

	pdf_begin_group(csi, bbox);

	base_fill_shade(csi->dev, shd, gstate->ctm, gstate->fill.alpha);

	pdf_end_group(csi);
}

static void
pdf_show_image(pdf_csi *csi, base_image *image)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	base_matrix image_ctm;
	base_rect bbox;

	if (csi->in_hidden_ocg > 0)
		return;

	
	image_ctm = base_concat(base_scale(1, -1), base_translate(0, 1));
	image_ctm = base_concat(image_ctm, gstate->ctm);

	bbox = base_transform_rect(image_ctm, base_unit_rect);

	if (image->mask)
	{
		
		if (gstate->blendmode)
			base_begin_group(csi->dev, bbox, 0, 0, gstate->blendmode, 1);
		base_clip_image_mask(csi->dev, image->mask, &bbox, image_ctm);
	}
	else
		pdf_begin_group(csi, bbox);

	if (!image->colorspace)
	{

		switch (gstate->fill.kind)
		{
		case PDF_MAT_NONE:
			break;
		case PDF_MAT_COLOR:
			base_fill_image_mask(csi->dev, image, image_ctm,
				gstate->fill.colorspace, gstate->fill.v, gstate->fill.alpha);
			break;
		case PDF_MAT_PATTERN:
			if (gstate->fill.pattern)
			{
				base_clip_image_mask(csi->dev, image, &bbox, image_ctm);
				pdf_show_pattern(csi, gstate->fill.pattern, bbox, PDF_FILL);
				base_pop_clip(csi->dev);
			}
			break;
		case PDF_MAT_SHADE:
			if (gstate->fill.shade)
			{
				base_clip_image_mask(csi->dev, image, &bbox, image_ctm);
				base_fill_shade(csi->dev, gstate->fill.shade, gstate->ctm, gstate->fill.alpha);
				base_pop_clip(csi->dev);
			}
			break;
		}
	}
	else
	{
		base_fill_image(csi->dev, image, image_ctm, gstate->fill.alpha);
	}

	if (image->mask)
	{
		base_pop_clip(csi->dev);
		if (gstate->blendmode)
			base_end_group(csi->dev);
	}
	else
		pdf_end_group(csi);
}

static void
pdf_show_path(pdf_csi *csi, int doclose, int dofill, int dostroke, int even_odd)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	base_path *path;
	base_rect bbox;

	if (dostroke) {
		if (csi->dev->flags & (base_DEVFLAG_STROKECOLOR_UNDEFINED | base_DEVFLAG_LINEJOIN_UNDEFINED | base_DEVFLAG_LINEWIDTH_UNDEFINED))
			csi->dev->flags |= base_DEVFLAG_UNCACHEABLE;
		else if (gstate->stroke_state->dash_len != 0 && csi->dev->flags & (base_DEVFLAG_STARTCAP_UNDEFINED | base_DEVFLAG_DASHCAP_UNDEFINED | base_DEVFLAG_ENDCAP_UNDEFINED))
			csi->dev->flags |= base_DEVFLAG_UNCACHEABLE;
		else if (gstate->stroke_state->linejoin == base_LINEJOIN_MITER && (csi->dev->flags & base_DEVFLAG_MITERLIMIT_UNDEFINED))
			csi->dev->flags |= base_DEVFLAG_UNCACHEABLE;
	}
	if (dofill) {
		if (csi->dev->flags & base_DEVFLAG_FILLCOLOR_UNDEFINED)
			csi->dev->flags |= base_DEVFLAG_UNCACHEABLE;
	}

	path = csi->path;
	csi->path = base_new_path(ctx);

	base_try(ctx)
	{
		if (doclose)
			base_closepath(ctx, path);

		if (dostroke)
			bbox = base_bound_path(ctx, path, gstate->stroke_state, gstate->ctm);
		else
			bbox = base_bound_path(ctx, path, NULL, gstate->ctm);

		if (csi->clip)
		{
			gstate->clip_depth++;
			base_clip_path(csi->dev, path, NULL, csi->clip_even_odd, gstate->ctm);
			csi->clip = 0;
		}

		if (csi->in_hidden_ocg > 0)
			dostroke = dofill = 0;

		if (dofill || dostroke)
			pdf_begin_group(csi, bbox);

		if (dofill)
		{
			switch (gstate->fill.kind)
			{
			case PDF_MAT_NONE:
				break;
			case PDF_MAT_COLOR:
				base_fill_path(csi->dev, path, even_odd, gstate->ctm,
					gstate->fill.colorspace, gstate->fill.v, gstate->fill.alpha);
				break;
			case PDF_MAT_PATTERN:
				if (gstate->fill.pattern)
				{
					base_clip_path(csi->dev, path, NULL, even_odd, gstate->ctm);
					pdf_show_pattern(csi, gstate->fill.pattern, bbox, PDF_FILL);
					base_pop_clip(csi->dev);
				}
				break;
			case PDF_MAT_SHADE:
				if (gstate->fill.shade)
				{
					base_clip_path(csi->dev, path, NULL, even_odd, gstate->ctm);
					base_fill_shade(csi->dev, gstate->fill.shade, csi->top_ctm, gstate->fill.alpha);
					base_pop_clip(csi->dev);
				}
				break;
			}
		}

		if (dostroke)
		{
			switch (gstate->stroke.kind)
			{
			case PDF_MAT_NONE:
				break;
			case PDF_MAT_COLOR:
				base_stroke_path(csi->dev, path, gstate->stroke_state, gstate->ctm,
					gstate->stroke.colorspace, gstate->stroke.v, gstate->stroke.alpha);
				break;
			case PDF_MAT_PATTERN:
				if (gstate->stroke.pattern)
				{
					base_clip_stroke_path(csi->dev, path, &bbox, gstate->stroke_state, gstate->ctm);
					pdf_show_pattern(csi, gstate->stroke.pattern, bbox, PDF_STROKE);
					base_pop_clip(csi->dev);
				}
				break;
			case PDF_MAT_SHADE:
				if (gstate->stroke.shade)
				{
					base_clip_stroke_path(csi->dev, path, &bbox, gstate->stroke_state, gstate->ctm);
					base_fill_shade(csi->dev, gstate->stroke.shade, csi->top_ctm, gstate->stroke.alpha);
					base_pop_clip(csi->dev);
				}
				break;
			}
		}

		if (dofill || dostroke)
			pdf_end_group(csi);
	}
	base_catch(ctx)
	{
		base_free_path(ctx, path);
		base_rethrow(ctx);
	}
	base_free_path(ctx, path);
}

static void
pdf_flush_text(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	base_text *text;
	int dofill = 0;
	int dostroke = 0;
	int doclip = 0;
	int doinvisible = 0;
	base_context *ctx = csi->dev->ctx;

	if (!csi->text)
		return;
	text = csi->text;
	csi->text = NULL;

	dofill = dostroke = doclip = doinvisible = 0;
	switch (csi->text_mode)
	{
	case 0: dofill = 1; break;
	case 1: dostroke = 1; break;
	case 2: dofill = dostroke = 1; break;
	case 3: doinvisible = 1; break;
	case 4: dofill = doclip = 1; break;
	case 5: dostroke = doclip = 1; break;
	case 6: dofill = dostroke = doclip = 1; break;
	case 7: doclip = 1; break;
	}

	if (csi->in_hidden_ocg > 0)
		dostroke = dofill = 0;

	base_try(ctx)
	{
		pdf_begin_group(csi, csi->text_bbox);

		if (doinvisible)
			base_ignore_text(csi->dev, text, gstate->ctm);

		if (doclip)
		{
			if (csi->accumulate < 2)
				gstate->clip_depth++;
			base_clip_text(csi->dev, text, gstate->ctm, csi->accumulate);
			csi->accumulate = 2;
		}

		if (dofill)
		{
			switch (gstate->fill.kind)
			{
			case PDF_MAT_NONE:
				break;
			case PDF_MAT_COLOR:
				base_fill_text(csi->dev, text, gstate->ctm,
					gstate->fill.colorspace, gstate->fill.v, gstate->fill.alpha);
				break;
			case PDF_MAT_PATTERN:
				if (gstate->fill.pattern)
				{
					base_clip_text(csi->dev, text, gstate->ctm, 0);
					pdf_show_pattern(csi, gstate->fill.pattern, csi->text_bbox, PDF_FILL);
					base_pop_clip(csi->dev);
				}
				break;
			case PDF_MAT_SHADE:
				if (gstate->fill.shade)
				{
					base_clip_text(csi->dev, text, gstate->ctm, 0);
					base_fill_shade(csi->dev, gstate->fill.shade, csi->top_ctm, gstate->fill.alpha);
					base_pop_clip(csi->dev);
				}
				break;
			}
		}

		if (dostroke)
		{
			switch (gstate->stroke.kind)
			{
			case PDF_MAT_NONE:
				break;
			case PDF_MAT_COLOR:
				base_stroke_text(csi->dev, text, gstate->stroke_state, gstate->ctm,
					gstate->stroke.colorspace, gstate->stroke.v, gstate->stroke.alpha);
				break;
			case PDF_MAT_PATTERN:
				if (gstate->stroke.pattern)
				{
					base_clip_stroke_text(csi->dev, text, gstate->stroke_state, gstate->ctm);
					pdf_show_pattern(csi, gstate->stroke.pattern, csi->text_bbox, PDF_STROKE);
					base_pop_clip(csi->dev);
				}
				break;
			case PDF_MAT_SHADE:
				if (gstate->stroke.shade)
				{
					base_clip_stroke_text(csi->dev, text, gstate->stroke_state, gstate->ctm);
					base_fill_shade(csi->dev, gstate->stroke.shade, csi->top_ctm, gstate->stroke.alpha);
					base_pop_clip(csi->dev);
				}
				break;
			}
		}

		pdf_end_group(csi);
	}
	base_catch(ctx)
	{
		base_free_text(ctx, text);
		base_rethrow(ctx);
	}

	base_free_text(ctx, text);
}

static void
pdf_show_char(pdf_csi *csi, int cid)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_font_desc *fontdesc = gstate->font;
	base_matrix tsm, trm;
	float w0, w1, tx, ty;
	pdf_hmtx h;
	pdf_vmtx v;
	int gid;
	int ucsbuf[8];
	int ucslen;
	int i;
	base_rect bbox;
	int render_direct;

	tsm.a = gstate->size * gstate->scale;
	tsm.b = 0;
	tsm.c = 0;
	tsm.d = gstate->size;
	tsm.e = 0;
	tsm.f = gstate->rise;

	ucslen = 0;
	if (fontdesc->to_unicode)
		ucslen = pdf_lookup_cmap_full(fontdesc->to_unicode, cid, ucsbuf);
	if (ucslen == 0 && cid < fontdesc->cid_to_ucs_len)
	{
		ucsbuf[0] = fontdesc->cid_to_ucs[cid];
		ucslen = 1;
	}
	if (ucslen == 0 || (ucslen == 1 && ucsbuf[0] == 0))
	{
		if (csi->dev->flags & base_DEVFLAG_ONLYIMAGE)
		{
			ucsbuf[0] = '?';
			ucslen = 1;
		}
		else
		{
			ucsbuf[0] = cid;
			ucslen = 1;

			if (ucsbuf[0] < 0x20)
				return;
		}
	}

	gid = pdf_font_cid_to_gid(ctx, fontdesc, cid);

	if (fontdesc->wmode == 1)
	{
		v = pdf_lookup_vmtx(ctx, fontdesc, cid);
		tsm.e -= v.x * gstate->size * 0.001f;
		tsm.f -= v.y * gstate->size * 0.001f;
	}

	trm = base_concat(tsm, csi->tm);

	bbox = base_bound_glyph(ctx, fontdesc->font, gid, trm);
	
	bbox.x0 -= 1;
	bbox.y0 -= 1;
	bbox.x1 += 1;
	bbox.y1 += 1;

	render_direct = !base_glyph_cacheable(ctx, fontdesc->font, gid);

	
	if (!csi->text ||
		fontdesc->font != csi->text->font ||
		fontdesc->wmode != csi->text->wmode ||
		fabsf(trm.a - csi->text->trm.a) > FLT_EPSILON ||
		fabsf(trm.b - csi->text->trm.b) > FLT_EPSILON ||
		fabsf(trm.c - csi->text->trm.c) > FLT_EPSILON ||
		fabsf(trm.d - csi->text->trm.d) > FLT_EPSILON ||
		gstate->render != csi->text_mode ||
		render_direct)
	{
		pdf_flush_text(csi);

		csi->text = base_new_text(ctx, fontdesc->font, trm, fontdesc->wmode);
		csi->text->trm.e = 0;
		csi->text->trm.f = 0;
		csi->text_mode = gstate->render;
		csi->text_bbox = base_empty_rect;
	}

	if (render_direct)
	{
		
		base_matrix composed = base_concat(trm, gstate->ctm);
		base_render_t3_glyph_direct(ctx, csi->dev, fontdesc->font, gid, composed, gstate);
	}
	else
	{
		csi->text_bbox = base_union_rect(csi->text_bbox, bbox);

		
		base_add_text(ctx, csi->text, gid, ucsbuf[0], trm.e, trm.f);

		
		for (i = 1; i < ucslen; i++)
			base_add_text(ctx, csi->text, -1, ucsbuf[i], trm.e, trm.f);
	}

	if (fontdesc->wmode == 0)
	{
		h = pdf_lookup_hmtx(ctx, fontdesc, cid);
		w0 = h.w * 0.001f;
		tx = (w0 * gstate->size + gstate->char_space) * gstate->scale;
		csi->tm = base_concat(base_translate(tx, 0), csi->tm);
	}

	if (fontdesc->wmode == 1)
	{
		w1 = v.w * 0.001f;
		ty = w1 * gstate->size + gstate->char_space;
		csi->tm = base_concat(base_translate(0, ty), csi->tm);
	}
}

static void
pdf_show_space(pdf_csi *csi, float tadj)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_font_desc *fontdesc = gstate->font;

	if (!fontdesc)
	{
		base_warn(ctx, "cannot draw text since font and size not set");
		return;
	}

	if (fontdesc->wmode == 0)
		csi->tm = base_concat(base_translate(tadj * gstate->scale, 0), csi->tm);
	else
		csi->tm = base_concat(base_translate(0, tadj), csi->tm);
}

static void
pdf_show_string(pdf_csi *csi, unsigned char *buf, int len)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_font_desc *fontdesc = gstate->font;
	unsigned char *end = buf + len;
	int cpt, cid;

	if (!fontdesc)
	{
		base_warn(ctx, "cannot draw text since font and size not set");
		return;
	}

	while (buf < end)
	{
		int w = pdf_decode_cmap(fontdesc->encoding, buf, &cpt);
		buf += w;

		cid = pdf_lookup_cmap(fontdesc->encoding, cpt);
		if (cid >= 0)
			pdf_show_char(csi, cid);
		else
			base_warn(ctx, "cannot encode character with code point %#x", cpt);
		if (cpt == 32 && w == 1)
			pdf_show_space(csi, gstate->word_space);
	}
}

static void
pdf_show_text(pdf_csi *csi, pdf_obj *text)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	int i;

	if (pdf_is_array(text))
	{
		int n = pdf_array_len(text);
		for (i = 0; i < n; i++)
		{
			pdf_obj *item = pdf_array_get(text, i);
			if (pdf_is_string(item))
				pdf_show_string(csi, (unsigned char *)pdf_to_str_buf(item), pdf_to_str_len(item));
			else
				pdf_show_space(csi, - pdf_to_real(item) * gstate->size * 0.001f);
		}
	}
	else if (pdf_is_string(text))
	{
		pdf_show_string(csi, (unsigned char *)pdf_to_str_buf(text), pdf_to_str_len(text));
	}
}

static void
pdf_init_gstate(base_context *ctx, pdf_gstate *gs, base_matrix ctm)
{
	gs->ctm = ctm;
	gs->clip_depth = 0;

	gs->stroke_state = base_new_stroke_state(ctx);

	gs->stroke.kind = PDF_MAT_COLOR;
	gs->stroke.colorspace = base_device_gray; 
	gs->stroke.v[0] = 0;
	gs->stroke.pattern = NULL;
	gs->stroke.shade = NULL;
	gs->stroke.alpha = 1;

	gs->fill.kind = PDF_MAT_COLOR;
	gs->fill.colorspace = base_device_gray; 
	gs->fill.v[0] = 0;
	gs->fill.pattern = NULL;
	gs->fill.shade = NULL;
	gs->fill.alpha = 1;

	gs->char_space = 0;
	gs->word_space = 0;
	gs->scale = 1;
	gs->leading = 0;
	gs->font = NULL;
	gs->size = -1;
	gs->render = 0;
	gs->rise = 0;

	gs->blendmode = 0;
	gs->softmask = NULL;
	gs->softmask_ctm = base_identity;
	gs->luminosity = 0;
}

static pdf_material *
pdf_keep_material(base_context *ctx, pdf_material *mat)
{
	if (mat->colorspace)
		base_keep_colorspace(ctx, mat->colorspace);
	if (mat->pattern)
		pdf_keep_pattern(ctx, mat->pattern);
	if (mat->shade)
		base_keep_shade(ctx, mat->shade);
	return mat;
}

static pdf_material *
pdf_drop_material(base_context *ctx, pdf_material *mat)
{
	if (mat->colorspace)
		base_drop_colorspace(ctx, mat->colorspace);
	if (mat->pattern)
		pdf_drop_pattern(ctx, mat->pattern);
	if (mat->shade)
		base_drop_shade(ctx, mat->shade);
	return mat;
}

static void
copy_state(base_context *ctx, pdf_gstate *gs, pdf_gstate *old)
{
	gs->stroke = old->stroke;
	gs->fill = old->fill;
	gs->font = old->font;
	gs->softmask = old->softmask;
	gs->stroke_state = base_keep_stroke_state(ctx, old->stroke_state);

	pdf_keep_material(ctx, &gs->stroke);
	pdf_keep_material(ctx, &gs->fill);
	if (gs->font)
		pdf_keep_font(ctx, gs->font);
	if (gs->softmask)
		pdf_keep_xobject(ctx, gs->softmask);
}

static pdf_csi *
pdf_new_csi(pdf_document *xref, base_device *dev, base_matrix ctm, char *event, base_cookie *cookie, pdf_gstate *gstate)
{
	pdf_csi *csi;
	base_context *ctx = dev->ctx;

	csi = base_malloc_struct(ctx, pdf_csi);
	base_try(ctx)
	{
		csi->xref = xref;
		csi->dev = dev;
		csi->event = event;

		csi->top = 0;
		csi->obj = NULL;
		csi->name[0] = 0;
		csi->string_len = 0;
		memset(csi->stack, 0, sizeof csi->stack);

		csi->xbalance = 0;
		csi->in_text = 0;
		csi->in_hidden_ocg = 0;

		csi->path = base_new_path(ctx);
		csi->clip = 0;
		csi->clip_even_odd = 0;

		csi->text = NULL;
		csi->tlm = base_identity;
		csi->tm = base_identity;
		csi->text_mode = 0;
		csi->accumulate = 1;

		csi->gcap = 64;
		csi->gstate = base_malloc_array(ctx, csi->gcap, sizeof(pdf_gstate));

		csi->top_ctm = ctm;
		pdf_init_gstate(ctx, &csi->gstate[0], ctm);
		if (gstate)
			copy_state(ctx, &csi->gstate[0], gstate);
		csi->gtop = 0;

		csi->cookie = cookie;
	}
	base_catch(ctx)
	{
		base_free_path(ctx, csi->path);
		base_free(ctx, csi);
		base_rethrow(ctx);
	}

	return csi;
}

static void
pdf_clear_stack(pdf_csi *csi)
{
	int i;

	if (csi->obj)
		pdf_drop_obj(csi->obj);
	csi->obj = NULL;

	csi->name[0] = 0;
	csi->string_len = 0;
	for (i = 0; i < csi->top; i++)
		csi->stack[i] = 0;

	csi->top = 0;
}

static void
pdf_gsave(pdf_csi *csi)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gs;

	if (csi->gtop == csi->gcap-1)
	{
		csi->gstate = base_resize_array(ctx, csi->gstate, csi->gcap*2, sizeof(pdf_gstate));
		csi->gcap *= 2;
	}

	memcpy(&csi->gstate[csi->gtop + 1], &csi->gstate[csi->gtop], sizeof(pdf_gstate));

	csi->gtop++;
	gs = &csi->gstate[csi->gtop];
	pdf_keep_material(ctx, &gs->stroke);
	pdf_keep_material(ctx, &gs->fill);
	if (gs->font)
		pdf_keep_font(ctx, gs->font);
	if (gs->softmask)
		pdf_keep_xobject(ctx, gs->softmask);
	base_keep_stroke_state(ctx, gs->stroke_state);
}

static void
pdf_grestore(pdf_csi *csi)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gs = csi->gstate + csi->gtop;
	int clip_depth = gs->clip_depth;

	if (csi->gtop == 0)
	{
		base_warn(ctx, "gstate underflow in content stream");
		return;
	}

	pdf_drop_material(ctx, &gs->stroke);
	pdf_drop_material(ctx, &gs->fill);
	if (gs->font)
		pdf_drop_font(ctx, gs->font);
	if (gs->softmask)
		pdf_drop_xobject(ctx, gs->softmask);
	base_drop_stroke_state(ctx, gs->stroke_state);

	csi->gtop --;

	gs = csi->gstate + csi->gtop;
	while (clip_depth > gs->clip_depth)
	{
		base_try(ctx)
		{
			base_pop_clip(csi->dev);
		}
		base_catch(ctx)
		{
			
		}
		clip_depth--;
	}
}

static void
pdf_free_csi(pdf_csi *csi)
{
	base_context *ctx = csi->dev->ctx;

	while (csi->gtop)
		pdf_grestore(csi);

	pdf_drop_material(ctx, &csi->gstate[0].fill);
	pdf_drop_material(ctx, &csi->gstate[0].stroke);
	if (csi->gstate[0].font)
		pdf_drop_font(ctx, csi->gstate[0].font);
	if (csi->gstate[0].softmask)
		pdf_drop_xobject(ctx, csi->gstate[0].softmask);
	base_drop_stroke_state(ctx, csi->gstate[0].stroke_state);

	while (csi->gstate[0].clip_depth--)
		base_pop_clip(csi->dev);

	if (csi->path) base_free_path(ctx, csi->path);
	if (csi->text) base_free_text(ctx, csi->text);

	pdf_clear_stack(csi);

	base_free(ctx, csi->gstate);

	base_free(ctx, csi);
}

static void
pdf_set_colorspace(pdf_csi *csi, int what, base_colorspace *colorspace)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gs = csi->gstate + csi->gtop;
	pdf_material *mat;

	pdf_flush_text(csi);

	mat = what == PDF_FILL ? &gs->fill : &gs->stroke;

	base_drop_colorspace(ctx, mat->colorspace);

	mat->kind = PDF_MAT_COLOR;
	mat->colorspace = base_keep_colorspace(ctx, colorspace);

	mat->v[0] = 0;
	mat->v[1] = 0;
	mat->v[2] = 0;
	mat->v[3] = 1;
}

static void
pdf_set_color(pdf_csi *csi, int what, float *v)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gs = csi->gstate + csi->gtop;
	pdf_material *mat;
	int i;

	pdf_flush_text(csi);

	mat = what == PDF_FILL ? &gs->fill : &gs->stroke;

	switch (mat->kind)
	{
	case PDF_MAT_PATTERN:
	case PDF_MAT_COLOR:
		if (!strcmp(mat->colorspace->name, "Lab"))
		{
			mat->v[0] = v[0] / 100;
			mat->v[1] = (v[1] + 100) / 200;
			mat->v[2] = (v[2] + 100) / 200;
		}
		for (i = 0; i < mat->colorspace->n; i++)
			mat->v[i] = v[i];
		break;
	default:
		base_warn(ctx, "color incompatible with material");
	}
}

static void
pdf_set_shade(pdf_csi *csi, int what, base_shade *shade)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gs = csi->gstate + csi->gtop;
	pdf_material *mat;

	pdf_flush_text(csi);

	mat = what == PDF_FILL ? &gs->fill : &gs->stroke;

	if (mat->shade)
		base_drop_shade(ctx, mat->shade);

	mat->kind = PDF_MAT_SHADE;
	mat->shade = base_keep_shade(ctx, shade);
}

static void
pdf_set_pattern(pdf_csi *csi, int what, pdf_pattern *pat, float *v)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gs = csi->gstate + csi->gtop;
	pdf_material *mat;

	pdf_flush_text(csi);

	mat = what == PDF_FILL ? &gs->fill : &gs->stroke;

	if (mat->pattern)
		pdf_drop_pattern(ctx, mat->pattern);

	mat->kind = PDF_MAT_PATTERN;
	if (pat)
		mat->pattern = pdf_keep_pattern(ctx, pat);
	else
		mat->pattern = NULL;

	if (v)
		pdf_set_color(csi, what, v);
}

static void
pdf_unset_pattern(pdf_csi *csi, int what)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gs = csi->gstate + csi->gtop;
	pdf_material *mat;
	mat = what == PDF_FILL ? &gs->fill : &gs->stroke;
	if (mat->kind == PDF_MAT_PATTERN)
	{
		if (mat->pattern)
			pdf_drop_pattern(ctx, mat->pattern);
		mat->pattern = NULL;
		mat->kind = PDF_MAT_COLOR;
	}
}

static void
pdf_show_pattern(pdf_csi *csi, pdf_pattern *pat, base_rect area, int what)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate;
	base_matrix ptm, invptm;
	base_matrix oldtopctm;
	int x0, y0, x1, y1;
	int oldtop;

	pdf_gsave(csi);
	gstate = csi->gstate + csi->gtop;

	if (pat->ismask)
	{
		pdf_unset_pattern(csi, PDF_FILL);
		pdf_unset_pattern(csi, PDF_STROKE);
		if (what == PDF_FILL)
		{
			pdf_drop_material(ctx, &gstate->stroke);
			pdf_keep_material(ctx, &gstate->fill);
			gstate->stroke = gstate->fill;
		}
		if (what == PDF_STROKE)
		{
			pdf_drop_material(ctx, &gstate->fill);
			pdf_keep_material(ctx, &gstate->stroke);
			gstate->fill = gstate->stroke;
		}
	}
	else
	{
		
		pdf_unset_pattern(csi, what);
	}

	
	if (gstate->softmask)
	{
		pdf_drop_xobject(ctx, gstate->softmask);
		gstate->softmask = NULL;
	}

	ptm = base_concat(pat->matrix, csi->top_ctm);
	invptm = base_invert_matrix(ptm);

	
	area = base_transform_rect(invptm, area);

	
	x0 = floorf((area.x0 - pat->bbox.x0) / pat->xstep + 0.001);
	y0 = floorf((area.y0 - pat->bbox.y0) / pat->ystep + 0.001);
	x1 = ceilf((area.x1 - pat->bbox.x0) / pat->xstep - 0.001);
	y1 = ceilf((area.y1 - pat->bbox.y0) / pat->ystep - 0.001);

	oldtopctm = csi->top_ctm;
	oldtop = csi->gtop;

#ifdef TILE
	if ((x1 - x0) * (y1 - y0) > 1)
#else
	if (0)
#endif
	{
		base_begin_tile(csi->dev, area, pat->bbox, pat->xstep, pat->ystep, ptm);
		gstate->ctm = ptm;
		csi->top_ctm = gstate->ctm;
		pdf_gsave(csi);
		pdf_run_buffer(csi, pat->resources, pat->contents);
		
		pdf_grestore(csi);
		while (oldtop < csi->gtop)
			pdf_grestore(csi);
		base_end_tile(csi->dev);
	}
	else
	{
		int x, y;
		for (y = y0; y < y1; y++)
		{
			for (x = x0; x < x1; x++)
			{
				gstate->ctm = base_concat(base_translate(x * pat->xstep, y * pat->ystep), ptm);
				csi->top_ctm = gstate->ctm;
				pdf_gsave(csi);
				base_try(ctx)
				{
					pdf_run_buffer(csi, pat->resources, pat->contents);
				}
				base_catch(ctx)
				{
					pdf_grestore(csi);
					while (oldtop < csi->gtop)
						pdf_grestore(csi);
					csi->top_ctm = oldtopctm;
					base_throw(ctx, "cannot render pattern tile");
				}
				pdf_grestore(csi);
				while (oldtop < csi->gtop)
					pdf_grestore(csi);
			}
		}
	}
	csi->top_ctm = oldtopctm;

	pdf_grestore(csi);
}

static void
pdf_run_xobject(pdf_csi *csi, pdf_obj *resources, pdf_xobject *xobj, base_matrix transform)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = NULL;
	base_matrix oldtopctm;
	int oldtop = 0;
	int popmask;

	
	if (xobj == NULL || pdf_dict_mark(xobj->me))
		return;

	base_var(gstate);
	base_var(oldtop);
	base_var(popmask);

	base_try(ctx)
	{
		pdf_gsave(csi);

		gstate = csi->gstate + csi->gtop;
		oldtop = csi->gtop;
		popmask = 0;

		
		transform = base_concat(xobj->matrix, transform);
		gstate->ctm = base_concat(transform, gstate->ctm);

		
		if (xobj->transparency)
		{
			if (gstate->softmask)
			{
				pdf_xobject *softmask = gstate->softmask;
				base_rect bbox = base_transform_rect(gstate->ctm, xobj->bbox);

				gstate->softmask = NULL;
				popmask = 1;

				base_begin_mask(csi->dev, bbox, gstate->luminosity,
					softmask->colorspace, gstate->softmask_bc);
				pdf_run_xobject(csi, resources, softmask, base_identity);
				
				base_end_mask(csi->dev);

				pdf_drop_xobject(ctx, softmask);
			}

			base_begin_group(csi->dev,
				base_transform_rect(gstate->ctm, xobj->bbox),
				xobj->isolated, xobj->knockout, gstate->blendmode, gstate->fill.alpha);

			gstate->blendmode = 0;
			gstate->stroke.alpha = 1;
			gstate->fill.alpha = 1;
		}

		

		base_moveto(ctx, csi->path, xobj->bbox.x0, xobj->bbox.y0);
		base_lineto(ctx, csi->path, xobj->bbox.x1, xobj->bbox.y0);
		base_lineto(ctx, csi->path, xobj->bbox.x1, xobj->bbox.y1);
		base_lineto(ctx, csi->path, xobj->bbox.x0, xobj->bbox.y1);
		base_closepath(ctx, csi->path);
		csi->clip = 1;
		pdf_show_path(csi, 0, 0, 0, 0);

		

		oldtopctm = csi->top_ctm;
		csi->top_ctm = gstate->ctm;

		if (xobj->resources)
			resources = xobj->resources;

		pdf_run_buffer(csi, resources, xobj->contents);
		
	}
	base_always(ctx)
	{
		if (gstate)
		{
			csi->top_ctm = oldtopctm;

			while (oldtop < csi->gtop)
				pdf_grestore(csi);

			pdf_grestore(csi);
		}

		pdf_dict_unmark(xobj->me);
	}
	base_catch(ctx)
	{
		base_rethrow(ctx);
	}

	
	if (xobj->transparency)
	{
		base_end_group(csi->dev);
		if (popmask)
			base_pop_clip(csi->dev);
	}
}

static void
pdf_run_extgstate(pdf_csi *csi, pdf_obj *rdb, pdf_obj *extgstate)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	base_colorspace *colorspace;
	int i, k, n;

	pdf_flush_text(csi);

	n = pdf_dict_len(extgstate);
	for (i = 0; i < n; i++)
	{
		pdf_obj *key = pdf_dict_get_key(extgstate, i);
		pdf_obj *val = pdf_dict_get_val(extgstate, i);
		char *s = pdf_to_name(key);

		if (!strcmp(s, "Font"))
		{
			if (pdf_is_array(val) && pdf_array_len(val) == 2)
			{
				pdf_obj *font = pdf_array_get(val, 0);

				if (gstate->font)
				{
					pdf_drop_font(ctx, gstate->font);
					gstate->font = NULL;
				}

				gstate->font = pdf_load_font(csi->xref, rdb, font);
				
				if (!gstate->font)
					base_throw(ctx, "cannot find font in store");
				gstate->size = pdf_to_real(pdf_array_get(val, 1));
			}
			else
				base_throw(ctx, "malformed /Font dictionary");
		}

		else if (!strcmp(s, "LC"))
		{
			csi->dev->flags &= ~(base_DEVFLAG_STARTCAP_UNDEFINED | base_DEVFLAG_DASHCAP_UNDEFINED | base_DEVFLAG_ENDCAP_UNDEFINED);
			gstate->stroke_state = base_unshare_stroke_state(ctx, gstate->stroke_state);
			gstate->stroke_state->start_cap = pdf_to_int(val);
			gstate->stroke_state->dash_cap = pdf_to_int(val);
			gstate->stroke_state->end_cap = pdf_to_int(val);
		}
		else if (!strcmp(s, "LW"))
		{
			csi->dev->flags &= ~base_DEVFLAG_LINEWIDTH_UNDEFINED;
			gstate->stroke_state = base_unshare_stroke_state(ctx, gstate->stroke_state);
			gstate->stroke_state->linewidth = pdf_to_real(val);
		}
		else if (!strcmp(s, "LJ"))
		{
			csi->dev->flags &= ~base_DEVFLAG_LINEJOIN_UNDEFINED;
			gstate->stroke_state = base_unshare_stroke_state(ctx, gstate->stroke_state);
			gstate->stroke_state->linejoin = pdf_to_int(val);
		}
		else if (!strcmp(s, "ML"))
		{
			csi->dev->flags &= ~base_DEVFLAG_MITERLIMIT_UNDEFINED;
			gstate->stroke_state = base_unshare_stroke_state(ctx, gstate->stroke_state);
			gstate->stroke_state->miterlimit = pdf_to_real(val);
		}

		else if (!strcmp(s, "D"))
		{
			if (pdf_is_array(val) && pdf_array_len(val) == 2)
			{
				pdf_obj *dashes = pdf_array_get(val, 0);
				int len = pdf_array_len(dashes);
				gstate->stroke_state = base_unshare_stroke_state_with_len(ctx, gstate->stroke_state, len);
				gstate->stroke_state->dash_len = len;
				for (k = 0; k < len; k++)
					gstate->stroke_state->dash_list[k] = pdf_to_real(pdf_array_get(dashes, k));
				gstate->stroke_state->dash_phase = pdf_to_real(pdf_array_get(val, 1));
			}
			else
				base_throw(ctx, "malformed /D");
		}

		else if (!strcmp(s, "CA"))
			gstate->stroke.alpha = pdf_to_real(val);

		else if (!strcmp(s, "ca"))
			gstate->fill.alpha = pdf_to_real(val);

		else if (!strcmp(s, "BM"))
		{
			if (pdf_is_array(val))
				val = pdf_array_get(val, 0);
			gstate->blendmode = base_lookup_blendmode(pdf_to_name(val));
		}

		else if (!strcmp(s, "SMask"))
		{
			if (pdf_is_dict(val))
			{
				pdf_xobject *xobj;
				pdf_obj *group, *luminosity, *bc;

				if (gstate->softmask)
				{
					pdf_drop_xobject(ctx, gstate->softmask);
					gstate->softmask = NULL;
				}

				group = pdf_dict_gets(val, "G");
				if (!group)
					base_throw(ctx, "cannot load softmask xobject (%d %d R)", pdf_to_num(val), pdf_to_gen(val));
				xobj = pdf_load_xobject(csi->xref, group);
				

				colorspace = xobj->colorspace;
				if (!colorspace)
					colorspace = base_device_gray;

				gstate->softmask_ctm = base_concat(xobj->matrix, gstate->ctm);
				gstate->softmask = xobj;
				for (k = 0; k < colorspace->n; k++)
					gstate->softmask_bc[k] = 0;

				bc = pdf_dict_gets(val, "BC");
				if (pdf_is_array(bc))
				{
					for (k = 0; k < colorspace->n; k++)
						gstate->softmask_bc[k] = pdf_to_real(pdf_array_get(bc, k));
				}

				luminosity = pdf_dict_gets(val, "S");
				if (pdf_is_name(luminosity) && !strcmp(pdf_to_name(luminosity), "Luminosity"))
					gstate->luminosity = 1;
				else
					gstate->luminosity = 0;
			}
			else if (pdf_is_name(val) && !strcmp(pdf_to_name(val), "None"))
			{
				if (gstate->softmask)
				{
					pdf_drop_xobject(ctx, gstate->softmask);
					gstate->softmask = NULL;
				}
			}
		}

		else if (!strcmp(s, "TR"))
		{
			if (!pdf_is_name(val) || strcmp(pdf_to_name(val), "Identity"))
				base_warn(ctx, "ignoring transfer function");
		}
	}
}

static void pdf_run_BDC(pdf_csi *csi, pdf_obj *rdb)
{
	pdf_obj *ocg;

	
	if (csi->in_hidden_ocg > 0)
	{
		csi->in_hidden_ocg++;
		return;
	}

	ocg = pdf_dict_gets(pdf_dict_gets(rdb, "Properties"), csi->name);
	if (!ocg)
	{
		
		return;
	}
	if (strcmp(pdf_to_name(pdf_dict_gets(ocg, "Type")), "OCG") != 0)
	{
		
		return;
	}
	if (pdf_is_hidden_ocg(ocg, csi, rdb))
		csi->in_hidden_ocg++;
}

static void pdf_run_BI(pdf_csi *csi, pdf_obj *rdb, base_stream *file)
{
	base_context *ctx = csi->dev->ctx;
	int ch;
	base_image *img;
	pdf_obj *obj;

	obj = pdf_parse_dict(csi->xref, file, &csi->xref->lexbuf.base);
	

	
	ch = base_read_byte(file);
	if (ch == '\r')
		if (base_peek_byte(file) == '\n')
			base_read_byte(file);

	img = pdf_load_inline_image(csi->xref, rdb, obj, file);
	pdf_drop_obj(obj);
	

	pdf_show_image(csi, img);

	base_drop_image(ctx, img);

	
	ch = base_read_byte(file);
	while (ch != 'E' && ch != EOF)
		ch = base_read_byte(file);
	ch = base_read_byte(file);
	if (ch != 'I')
		base_throw(ctx, "syntax error after inline image");
}

static void pdf_run_B(pdf_csi *csi)
{
	pdf_show_path(csi, 0, 1, 1, 0);
}

static void pdf_run_BMC(pdf_csi *csi)
{
	
	if (csi->in_hidden_ocg > 0)
	{
		csi->in_hidden_ocg++;
	}
}

static void pdf_run_BT(pdf_csi *csi)
{
	csi->in_text = 1;
	csi->tm = base_identity;
	csi->tlm = base_identity;
}

static void pdf_run_BX(pdf_csi *csi)
{
	csi->xbalance ++;
}

static void pdf_run_Bstar(pdf_csi *csi)
{
	pdf_show_path(csi, 0, 1, 1, 1);
}

static void pdf_run_cs_imp(pdf_csi *csi, pdf_obj *rdb, int what)
{
	base_context *ctx = csi->dev->ctx;
	base_colorspace *colorspace;
	pdf_obj *obj, *dict;

	if (!strcmp(csi->name, "Pattern"))
	{
		pdf_set_pattern(csi, what, NULL, NULL);
	}
	else
	{
		if (!strcmp(csi->name, "DeviceGray"))
			colorspace = base_device_gray; 
		else if (!strcmp(csi->name, "DeviceRGB"))
			colorspace = base_device_rgb; 
		else if (!strcmp(csi->name, "DeviceCMYK"))
			colorspace = base_device_cmyk; 
		else
		{
			dict = pdf_dict_gets(rdb, "ColorSpace");
			if (!dict)
				base_throw(ctx, "cannot find ColorSpace dictionary");
			obj = pdf_dict_gets(dict, csi->name);
			if (!obj)
				base_throw(ctx, "cannot find colorspace resource '%s'", csi->name);
			colorspace = pdf_load_colorspace(csi->xref, obj);
			
		}

		pdf_set_colorspace(csi, what, colorspace);

		base_drop_colorspace(ctx, colorspace);
	}
}

static void pdf_run_CS(pdf_csi *csi, pdf_obj *rdb)
{
	csi->dev->flags &= ~base_DEVFLAG_STROKECOLOR_UNDEFINED;

	pdf_run_cs_imp(csi, rdb, PDF_STROKE);
	
}

static void pdf_run_cs(pdf_csi *csi, pdf_obj *rdb)
{
	csi->dev->flags &= ~base_DEVFLAG_FILLCOLOR_UNDEFINED;

	pdf_run_cs_imp(csi, rdb, PDF_FILL);
	
}

static void pdf_run_DP(pdf_csi *csi)
{
}

static void pdf_run_Do(pdf_csi *csi, pdf_obj *rdb)
{
	base_context *ctx = csi->dev->ctx;
	pdf_obj *dict;
	pdf_obj *obj;
	pdf_obj *subtype;

	dict = pdf_dict_gets(rdb, "XObject");
	if (!dict)
		base_throw(ctx, "cannot find XObject dictionary when looking for: '%s'", csi->name);

	obj = pdf_dict_gets(dict, csi->name);
	if (!obj)
		base_throw(ctx, "cannot find xobject resource: '%s'", csi->name);

	subtype = pdf_dict_gets(obj, "Subtype");
	if (!pdf_is_name(subtype))
		base_throw(ctx, "no XObject subtype specified");

	if (pdf_is_hidden_ocg(pdf_dict_gets(obj, "OC"), csi, rdb))
		return;

	if (!strcmp(pdf_to_name(subtype), "Form") && pdf_dict_gets(obj, "Subtype2"))
		subtype = pdf_dict_gets(obj, "Subtype2");

	if (!strcmp(pdf_to_name(subtype), "Form"))
	{
		pdf_xobject *xobj;

		xobj = pdf_load_xobject(csi->xref, obj);
		

		
		if (!xobj->resources)
			xobj->resources = pdf_keep_obj(rdb);

		base_try(ctx)
		{
			pdf_run_xobject(csi, xobj->resources, xobj, base_identity);
		}
		base_catch(ctx)
		{
			pdf_drop_xobject(ctx, xobj);
			base_throw(ctx, "cannot draw xobject (%d %d R)", pdf_to_num(obj), pdf_to_gen(obj));
		}

		pdf_drop_xobject(ctx, xobj);
	}

	else if (!strcmp(pdf_to_name(subtype), "Image"))
	{
		if ((csi->dev->hints & base_IGNORE_IMAGE) == 0)
		{
			base_image *img = pdf_load_image(csi->xref, obj);
			
			base_try(ctx)
			{
				pdf_show_image(csi, img);
			}
			base_always(ctx)
			{
				base_drop_image(ctx, img);
			}
			base_catch(ctx)
			{
				base_rethrow(ctx);
			}
		}
	}

	else if (!strcmp(pdf_to_name(subtype), "PS"))
	{
		base_warn(ctx, "ignoring XObject with subtype PS");
	}

	else
	{
		base_throw(ctx, "unknown XObject subtype: '%s'", pdf_to_name(subtype));
	}
}

static void pdf_run_EMC(pdf_csi *csi)
{
	if (csi->in_hidden_ocg > 0)
		csi->in_hidden_ocg--;
}

static void pdf_run_ET(pdf_csi *csi)
{
	pdf_flush_text(csi);
	csi->accumulate = 1;
	csi->in_text = 0;
}

static void pdf_run_EX(pdf_csi *csi)
{
	csi->xbalance --;
}

static void pdf_run_F(pdf_csi *csi)
{
	pdf_show_path(csi, 0, 1, 0, 0);
}

static void pdf_run_G(pdf_csi *csi)
{
	csi->dev->flags &= ~base_DEVFLAG_STROKECOLOR_UNDEFINED;
	pdf_set_colorspace(csi, PDF_STROKE, base_device_gray);
	pdf_set_color(csi, PDF_STROKE, csi->stack);
}

static void pdf_run_J(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	csi->dev->flags &= ~(base_DEVFLAG_STARTCAP_UNDEFINED | base_DEVFLAG_DASHCAP_UNDEFINED | base_DEVFLAG_ENDCAP_UNDEFINED);
	gstate->stroke_state = base_unshare_stroke_state(csi->dev->ctx, gstate->stroke_state);
	gstate->stroke_state->start_cap = csi->stack[0];
	gstate->stroke_state->dash_cap = csi->stack[0];
	gstate->stroke_state->end_cap = csi->stack[0];
}

static void pdf_run_K(pdf_csi *csi)
{
	csi->dev->flags &= ~base_DEVFLAG_STROKECOLOR_UNDEFINED;
	pdf_set_colorspace(csi, PDF_STROKE, base_device_cmyk);
	pdf_set_color(csi, PDF_STROKE, csi->stack);
}

static void pdf_run_M(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	csi->dev->flags &= ~base_DEVFLAG_MITERLIMIT_UNDEFINED;
	gstate->stroke_state = base_unshare_stroke_state(csi->dev->ctx, gstate->stroke_state);
	gstate->stroke_state->miterlimit = csi->stack[0];
}

static void pdf_run_MP(pdf_csi *csi)
{
}

static void pdf_run_Q(pdf_csi *csi)
{
	pdf_grestore(csi);
}

static void pdf_run_RG(pdf_csi *csi)
{
	csi->dev->flags &= ~base_DEVFLAG_STROKECOLOR_UNDEFINED;
	pdf_set_colorspace(csi, PDF_STROKE, base_device_rgb);
	pdf_set_color(csi, PDF_STROKE, csi->stack);
}

static void pdf_run_S(pdf_csi *csi)
{
	pdf_show_path(csi, 0, 0, 1, 0);
}

static void pdf_run_SC_imp(pdf_csi *csi, pdf_obj *rdb, int what, pdf_material *mat)
{
	base_context *ctx = csi->dev->ctx;
	pdf_obj *patterntype;
	pdf_obj *dict;
	pdf_obj *obj;
	int kind;

	kind = mat->kind;
	if (csi->name[0])
		kind = PDF_MAT_PATTERN;

	switch (kind)
	{
	case PDF_MAT_NONE:
		base_throw(ctx, "cannot set color in mask objects");

	case PDF_MAT_COLOR:
		pdf_set_color(csi, what, csi->stack);
		break;

	case PDF_MAT_PATTERN:
		dict = pdf_dict_gets(rdb, "Pattern");
		if (!dict)
			base_throw(ctx, "cannot find Pattern dictionary");

		obj = pdf_dict_gets(dict, csi->name);
		if (!obj)
			base_throw(ctx, "cannot find pattern resource '%s'", csi->name);

		patterntype = pdf_dict_gets(obj, "PatternType");

		if (pdf_to_int(patterntype) == 1)
		{
			pdf_pattern *pat;
			pat = pdf_load_pattern(csi->xref, obj);
			
			pdf_set_pattern(csi, what, pat, csi->top > 0 ? csi->stack : NULL);
			pdf_drop_pattern(ctx, pat);
		}
		else if (pdf_to_int(patterntype) == 2)
		{
			base_shade *shd;
			shd = pdf_load_shading(csi->xref, obj);
			
			pdf_set_shade(csi, what, shd);
			base_drop_shade(ctx, shd);
		}
		else
		{
			base_throw(ctx, "unknown pattern type: %d", pdf_to_int(patterntype));
		}
		break;

	case PDF_MAT_SHADE:
		base_throw(ctx, "cannot set color in shade objects");
	}
}

static void pdf_run_SC(pdf_csi *csi, pdf_obj *rdb)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	csi->dev->flags &= ~base_DEVFLAG_STROKECOLOR_UNDEFINED;
	pdf_run_SC_imp(csi, rdb, PDF_STROKE, &gstate->stroke);
	
}

static void pdf_run_sc(pdf_csi *csi, pdf_obj *rdb)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	csi->dev->flags &= ~base_DEVFLAG_FILLCOLOR_UNDEFINED;
	pdf_run_SC_imp(csi, rdb, PDF_FILL, &gstate->fill);
	
}

static void pdf_run_Tc(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->char_space = csi->stack[0];
}

static void pdf_run_Tw(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->word_space = csi->stack[0];
}

static void pdf_run_Tz(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	float a = csi->stack[0] / 100;
	pdf_flush_text(csi);
	gstate->scale = a;
}

static void pdf_run_TL(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->leading = csi->stack[0];
}

static void pdf_run_Tf(pdf_csi *csi, pdf_obj *rdb)
{
	base_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_obj *dict;
	pdf_obj *obj;

	gstate->size = csi->stack[0];
	if (gstate->font)
		pdf_drop_font(ctx, gstate->font);
	gstate->font = NULL;

	dict = pdf_dict_gets(rdb, "Font");
	if (!dict)
		base_throw(ctx, "cannot find Font dictionary");

	obj = pdf_dict_gets(dict, csi->name);
	if (!obj)
		base_throw(ctx, "cannot find font resource: '%s'", csi->name);

	gstate->font = pdf_load_font(csi->xref, rdb, obj);
	
}

static void pdf_run_Tr(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->render = csi->stack[0];
}

static void pdf_run_Ts(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->rise = csi->stack[0];
}

static void pdf_run_Td(pdf_csi *csi)
{
	base_matrix m = base_translate(csi->stack[0], csi->stack[1]);
	csi->tlm = base_concat(m, csi->tlm);
	csi->tm = csi->tlm;
}

static void pdf_run_TD(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	base_matrix m;

	gstate->leading = -csi->stack[1];
	m = base_translate(csi->stack[0], csi->stack[1]);
	csi->tlm = base_concat(m, csi->tlm);
	csi->tm = csi->tlm;
}

static void pdf_run_Tm(pdf_csi *csi)
{
	csi->tm.a = csi->stack[0];
	csi->tm.b = csi->stack[1];
	csi->tm.c = csi->stack[2];
	csi->tm.d = csi->stack[3];
	csi->tm.e = csi->stack[4];
	csi->tm.f = csi->stack[5];
	csi->tlm = csi->tm;
}

static void pdf_run_Tstar(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	base_matrix m = base_translate(0, -gstate->leading);
	csi->tlm = base_concat(m, csi->tlm);
	csi->tm = csi->tlm;
}

static void pdf_run_Tj(pdf_csi *csi)
{
	if (csi->string_len)
		pdf_show_string(csi, csi->string, csi->string_len);
	else
		pdf_show_text(csi, csi->obj);
}

static void pdf_run_TJ(pdf_csi *csi)
{
	if (csi->string_len)
		pdf_show_string(csi, csi->string, csi->string_len);
	else
		pdf_show_text(csi, csi->obj);
}

static void pdf_run_W(pdf_csi *csi)
{
	csi->clip = 1;
	csi->clip_even_odd = 0;
}

static void pdf_run_Wstar(pdf_csi *csi)
{
	csi->clip = 1;
	csi->clip_even_odd = 1;
}

static void pdf_run_b(pdf_csi *csi)
{
	pdf_show_path(csi, 1, 1, 1, 0);
}

static void pdf_run_bstar(pdf_csi *csi)
{
	pdf_show_path(csi, 1, 1, 1, 1);
}

static void pdf_run_c(pdf_csi *csi)
{
	float a, b, c, d, e, f;
	a = csi->stack[0];
	b = csi->stack[1];
	c = csi->stack[2];
	d = csi->stack[3];
	e = csi->stack[4];
	f = csi->stack[5];
	base_curveto(csi->dev->ctx, csi->path, a, b, c, d, e, f);
}

static void pdf_run_cm(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	base_matrix m;

	m.a = csi->stack[0];
	m.b = csi->stack[1];
	m.c = csi->stack[2];
	m.d = csi->stack[3];
	m.e = csi->stack[4];
	m.f = csi->stack[5];

	gstate->ctm = base_concat(m, gstate->ctm);
}

static void pdf_run_d(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_obj *array;
	int i;
	int len;

	array = csi->obj;
	len = pdf_array_len(array);
	gstate->stroke_state = base_unshare_stroke_state_with_len(csi->dev->ctx, gstate->stroke_state, len);
	gstate->stroke_state->dash_len = len;
	for (i = 0; i < len; i++)
		gstate->stroke_state->dash_list[i] = pdf_to_real(pdf_array_get(array, i));
	gstate->stroke_state->dash_phase = csi->stack[0];
}

static void pdf_run_d0(pdf_csi *csi)
{
	csi->dev->flags |= base_DEVFLAG_COLOR;
}

static void pdf_run_d1(pdf_csi *csi)
{
	csi->dev->flags |= base_DEVFLAG_MASK;
	csi->dev->flags &= ~(base_DEVFLAG_FILLCOLOR_UNDEFINED |
				base_DEVFLAG_STROKECOLOR_UNDEFINED |
				base_DEVFLAG_STARTCAP_UNDEFINED |
				base_DEVFLAG_DASHCAP_UNDEFINED |
				base_DEVFLAG_ENDCAP_UNDEFINED |
				base_DEVFLAG_LINEJOIN_UNDEFINED |
				base_DEVFLAG_MITERLIMIT_UNDEFINED |
				base_DEVFLAG_LINEWIDTH_UNDEFINED);
}

static void pdf_run_f(pdf_csi *csi)
{
	pdf_show_path(csi, 0, 1, 0, 0);
}

static void pdf_run_fstar(pdf_csi *csi)
{
	pdf_show_path(csi, 0, 1, 0, 1);
}

static void pdf_run_g(pdf_csi *csi)
{
	csi->dev->flags &= ~base_DEVFLAG_FILLCOLOR_UNDEFINED;
	pdf_set_colorspace(csi, PDF_FILL, base_device_gray);
	pdf_set_color(csi, PDF_FILL, csi->stack);
}

static void pdf_run_gs(pdf_csi *csi, pdf_obj *rdb)
{
	pdf_obj *dict;
	pdf_obj *obj;
	base_context *ctx = csi->dev->ctx;

	dict = pdf_dict_gets(rdb, "ExtGState");
	if (!dict)
		base_throw(ctx, "cannot find ExtGState dictionary");

	obj = pdf_dict_gets(dict, csi->name);
	if (!obj)
		base_throw(ctx, "cannot find extgstate resource '%s'", csi->name);

	pdf_run_extgstate(csi, rdb, obj);
	
}

static void pdf_run_h(pdf_csi *csi)
{
	base_closepath(csi->dev->ctx, csi->path);
}

static void pdf_run_i(pdf_csi *csi)
{
}

static void pdf_run_j(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	csi->dev->flags &= ~base_DEVFLAG_LINEJOIN_UNDEFINED;
	gstate->stroke_state = base_unshare_stroke_state(csi->dev->ctx, gstate->stroke_state);
	gstate->stroke_state->linejoin = csi->stack[0];
}

static void pdf_run_k(pdf_csi *csi)
{
	csi->dev->flags &= ~base_DEVFLAG_FILLCOLOR_UNDEFINED;
	pdf_set_colorspace(csi, PDF_FILL, base_device_cmyk);
	pdf_set_color(csi, PDF_FILL, csi->stack);
}

static void pdf_run_l(pdf_csi *csi)
{
	float a, b;
	a = csi->stack[0];
	b = csi->stack[1];
	base_lineto(csi->dev->ctx, csi->path, a, b);
}

static void pdf_run_m(pdf_csi *csi)
{
	float a, b;
	a = csi->stack[0];
	b = csi->stack[1];
	base_moveto(csi->dev->ctx, csi->path, a, b);
}

static void pdf_run_n(pdf_csi *csi)
{
	pdf_show_path(csi, 0, 0, 0, 0);
}

static void pdf_run_q(pdf_csi *csi)
{
	pdf_gsave(csi);
}

static void pdf_run_re(pdf_csi *csi)
{
	base_context *ctx = csi->dev->ctx;
	float x, y, w, h;

	x = csi->stack[0];
	y = csi->stack[1];
	w = csi->stack[2];
	h = csi->stack[3];

	base_moveto(ctx, csi->path, x, y);
	base_lineto(ctx, csi->path, x + w, y);
	base_lineto(ctx, csi->path, x + w, y + h);
	base_lineto(ctx, csi->path, x, y + h);
	base_closepath(ctx, csi->path);
}

static void pdf_run_rg(pdf_csi *csi)
{
	csi->dev->flags &= ~base_DEVFLAG_FILLCOLOR_UNDEFINED;
	pdf_set_colorspace(csi, PDF_FILL, base_device_rgb);
	pdf_set_color(csi, PDF_FILL, csi->stack);
}

static void pdf_run_ri(pdf_csi *csi)
{
}

static void pdf_run(pdf_csi *csi)
{
	pdf_show_path(csi, 1, 0, 1, 0);
}

static void pdf_run_sh(pdf_csi *csi, pdf_obj *rdb)
{
	base_context *ctx = csi->dev->ctx;
	pdf_obj *dict;
	pdf_obj *obj;
	base_shade *shd;

	dict = pdf_dict_gets(rdb, "Shading");
	if (!dict)
		base_throw(ctx, "cannot find shading dictionary");

	obj = pdf_dict_gets(dict, csi->name);
	if (!obj)
		base_throw(ctx, "cannot find shading resource: '%s'", csi->name);

	if ((csi->dev->hints & base_IGNORE_SHADE) == 0)
	{
		shd = pdf_load_shading(csi->xref, obj);
		
		base_try(ctx)
		{
			pdf_show_shade(csi, shd);
		}
		base_catch(ctx)
		{
			base_drop_shade(ctx, shd);
			base_rethrow(ctx);
		}
		base_drop_shade(ctx, shd);
	}
}

static void pdf_run_v(pdf_csi *csi)
{
	float a, b, c, d;
	a = csi->stack[0];
	b = csi->stack[1];
	c = csi->stack[2];
	d = csi->stack[3];
	base_curvetov(csi->dev->ctx, csi->path, a, b, c, d);
}

static void pdf_run_w(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_flush_text(csi); 
	csi->dev->flags &= ~base_DEVFLAG_LINEWIDTH_UNDEFINED;
	gstate->stroke_state = base_unshare_stroke_state(csi->dev->ctx, gstate->stroke_state);
	gstate->stroke_state->linewidth = csi->stack[0];
}

static void pdf_run_y(pdf_csi *csi)
{
	float a, b, c, d;
	a = csi->stack[0];
	b = csi->stack[1];
	c = csi->stack[2];
	d = csi->stack[3];
	base_curvetoy(csi->dev->ctx, csi->path, a, b, c, d);
}

static void pdf_run_squote(pdf_csi *csi)
{
	base_matrix m;
	pdf_gstate *gstate = csi->gstate + csi->gtop;

	m = base_translate(0, -gstate->leading);
	csi->tlm = base_concat(m, csi->tlm);
	csi->tm = csi->tlm;

	if (csi->string_len)
		pdf_show_string(csi, csi->string, csi->string_len);
	else
		pdf_show_text(csi, csi->obj);
}

static void pdf_run_dquote(pdf_csi *csi)
{
	base_matrix m;
	pdf_gstate *gstate = csi->gstate + csi->gtop;

	gstate->word_space = csi->stack[0];
	gstate->char_space = csi->stack[1];

	m = base_translate(0, -gstate->leading);
	csi->tlm = base_concat(m, csi->tlm);
	csi->tm = csi->tlm;

	if (csi->string_len)
		pdf_show_string(csi, csi->string, csi->string_len);
	else
		pdf_show_text(csi, csi->obj);
}

#define A(a) (a)
#define B(a,b) (a | b << 8)
#define C(a,b,c) (a | b << 8 | c << 16)

static void
pdf_run_keyword(pdf_csi *csi, pdf_obj *rdb, base_stream *file, char *buf)
{
	base_context *ctx = csi->dev->ctx;
	int key;

	key = buf[0];
	if (buf[1])
	{
		key |= buf[1] << 8;
		if (buf[2])
		{
			key |= buf[2] << 16;
			if (buf[3])
				key = 0;
		}
	}

	switch (key)
	{
	case A('"'): pdf_run_dquote(csi); break;
	case A('\''): pdf_run_squote(csi); break;
	case A('B'): pdf_run_B(csi); break;
	case B('B','*'): pdf_run_Bstar(csi); break;
	case C('B','D','C'): pdf_run_BDC(csi, rdb); break;
	case B('B','I'):
		pdf_run_BI(csi, rdb, file);
		
		break;
	case C('B','M','C'): pdf_run_BMC(csi); break;
	case B('B','T'): pdf_run_BT(csi); break;
	case B('B','X'): pdf_run_BX(csi); break;
	case B('C','S'): pdf_run_CS(csi, rdb); break;
	case B('D','P'): pdf_run_DP(csi); break;
	case B('D','o'):
		base_try(ctx)
		{
			pdf_run_Do(csi, rdb);
		}
		base_catch(ctx)
		{
			base_warn(ctx, "cannot draw xobject/image");
		}
		break;
	case C('E','M','C'): pdf_run_EMC(csi); break;
	case B('E','T'): pdf_run_ET(csi); break;
	case B('E','X'): pdf_run_EX(csi); break;
	case A('F'): pdf_run_F(csi); break;
	case A('G'): pdf_run_G(csi); break;
	case A('J'): pdf_run_J(csi); break;
	case A('K'): pdf_run_K(csi); break;
	case A('M'): pdf_run_M(csi); break;
	case B('M','P'): pdf_run_MP(csi); break;
	case A('Q'): pdf_run_Q(csi); break;
	case B('R','G'): pdf_run_RG(csi); break;
	case A('S'): pdf_run_S(csi); break;
	case B('S','C'): pdf_run_SC(csi, rdb); break;
	case C('S','C','N'): pdf_run_SC(csi, rdb); break;
	case B('T','*'): pdf_run_Tstar(csi); break;
	case B('T','D'): pdf_run_TD(csi); break;
	case B('T','J'): pdf_run_TJ(csi); break;
	case B('T','L'): pdf_run_TL(csi); break;
	case B('T','c'): pdf_run_Tc(csi); break;
	case B('T','d'): pdf_run_Td(csi); break;
	case B('T','f'):
		base_try(ctx)
		{
			pdf_run_Tf(csi, rdb);
		}
		base_catch(ctx)
		{
			base_warn(ctx, "cannot set font");
		}
		break;
	case B('T','j'): pdf_run_Tj(csi); break;
	case B('T','m'): pdf_run_Tm(csi); break;
	case B('T','r'): pdf_run_Tr(csi); break;
	case B('T','s'): pdf_run_Ts(csi); break;
	case B('T','w'): pdf_run_Tw(csi); break;
	case B('T','z'): pdf_run_Tz(csi); break;
	case A('W'): pdf_run_W(csi); break;
	case B('W','*'): pdf_run_Wstar(csi); break;
	case A('b'): pdf_run_b(csi); break;
	case B('b','*'): pdf_run_bstar(csi); break;
	case A('c'): pdf_run_c(csi); break;
	case B('c','m'): pdf_run_cm(csi); break;
	case B('c','s'): pdf_run_cs(csi, rdb); break;
	case A('d'): pdf_run_d(csi); break;
	case B('d','0'): pdf_run_d0(csi); break;
	case B('d','1'): pdf_run_d1(csi); break;
	case A('f'): pdf_run_f(csi); break;
	case B('f','*'): pdf_run_fstar(csi); break;
	case A('g'): pdf_run_g(csi); break;
	case B('g','s'):
		base_try(ctx)
		{
			pdf_run_gs(csi, rdb);
		}
		base_catch(ctx)
		{
			base_warn(ctx, "cannot set graphics state");
		}
		break;
	case A('h'): pdf_run_h(csi); break;
	case A('i'): pdf_run_i(csi); break;
	case A('j'): pdf_run_j(csi); break;
	case A('k'): pdf_run_k(csi); break;
	case A('l'): pdf_run_l(csi); break;
	case A('m'): pdf_run_m(csi); break;
	case A('n'): pdf_run_n(csi); break;
	case A('q'): pdf_run_q(csi); break;
	case B('r','e'): pdf_run_re(csi); break;
	case B('r','g'): pdf_run_rg(csi); break;
	case B('r','i'): pdf_run_ri(csi); break;
	case A('s'): pdf_run(csi); break;
	case B('s','c'): pdf_run_sc(csi, rdb); break;
	case C('s','c','n'): pdf_run_sc(csi, rdb); break;
	case B('s','h'):
		base_try(ctx)
		{
			pdf_run_sh(csi, rdb);
		}
		base_catch(ctx)
		{
			base_warn(ctx, "cannot draw shading");
		}
		break;
	case A('v'): pdf_run_v(csi); break;
	case A('w'): pdf_run_w(csi); break;
	case A('y'): pdf_run_y(csi); break;
	default:
		if (!csi->xbalance)
			base_warn(ctx, "unknown keyword: '%s'", buf);
		break;
	}
	base_assert_lock_not_held(ctx, base_LOCK_FILE);
}

static void
pdf_run_stream(pdf_csi *csi, pdf_obj *rdb, base_stream *file, pdf_lexbuf *buf)
{
	base_context *ctx = csi->dev->ctx;
	int tok, in_array;

	
	pdf_clear_stack(csi);
	in_array = 0;

	if (csi->cookie)
	{
		csi->cookie->progress_max = -1;
		csi->cookie->progress = 0;
	}

	while (1)
	{
		if (csi->top == nelem(csi->stack) - 1)
			base_throw(ctx, "stack overflow");

		
		if (csi->cookie)
		{
			if (csi->cookie->abort)
				break;
			csi->cookie->progress++;
		}

		tok = pdf_lex(file, buf);
		

		if (in_array)
		{
			if (tok == PDF_TOK_CLOSE_ARRAY)
			{
				in_array = 0;
			}
			else if (tok == PDF_TOK_REAL)
			{
				pdf_gstate *gstate = csi->gstate + csi->gtop;
				pdf_show_space(csi, -buf->f * gstate->size * 0.001f);
			}
			else if (tok == PDF_TOK_INT)
			{
				pdf_gstate *gstate = csi->gstate + csi->gtop;
				pdf_show_space(csi, -buf->i * gstate->size * 0.001f);
			}
			else if (tok == PDF_TOK_STRING)
			{
				pdf_show_string(csi, (unsigned char *)buf->scratch, buf->len);
			}
			else if (tok == PDF_TOK_KEYWORD)
			{
				if (!strcmp(buf->scratch, "Tw") || !strcmp(buf->scratch, "Tc"))
					base_warn(ctx, "ignoring keyword '%s' inside array", buf->scratch);
				else
					base_throw(ctx, "syntax error in array");
			}
			else if (tok == PDF_TOK_EOF)
				return;
			else
				base_throw(ctx, "syntax error in array");
		}

		else switch (tok)
		{
		case PDF_TOK_ENDSTREAM:
		case PDF_TOK_EOF:
			return;

		case PDF_TOK_OPEN_ARRAY:
			if (!csi->in_text)
			{
				csi->obj = pdf_parse_array(csi->xref, file, buf);
				
			}
			else
			{
				in_array = 1;
			}
			break;

		case PDF_TOK_OPEN_DICT:
			csi->obj = pdf_parse_dict(csi->xref, file, buf);
			
			break;

		case PDF_TOK_NAME:
			base_strlcpy(csi->name, buf->scratch, sizeof(csi->name));
			break;

		case PDF_TOK_INT:
			csi->stack[csi->top] = buf->i;
			csi->top ++;
			break;

		case PDF_TOK_REAL:
			csi->stack[csi->top] = buf->f;
			csi->top ++;
			break;

		case PDF_TOK_STRING:
			if (buf->len <= sizeof(csi->string))
			{
				memcpy(csi->string, buf->scratch, buf->len);
				csi->string_len = buf->len;
			}
			else
			{
				csi->obj = pdf_new_string(ctx, buf->scratch, buf->len);
			}
			break;

		case PDF_TOK_KEYWORD:
			pdf_run_keyword(csi, rdb, file, buf->scratch);
			
			pdf_clear_stack(csi);
			break;

		default:
			base_throw(ctx, "syntax error in content stream");
		}
	}
}

static void
pdf_run_buffer(pdf_csi *csi, pdf_obj *rdb, base_buffer *contents)
{
	base_context *ctx = csi->dev->ctx;
	pdf_lexbuf_large *buf;
	base_stream * file = NULL;
	int save_in_text;

	base_var(buf);
	base_var(file);

	if (contents == NULL)
		return;

	base_try(ctx)
	{
		buf = base_malloc(ctx, sizeof(*buf)); 
		buf->base.size = PDF_LEXBUF_LARGE;
		file = base_open_buffer(ctx, contents);
		save_in_text = csi->in_text;
		csi->in_text = 0;
		base_try(ctx)
		{
			pdf_run_stream(csi, rdb, file, &buf->base);
		}
		base_catch(ctx)
		{
			base_warn(ctx, "Content stream parsing error - rendering truncated");
		}
		csi->in_text = save_in_text;
	}
	base_always(ctx)
	{
		base_close(file);
		base_free(ctx, buf);
	}
	base_catch(ctx)
	{
		base_throw(ctx, "cannot parse context stream");
	}
}

void
pdf_run_page_with_usage(pdf_document *xref, pdf_page *page, base_device *dev, base_matrix ctm, char *event, base_cookie *cookie)
{
	base_context *ctx = dev->ctx;
	pdf_csi *csi;
	pdf_annot *annot;
	int flags;

	ctm = base_concat(page->ctm, ctm);

	if (page->transparency)
		base_begin_group(dev, base_transform_rect(ctm, page->mediabox), 1, 0, 0, 1);

	csi = pdf_new_csi(xref, dev, ctm, event, cookie, NULL);
	base_try(ctx)
	{
		pdf_run_buffer(csi, page->resources, page->contents);
	}
	base_catch(ctx)
	{
		pdf_free_csi(csi);
		base_throw(ctx, "cannot parse page content stream");
	}
	pdf_free_csi(csi);

	if (cookie && cookie->progress_max != -1)
	{
		int count = 1;
		for (annot = page->annots; annot; annot = annot->next)
			count++;
		cookie->progress_max += count;
	}

	for (annot = page->annots; annot; annot = annot->next)
	{
		
		if (cookie)
		{
			if (cookie->abort)
				break;
			cookie->progress++;
		}

		flags = pdf_to_int(pdf_dict_gets(annot->obj, "F"));

		
		if (flags & (1 << 0)) 
			continue;
		if (flags & (1 << 1)) 
			continue;
		if (!strcmp(event, "Print") && !(flags & (1 << 2))) 
			continue;
		if (!strcmp(event, "View") && (flags & (1 << 5))) 
			continue;

		csi = pdf_new_csi(xref, dev, ctm, event, cookie, NULL);
		if (!pdf_is_hidden_ocg(pdf_dict_gets(annot->obj, "OC"), csi, page->resources))
		{
			base_try(ctx)
			{
				pdf_run_xobject(csi, page->resources, annot->ap, annot->matrix);
			}
			base_catch(ctx)
			{
				pdf_free_csi(csi);
				base_throw(ctx, "cannot parse annotation appearance stream");
			}
		}
		pdf_free_csi(csi);
	}

	if (page->transparency)
		base_end_group(dev);
}

void
pdf_run_page(pdf_document *xref, pdf_page *page, base_device *dev, base_matrix ctm, base_cookie *cookie)
{
	pdf_run_page_with_usage(xref, page, dev, ctm, "View", cookie);
}

void
pdf_run_glyph(pdf_document *xref, pdf_obj *resources, base_buffer *contents, base_device *dev, base_matrix ctm, void *gstate)
{
	pdf_csi *csi = pdf_new_csi(xref, dev, ctm, "View", NULL, gstate);
	base_context *ctx = xref->ctx;

	base_try(ctx)
	{
		pdf_run_buffer(csi, resources, contents);
	}
	base_catch(ctx)
	{
		pdf_free_csi(csi);
		base_throw(ctx, "cannot parse glyph content stream");
	}
	pdf_free_csi(csi);
}
