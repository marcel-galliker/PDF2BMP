#include "pdf-internal.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

#define MAX_BBOX_TABLE_SIZE 4096

static void base_drop_freetype(base_context *ctx);

static base_font *
base_new_font(base_context *ctx, char *name, int use_glyph_bbox, int glyph_count)
{
	base_font *font;
	int i;

	font = base_malloc_struct(ctx, base_font);
	font->refs = 1;

	if (name)
		base_strlcpy(font->name, name, sizeof font->name);
	else
		base_strlcpy(font->name, "(null)", sizeof font->name);

	font->ft_face = NULL;
	font->ft_substitute = 0;
	font->ft_bold = 0;
	font->ft_italic = 0;
	font->ft_hint = 0;

	font->ft_file = NULL;
	font->ft_data = NULL;
	font->ft_size = 0;

	font->t3matrix = base_identity;
	font->t3resources = NULL;
	font->t3procs = NULL;
	font->t3widths = NULL;
	font->t3flags = NULL;
	font->t3doc = NULL;
	font->t3run = NULL;

	font->bbox.x0 = 0;
	font->bbox.y0 = 0;
	font->bbox.x1 = 1;
	font->bbox.y1 = 1;

	font->use_glyph_bbox = use_glyph_bbox;
	if (use_glyph_bbox && glyph_count <= MAX_BBOX_TABLE_SIZE)
	{
		font->bbox_count = glyph_count;
		font->bbox_table = base_malloc_array(ctx, glyph_count, sizeof(base_rect));
		for (i = 0; i < glyph_count; i++)
			font->bbox_table[i] = base_infinite_rect;
	}
	else
	{
		if (use_glyph_bbox)
			base_warn(ctx, "not building glyph bbox table for font '%s' with %d glyphs", name, glyph_count);
		font->bbox_count = 0;
		font->bbox_table = NULL;
	}

	font->width_count = 0;
	font->width_table = NULL;

	return font;
}

base_font *
base_keep_font(base_context *ctx, base_font *font)
{
	if (!font)
		return NULL;
	base_lock(ctx, base_LOCK_ALLOC);
	font->refs ++;
	base_unlock(ctx, base_LOCK_ALLOC);
	return font;
}

void
base_drop_font(base_context *ctx, base_font *font)
{
	int fterr;
	int i, drop;

	base_lock(ctx, base_LOCK_ALLOC);
	drop = (font && --font->refs == 0);
	base_unlock(ctx, base_LOCK_ALLOC);
	if (!drop)
		return;

	if (font->t3procs)
	{
		if (font->t3resources)
			font->t3freeres(font->t3doc, font->t3resources);
		for (i = 0; i < 256; i++)
			if (font->t3procs[i])
				base_drop_buffer(ctx, font->t3procs[i]);
		base_free(ctx, font->t3procs);
		base_free(ctx, font->t3widths);
		base_free(ctx, font->t3flags);
	}

	if (font->ft_face)
	{
		base_lock(ctx, base_LOCK_FREETYPE);
		fterr = FT_Done_Face((FT_Face)font->ft_face);
		base_unlock(ctx, base_LOCK_FREETYPE);
		if (fterr)
			base_warn(ctx, "freetype finalizing face: %s", ft_error_string(fterr));
		base_drop_freetype(ctx);
	}

	base_free(ctx, font->ft_file);
	base_free(ctx, font->ft_data);
	base_free(ctx, font->bbox_table);
	base_free(ctx, font->width_table);
	base_free(ctx, font);
}

void
base_set_font_bbox(base_context *ctx, base_font *font, float xmin, float ymin, float xmax, float ymax)
{
	font->bbox.x0 = xmin;
	font->bbox.y0 = ymin;
	font->bbox.x1 = xmax;
	font->bbox.y1 = ymax;
}

struct base_font_context_s {
	int ctx_refs;
	FT_Library ftlib;
	int ftlib_refs;
};

#undef __FTERRORS_H__
#define FT_ERRORDEF(e, v, s)	{ (e), (s) },
#define FT_ERROR_START_LIST
#define FT_ERROR_END_LIST	{ 0, NULL }

struct ft_error
{
	int err;
	char *str;
};

void base_new_font_context(base_context *ctx)
{
	ctx->font = base_malloc_struct(ctx, base_font_context);
	ctx->font->ctx_refs = 1;
	ctx->font->ftlib = NULL;
	ctx->font->ftlib_refs = 0;
}

base_font_context *
base_keep_font_context(base_context *ctx)
{
	if (!ctx || !ctx->font)
		return NULL;
	base_lock(ctx, base_LOCK_ALLOC);
	ctx->font->ctx_refs++;
	base_unlock(ctx, base_LOCK_ALLOC);
	return ctx->font;
}

void base_drop_font_context(base_context *ctx)
{
	int drop;
	if (!ctx || !ctx->font)
		return;
	base_lock(ctx, base_LOCK_ALLOC);
	drop = --ctx->font->ctx_refs;
	base_unlock(ctx, base_LOCK_ALLOC);
	if (drop == 0)
		base_free(ctx, ctx->font);
}

static const struct ft_error ft_errors[] =
{
#include FT_ERRORS_H
};

char *ft_error_string(int err)
{
	const struct ft_error *e;

	for (e = ft_errors; e->str; e++)
		if (e->err == err)
			return e->str;

	return "Unknown error";
}

static void
base_keep_freetype(base_context *ctx)
{
	int fterr;
	int maj, min, pat;
	base_font_context *fct = ctx->font;

	base_lock(ctx, base_LOCK_FREETYPE);
	if (fct->ftlib)
	{
		fct->ftlib_refs++;
		base_unlock(ctx, base_LOCK_FREETYPE);
		return;
	}

	fterr = FT_Init_FreeType(&fct->ftlib);
	if (fterr)
	{
		char *mess = ft_error_string(fterr);
		base_unlock(ctx, base_LOCK_FREETYPE);
		base_throw(ctx, "cannot init freetype: %s", mess);
	}

	FT_Library_Version(fct->ftlib, &maj, &min, &pat);
	if (maj == 2 && min == 1 && pat < 7)
	{
		fterr = FT_Done_FreeType(fct->ftlib);
		if (fterr)
			base_warn(ctx, "freetype finalizing: %s", ft_error_string(fterr));
		base_unlock(ctx, base_LOCK_FREETYPE);
		base_throw(ctx, "freetype version too old: %d.%d.%d", maj, min, pat);
	}

	fct->ftlib_refs++;
	base_unlock(ctx, base_LOCK_FREETYPE);
}

static void
base_drop_freetype(base_context *ctx)
{
	int fterr;
	base_font_context *fct = ctx->font;

	base_lock(ctx, base_LOCK_FREETYPE);
	if (--fct->ftlib_refs == 0)
	{
		fterr = FT_Done_FreeType(fct->ftlib);
		if (fterr)
			base_warn(ctx, "freetype finalizing: %s", ft_error_string(fterr));
		fct->ftlib = NULL;
	}
	base_unlock(ctx, base_LOCK_FREETYPE);
}

base_font *
base_new_font_from_file(base_context *ctx, char *path, int index, int use_glyph_bbox)
{
	FT_Face face;
	base_font *font;
	int fterr;

	base_keep_freetype(ctx);

	base_lock(ctx, base_LOCK_FREETYPE);
	fterr = FT_New_Face(ctx->font->ftlib, path, index, &face);
	base_unlock(ctx, base_LOCK_FREETYPE);
	if (fterr)
	{
		base_drop_freetype(ctx);
		base_throw(ctx, "freetype: cannot load font: %s", ft_error_string(fterr));
	}

	font = base_new_font(ctx, face->family_name, use_glyph_bbox, face->num_glyphs);
	font->ft_face = face;
	font->bbox.x0 = (float) face->bbox.xMin / face->units_per_EM;
	font->bbox.y0 = (float) face->bbox.yMin / face->units_per_EM;
	font->bbox.x1 = (float) face->bbox.xMax / face->units_per_EM;
	font->bbox.y1 = (float) face->bbox.yMax / face->units_per_EM;

	return font;
}

base_font *
base_new_font_from_memory(base_context *ctx, unsigned char *data, int len, int index, int use_glyph_bbox)
{
	FT_Face face;
	base_font *font;
	int fterr;

	base_keep_freetype(ctx);

	base_lock(ctx, base_LOCK_FREETYPE);
	fterr = FT_New_Memory_Face(ctx->font->ftlib, data, len, index, &face);
	base_unlock(ctx, base_LOCK_FREETYPE);
	if (fterr)
	{
		base_drop_freetype(ctx);
		base_throw(ctx, "freetype: cannot load font: %s", ft_error_string(fterr));
	}

	font = base_new_font(ctx, face->family_name, use_glyph_bbox, face->num_glyphs);
	font->ft_face = face;
	font->bbox.x0 = (float) face->bbox.xMin / face->units_per_EM;
	font->bbox.y0 = (float) face->bbox.yMin / face->units_per_EM;
	font->bbox.x1 = (float) face->bbox.xMax / face->units_per_EM;
	font->bbox.y1 = (float) face->bbox.yMax / face->units_per_EM;

	return font;
}

static base_matrix
base_adjust_ft_glyph_width(base_context *ctx, base_font *font, int gid, base_matrix trm)
{
	
	if (font->ft_substitute && font->width_table && gid < font->width_count)
	{
		FT_Error fterr;
		int subw;
		int realw;
		float scale;

		base_lock(ctx, base_LOCK_FREETYPE);
		
		fterr = FT_Set_Char_Size(font->ft_face, 1000, 1000, 72, 72);
		if (fterr)
			base_warn(ctx, "freetype setting character size: %s", ft_error_string(fterr));

		fterr = FT_Load_Glyph(font->ft_face, gid,
			FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_TRANSFORM);
		if (fterr)
			base_warn(ctx, "freetype failed to load glyph: %s", ft_error_string(fterr));

		realw = ((FT_Face)font->ft_face)->glyph->metrics.horiAdvance;
		base_unlock(ctx, base_LOCK_FREETYPE);
		subw = font->width_table[gid];
		if (realw)
			scale = (float) subw / realw;
		else
			scale = 1;

		return base_concat(base_scale(scale, 1), trm);
	}

	return trm;
}

static base_pixmap *
base_copy_ft_bitmap(base_context *ctx, int left, int top, FT_Bitmap *bitmap)
{
	base_pixmap *pixmap;
	int y;

	pixmap = base_new_pixmap(ctx, NULL, bitmap->width, bitmap->rows);
	pixmap->x = left;
	pixmap->y = top - bitmap->rows;

	if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO)
	{
		for (y = 0; y < pixmap->h; y++)
		{
			unsigned char *out = pixmap->samples + y * pixmap->w;
			unsigned char *in = bitmap->buffer + (pixmap->h - y - 1) * bitmap->pitch;
			unsigned char bit = 0x80;
			int w = pixmap->w;
			while (w--)
			{
				*out++ = (*in & bit) ? 255 : 0;
				bit >>= 1;
				if (bit == 0)
				{
					bit = 0x80;
					in++;
				}
			}
		}
	}
	else
	{
		for (y = 0; y < pixmap->h; y++)
		{
			memcpy(pixmap->samples + y * pixmap->w,
				bitmap->buffer + (pixmap->h - y - 1) * bitmap->pitch,
				pixmap->w);
		}
	}

	return pixmap;
}

base_pixmap *
base_render_ft_glyph(base_context *ctx, base_font *font, int gid, base_matrix trm, int aa)
{
	FT_Face face = font->ft_face;
	FT_Matrix m;
	FT_Vector v;
	FT_Error fterr;
	base_pixmap *result;

	trm = base_adjust_ft_glyph_width(ctx, font, gid, trm);

	if (font->ft_italic)
		trm = base_concat(base_shear(0.3f, 0), trm);

	

	m.xx = trm.a * 64; 
	m.yx = trm.b * 64;
	m.xy = trm.c * 64;
	m.yy = trm.d * 64;
	v.x = trm.e * 64;
	v.y = trm.f * 64;

	base_lock(ctx, base_LOCK_FREETYPE);
	fterr = FT_Set_Char_Size(face, 65536, 65536, 72, 72); 
	if (fterr)
		base_warn(ctx, "freetype setting character size: %s", ft_error_string(fterr));
	FT_Set_Transform(face, &m, &v);

	if (aa == 0)
	{
		
		float scale = base_matrix_expansion(trm);
		m.xx = trm.a * 65536 / scale;
		m.xy = trm.b * 65536 / scale;
		m.yx = trm.c * 65536 / scale;
		m.yy = trm.d * 65536 / scale;
		v.x = 0;
		v.y = 0;

		fterr = FT_Set_Char_Size(face, 64 * scale, 64 * scale, 72, 72);
		if (fterr)
			base_warn(ctx, "freetype setting character size: %s", ft_error_string(fterr));
		FT_Set_Transform(face, &m, &v);
		fterr = FT_Load_Glyph(face, gid, FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_MONO);
		if (fterr) {
			base_warn(ctx, "freetype load hinted glyph (gid %d): %s", gid, ft_error_string(fterr));
			goto retry_unhinted;
		}
	}
	else if (font->ft_hint)
	{
		
		fterr = FT_Load_Glyph(face, gid, FT_LOAD_NO_BITMAP);
		if (fterr) {
			base_warn(ctx, "freetype load hinted glyph (gid %d): %s", gid, ft_error_string(fterr));
			goto retry_unhinted;
		}
	}
	else
	{
retry_unhinted:
		fterr = FT_Load_Glyph(face, gid, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING);
		if (fterr)
		{
			base_warn(ctx, "freetype load glyph (gid %d): %s", gid, ft_error_string(fterr));
			base_unlock(ctx, base_LOCK_FREETYPE);
			return NULL;
		}
	}

	if (font->ft_bold)
	{
		float strength = base_matrix_expansion(trm) * 0.04f;
		FT_Outline_Embolden(&face->glyph->outline, strength * 64);
		FT_Outline_Translate(&face->glyph->outline, -strength * 32, -strength * 32);
	}

	fterr = FT_Render_Glyph(face->glyph, base_aa_level(ctx) > 0 ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO);
	if (fterr)
	{
		base_warn(ctx, "freetype render glyph (gid %d): %s", gid, ft_error_string(fterr));
		base_unlock(ctx, base_LOCK_FREETYPE);
		return NULL;
	}

	result = base_copy_ft_bitmap(ctx, face->glyph->bitmap_left, face->glyph->bitmap_top, &face->glyph->bitmap);
	base_unlock(ctx, base_LOCK_FREETYPE);
	return result;
}

base_pixmap *
base_render_ft_stroked_glyph(base_context *ctx, base_font *font, int gid, base_matrix trm, base_matrix ctm, base_stroke_state *state)
{
	FT_Face face = font->ft_face;
	float expansion = base_matrix_expansion(ctm);
	int linewidth = state->linewidth * expansion * 64 / 2;
	FT_Matrix m;
	FT_Vector v;
	FT_Error fterr;
	FT_Stroker stroker;
	FT_Glyph glyph;
	FT_BitmapGlyph bitmap;
	base_pixmap *pixmap;
	FT_Stroker_LineJoin line_join;

	trm = base_adjust_ft_glyph_width(ctx, font, gid, trm);

	if (font->ft_italic)
		trm = base_concat(base_shear(0.3f, 0), trm);

	m.xx = trm.a * 64; 
	m.yx = trm.b * 64;
	m.xy = trm.c * 64;
	m.yy = trm.d * 64;
	v.x = trm.e * 64;
	v.y = trm.f * 64;

	base_lock(ctx, base_LOCK_FREETYPE);
	fterr = FT_Set_Char_Size(face, 65536, 65536, 72, 72); 
	if (fterr)
	{
		base_warn(ctx, "FT_Set_Char_Size: %s", ft_error_string(fterr));
		base_unlock(ctx, base_LOCK_FREETYPE);
		return NULL;
	}

	FT_Set_Transform(face, &m, &v);

	fterr = FT_Load_Glyph(face, gid, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING);
	if (fterr)
	{
		base_warn(ctx, "FT_Load_Glyph(gid %d): %s", gid, ft_error_string(fterr));
		base_unlock(ctx, base_LOCK_FREETYPE);
		return NULL;
	}

	fterr = FT_Stroker_New(ctx->font->ftlib, &stroker);
	if (fterr)
	{
		base_warn(ctx, "FT_Stroker_New: %s", ft_error_string(fterr));
		base_unlock(ctx, base_LOCK_FREETYPE);
		return NULL;
	}

#if FREETYPE_MAJOR * 10000 + FREETYPE_MINOR * 100 + FREETYPE_PATCH > 20405
	
	line_join =
		state->linejoin == base_LINEJOIN_MITER ? FT_STROKER_LINEJOIN_MITER_FIXED :
		state->linejoin == base_LINEJOIN_ROUND ? FT_STROKER_LINEJOIN_ROUND :
		state->linejoin == base_LINEJOIN_BEVEL ? FT_STROKER_LINEJOIN_BEVEL :
		FT_STROKER_LINEJOIN_MITER_VARIABLE;
#else
	
	line_join =
		state->linejoin == base_LINEJOIN_MITER ? FT_STROKER_LINEJOIN_MITER :
		state->linejoin == base_LINEJOIN_ROUND ? FT_STROKER_LINEJOIN_ROUND :
		state->linejoin == base_LINEJOIN_BEVEL ? FT_STROKER_LINEJOIN_BEVEL :
		FT_STROKER_LINEJOIN_MITER;
#endif
	FT_Stroker_Set(stroker, linewidth, state->start_cap, line_join, state->miterlimit * 65536);

	fterr = FT_Get_Glyph(face->glyph, &glyph);
	if (fterr)
	{
		base_warn(ctx, "FT_Get_Glyph: %s", ft_error_string(fterr));
		FT_Stroker_Done(stroker);
		base_unlock(ctx, base_LOCK_FREETYPE);
		return NULL;
	}

	fterr = FT_Glyph_Stroke(&glyph, stroker, 1);
	if (fterr)
	{
		base_warn(ctx, "FT_Glyph_Stroke: %s", ft_error_string(fterr));
		FT_Done_Glyph(glyph);
		FT_Stroker_Done(stroker);
		base_unlock(ctx, base_LOCK_FREETYPE);
		return NULL;
	}

	FT_Stroker_Done(stroker);

	fterr = FT_Glyph_To_Bitmap(&glyph, base_aa_level(ctx) > 0 ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO, 0, 1);
	if (fterr)
	{
		base_warn(ctx, "FT_Glyph_To_Bitmap: %s", ft_error_string(fterr));
		FT_Done_Glyph(glyph);
		base_unlock(ctx, base_LOCK_FREETYPE);
		return NULL;
	}

	bitmap = (FT_BitmapGlyph)glyph;
	pixmap = base_copy_ft_bitmap(ctx, bitmap->left, bitmap->top, &bitmap->bitmap);
	FT_Done_Glyph(glyph);
	base_unlock(ctx, base_LOCK_FREETYPE);

	return pixmap;
}

static base_rect
base_bound_ft_glyph(base_context *ctx, base_font *font, int gid, base_matrix trm)
{
	FT_Face face = font->ft_face;
	FT_Error fterr;
	FT_BBox cbox;
	FT_Matrix m;
	FT_Vector v;
	base_rect bounds;

	
	

	trm = base_adjust_ft_glyph_width(ctx, font, gid, trm);

	if (font->ft_italic)
		trm = base_concat(base_shear(0.3f, 0), trm);

	m.xx = trm.a * 64; 
	m.yx = trm.b * 64;
	m.xy = trm.c * 64;
	m.yy = trm.d * 64;
	v.x = trm.e * 64;
	v.y = trm.f * 64;

	base_lock(ctx, base_LOCK_FREETYPE);
	fterr = FT_Set_Char_Size(face, 65536, 65536, 72, 72); 
	if (fterr)
		base_warn(ctx, "freetype setting character size: %s", ft_error_string(fterr));
	FT_Set_Transform(face, &m, &v);

	fterr = FT_Load_Glyph(face, gid, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING);
	if (fterr)
	{
		base_warn(ctx, "freetype load glyph (gid %d): %s", gid, ft_error_string(fterr));
		base_unlock(ctx, base_LOCK_FREETYPE);
		bounds.x0 = bounds.x1 = trm.e;
		bounds.y0 = bounds.y1 = trm.f;
		return bounds;
	}

	if (font->ft_bold)
	{
		float strength = base_matrix_expansion(trm) * 0.04f;
		FT_Outline_Embolden(&face->glyph->outline, strength * 64);
		FT_Outline_Translate(&face->glyph->outline, -strength * 32, -strength * 32);
	}

	FT_Outline_Get_CBox(&face->glyph->outline, &cbox);
	base_unlock(ctx, base_LOCK_FREETYPE);
	bounds.x0 = cbox.xMin / 64.0f;
	bounds.y0 = cbox.yMin / 64.0f;
	bounds.x1 = cbox.xMax / 64.0f;
	bounds.y1 = cbox.yMax / 64.0f;

	if (base_is_empty_rect(bounds))
	{
		bounds.x0 = bounds.x1 = trm.e;
		bounds.y0 = bounds.y1 = trm.f;
	}

	return bounds;
}

base_font *
base_new_type3_font(base_context *ctx, char *name, base_matrix matrix)
{
	base_font *font;
	int i;

	font = base_new_font(ctx, name, 1, 256);
	font->t3procs = base_malloc_array(ctx, 256, sizeof(base_buffer*));
	font->t3widths = base_malloc_array(ctx, 256, sizeof(float));
	font->t3flags = base_malloc_array(ctx, 256, sizeof(char));

	font->t3matrix = matrix;
	for (i = 0; i < 256; i++)
	{
		font->t3procs[i] = NULL;
		font->t3widths[i] = 0;
		font->t3flags[i] = 0;
	}

	return font;
}

static base_rect
base_bound_t3_glyph(base_context *ctx, base_font *font, int gid, base_matrix trm)
{
	base_matrix ctm;
	base_buffer *contents;
	base_rect bounds;
	base_bbox bbox;
	base_device *dev;

	contents = font->t3procs[gid];
	if (!contents)
		return base_transform_rect(trm, base_empty_rect);

	ctm = base_concat(font->t3matrix, trm);
	dev = base_new_bbox_device(ctx, &bbox);
	dev->flags = base_DEVFLAG_FILLCOLOR_UNDEFINED |
			base_DEVFLAG_STROKECOLOR_UNDEFINED |
			base_DEVFLAG_STARTCAP_UNDEFINED |
			base_DEVFLAG_DASHCAP_UNDEFINED |
			base_DEVFLAG_ENDCAP_UNDEFINED |
			base_DEVFLAG_LINEJOIN_UNDEFINED |
			base_DEVFLAG_MITERLIMIT_UNDEFINED |
			base_DEVFLAG_LINEWIDTH_UNDEFINED;
	font->t3run(font->t3doc, font->t3resources, contents, dev, ctm, NULL);
	font->t3flags[gid] = dev->flags;
	base_free_device(dev);

	bounds.x0 = bbox.x0;
	bounds.y0 = bbox.y0;
	bounds.x1 = bbox.x1;
	bounds.y1 = bbox.y1;
	return bounds;
}

base_pixmap *
base_render_t3_glyph(base_context *ctx, base_font *font, int gid, base_matrix trm, base_colorspace *model)
{
	base_matrix ctm;
	base_buffer *contents;
	base_bbox bbox;
	base_device *dev;
	base_pixmap *glyph;
	base_pixmap *result;

	if (gid < 0 || gid > 255)
		return NULL;

	contents = font->t3procs[gid];
	if (!contents)
		return NULL;

	if (font->t3flags[gid] & base_DEVFLAG_MASK)
	{
		if (font->t3flags[gid] & base_DEVFLAG_COLOR)
			base_warn(ctx, "type3 glyph claims to be both masked and colored");
		model = NULL;
	}
	else if (font->t3flags[gid] & base_DEVFLAG_COLOR)
	{
		if (!model)
			base_warn(ctx, "colored type3 glyph wanted in masked context");
	}
	else
	{
		base_warn(ctx, "type3 glyph doesn't specify masked or colored");
		model = NULL; 
	}

	bbox = base_bbox_covering_rect(base_bound_glyph(ctx, font, gid, trm));
	bbox.x0--;
	bbox.y0--;
	bbox.x1++;
	bbox.y1++;

	glyph = base_new_pixmap_with_bbox(ctx, model ? model : base_device_gray, bbox);
	base_clear_pixmap(ctx, glyph);

	ctm = base_concat(font->t3matrix, trm);
	dev = base_new_draw_device_type3(ctx, glyph);
	font->t3run(font->t3doc, font->t3resources, contents, dev, ctm, NULL);
	
	base_free_device(dev);

	if (!model)
	{
		result = base_alpha_from_gray(ctx, glyph, 0);
		base_drop_pixmap(ctx, glyph);
	}
	else
		result = glyph;

	return result;
}

void
base_render_t3_glyph_direct(base_context *ctx, base_device *dev, base_font *font, int gid, base_matrix trm, void *gstate)
{
	base_matrix ctm;
	base_buffer *contents;

	if (gid < 0 || gid > 255)
		return;

	contents = font->t3procs[gid];
	if (!contents)
		return;

	if (font->t3flags[gid] & base_DEVFLAG_MASK)
	{
		if (font->t3flags[gid] & base_DEVFLAG_COLOR)
			base_warn(ctx, "type3 glyph claims to be both masked and colored");
	}
	else if (font->t3flags[gid] & base_DEVFLAG_COLOR)
	{
	}
	else
	{
		base_warn(ctx, "type3 glyph doesn't specify masked or colored");
	}

	ctm = base_concat(font->t3matrix, trm);
	font->t3run(font->t3doc, font->t3resources, contents, dev, ctm, gstate);
	
}

void
base_print_font(base_context *ctx, FILE *out, base_font *font)
{
	fprintf(out, "font '%s' {\n", font->name);

	if (font->ft_face)
	{
		fprintf(out, "\tfreetype face %p\n", font->ft_face);
		if (font->ft_substitute)
			fprintf(out, "\tsubstitute font\n");
	}

	if (font->t3procs)
	{
		fprintf(out, "\ttype3 matrix [%g %g %g %g]\n",
			font->t3matrix.a, font->t3matrix.b,
			font->t3matrix.c, font->t3matrix.d);

		fprintf(out, "\ttype3 bbox [%g %g %g %g]\n",
			font->bbox.x0, font->bbox.y0,
			font->bbox.x1, font->bbox.y1);
	}

	fprintf(out, "}\n");
}

base_rect
base_bound_glyph(base_context *ctx, base_font *font, int gid, base_matrix trm)
{
	if (font->bbox_table && gid < font->bbox_count)
	{
		if (base_is_infinite_rect(font->bbox_table[gid]))
		{
			if (font->ft_face)
				font->bbox_table[gid] = base_bound_ft_glyph(ctx, font, gid, base_identity);
			else if (font->t3procs)
				font->bbox_table[gid] = base_bound_t3_glyph(ctx, font, gid, base_identity);
			else
				font->bbox_table[gid] = base_empty_rect;
		}
		return base_transform_rect(trm, font->bbox_table[gid]);
	}

	
	return base_transform_rect(trm, font->bbox);
}

int base_glyph_cacheable(base_context *ctx, base_font *font, int gid)
{
	if (!font->t3procs || !font->t3flags || gid < 0 || gid >= font->bbox_count)
		return 1;
	return (font->t3flags[gid] & base_DEVFLAG_UNCACHEABLE) == 0;
}
