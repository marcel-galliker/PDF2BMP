#include "pdf-internal.h"

#define QUANT(x,a) (((int)((x) * (a))) / (a))
#define HSUBPIX 5.0
#define VSUBPIX 5.0

#define STACK_SIZE 96

#define ATTEMPT_KNOCKOUT_AND_ISOLATED

#undef DUMP_GROUP_BLENDS

typedef struct base_draw_device_s base_draw_device;

enum {
	base_DRAWDEV_FLAGS_TYPE3 = 1,
};

typedef struct base_draw_state_s base_draw_state;

struct base_draw_state_s {
	base_bbox scissor;
	base_pixmap *dest;
	base_pixmap *mask;
	base_pixmap *shape;
	int blendmode;
	int luminosity;
	float alpha;
	base_matrix ctm;
	float xstep, ystep;
	base_rect area;
};

struct base_draw_device_s
{
	base_gel *gel;
	base_context *ctx;
	int flags;
	int top;
	base_draw_state *stack;
	int stack_max;
	base_draw_state init_stack[STACK_SIZE];
};

#ifdef DUMP_GROUP_BLENDS
static int group_dump_count = 0;

static void base_dump_blend(base_context *ctx, base_pixmap *pix, const char *s)
{
	char name[80];

	if (!pix)
		return;

	sprintf(name, "dump%02d.png", group_dump_count);
	if (s)
		printf("%s%02d", s, group_dump_count);
	group_dump_count++;

	base_write_png(ctx, pix, name, (pix->n > 1));
}

static void dump_spaces(int x, const char *s)
{
	int i;
	for (i = 0; i < x; i++)
		printf(" ");
	printf("%s", s);
}

#endif

static void base_grow_stack(base_draw_device *dev)
{
	int max = dev->stack_max * 2;
	base_draw_state *stack;

	if (dev->stack == &dev->init_stack[0])
	{
		stack = base_malloc(dev->ctx, sizeof(*stack) * max);
		memcpy(stack, dev->stack, sizeof(*stack) * dev->stack_max);
	}
	else
	{
		stack = base_resize_array(dev->ctx, dev->stack, max, sizeof(*stack));
	}
	dev->stack = stack;
	dev->stack_max = max;
}

static base_draw_state *
push_stack(base_draw_device *dev)
{
	base_draw_state *state;

	if (dev->top == dev->stack_max-1)
		base_grow_stack(dev);
	state = &dev->stack[dev->top];
	dev->top++;
	memcpy(&state[1], state, sizeof(*state));
	return state;
}

static base_draw_state *
base_knockout_begin(base_draw_device *dev)
{
	base_context *ctx = dev->ctx;
	base_bbox bbox;
	base_pixmap *dest, *shape;
	base_draw_state *state = &dev->stack[dev->top];
	int isolated = state->blendmode & base_BLEND_ISOLATED;

	if ((state->blendmode & base_BLEND_KNOCKOUT) == 0)
		return state;

	state = push_stack(dev);

	bbox = base_pixmap_bbox(dev->ctx, state->dest);
	bbox = base_intersect_bbox(bbox, state->scissor);
	dest = base_new_pixmap_with_bbox(dev->ctx, state->dest->colorspace, bbox);

	if (isolated)
	{
		base_clear_pixmap(ctx, dest);
	}
	else
	{
		
		int i = dev->top-1; 
		base_pixmap *prev = state->dest;
		while (i > 0)
		{
			prev = dev->stack[--i].dest;
			if (prev != state->dest)
				break;
		}
		if (prev)
			base_copy_pixmap_rect(ctx, dest, prev, bbox);
		else
			base_clear_pixmap(ctx, dest);
	}

	if (state->blendmode == 0 && isolated)
	{
		
		shape = state->shape;
	}
	else
	{
		shape = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
		base_clear_pixmap(dev->ctx, shape);
	}
#ifdef DUMP_GROUP_BLENDS
	dump_spaces(dev->top-1, "Knockout begin\n");
#endif
	state[1].scissor = bbox;
	state[1].dest = dest;
	state[1].shape = shape;
	state[1].blendmode &= ~base_BLEND_MODEMASK;

	return &state[1];
}

static void base_knockout_end(base_draw_device *dev)
{
	base_draw_state *state;
	int blendmode;
	int isolated;
	base_context *ctx = dev->ctx;

	if (dev->top == 0)
	{
		base_warn(ctx, "unexpected knockout end");
		return;
	}
	state = &dev->stack[--dev->top];
	if ((state[0].blendmode & base_BLEND_KNOCKOUT) == 0)
		return;

	blendmode = state->blendmode & base_BLEND_MODEMASK;
	isolated = state->blendmode & base_BLEND_ISOLATED;

#ifdef DUMP_GROUP_BLENDS
	dump_spaces(dev->top, "");
	base_dump_blend(dev->ctx, state[1].dest, "Knockout end: blending ");
	if (state[1].shape)
		base_dump_blend(dev->ctx, state[1].shape, "/");
	base_dump_blend(dev->ctx, state[0].dest, " onto ");
	if (state[0].shape)
		base_dump_blend(dev->ctx, state[0].shape, "/");
	if (blendmode != 0)
		printf(" (blend %d)", blendmode);
	if (isolated != 0)
		printf(" (isolated)");
	printf(" (knockout)");
#endif
	if ((blendmode == 0) && (state[0].shape == state[1].shape))
		base_paint_pixmap(state[0].dest, state[1].dest, 255);
	else
		base_blend_pixmap(state[0].dest, state[1].dest, 255, blendmode, isolated, state[1].shape);

	base_drop_pixmap(dev->ctx, state[1].dest);
	if (state[0].shape != state[1].shape)
	{
		if (state[0].shape)
			base_paint_pixmap(state[0].shape, state[1].shape, 255);
		base_drop_pixmap(dev->ctx, state[1].shape);
	}
#ifdef DUMP_GROUP_BLENDS
	base_dump_blend(dev->ctx, state[0].dest, " to get ");
	if (state[0].shape)
		base_dump_blend(dev->ctx, state[0].shape, "/");
	printf("\n");
#endif
}

static void
base_draw_fill_path(base_device *devp, base_path *path, int even_odd, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_draw_device *dev = devp->user;
	float expansion = base_matrix_expansion(ctm);
	float flatness = 0.3f / expansion;
	unsigned char colorbv[base_MAX_COLORS + 1];
	float colorfv[base_MAX_COLORS];
	base_bbox bbox;
	int i;
	base_draw_state *state = &dev->stack[dev->top];
	base_colorspace *model = state->dest->colorspace;

	base_reset_gel(dev->gel, state->scissor);
	base_flatten_fill_path(dev->gel, path, ctm, flatness);
	base_sort_gel(dev->gel);

	bbox = base_bound_gel(dev->gel);
	bbox = base_intersect_bbox(bbox, state->scissor);

	if (base_is_empty_rect(bbox))
		return;

	if (state->blendmode & base_BLEND_KNOCKOUT)
		state = base_knockout_begin(dev);

	base_convert_color(dev->ctx, model, colorfv, colorspace, color);
	for (i = 0; i < model->n; i++)
		colorbv[i] = colorfv[i] * 255;
	colorbv[i] = alpha * 255;

	base_scan_convert(dev->gel, even_odd, bbox, state->dest, colorbv);
	if (state->shape)
	{
		base_reset_gel(dev->gel, state->scissor);
		base_flatten_fill_path(dev->gel, path, ctm, flatness);
		base_sort_gel(dev->gel);

		colorbv[0] = alpha * 255;
		base_scan_convert(dev->gel, even_odd, bbox, state->shape, colorbv);
	}

	if (state->blendmode & base_BLEND_KNOCKOUT)
		base_knockout_end(dev);
}

static void
base_draw_stroke_path(base_device *devp, base_path *path, base_stroke_state *stroke, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_draw_device *dev = devp->user;
	float expansion = base_matrix_expansion(ctm);
	float flatness = 0.3f / expansion;
	float linewidth = stroke->linewidth;
	unsigned char colorbv[base_MAX_COLORS + 1];
	float colorfv[base_MAX_COLORS];
	base_bbox bbox;
	int i;
	base_draw_state *state = &dev->stack[dev->top];
	base_colorspace *model = state->dest->colorspace;

	if (linewidth * expansion < 0.1f)
		linewidth = 1 / expansion;

	base_reset_gel(dev->gel, state->scissor);
	if (stroke->dash_len > 0)
		base_flatten_dash_path(dev->gel, path, stroke, ctm, flatness, linewidth);
	else
		base_flatten_stroke_path(dev->gel, path, stroke, ctm, flatness, linewidth);
	base_sort_gel(dev->gel);

	bbox = base_bound_gel(dev->gel);
	bbox = base_intersect_bbox(bbox, state->scissor);

	if (base_is_empty_rect(bbox))
		return;

	if (state->blendmode & base_BLEND_KNOCKOUT)
		state = base_knockout_begin(dev);

	base_convert_color(dev->ctx, model, colorfv, colorspace, color);
	for (i = 0; i < model->n; i++)
		colorbv[i] = colorfv[i] * 255;
	colorbv[i] = alpha * 255;

	base_scan_convert(dev->gel, 0, bbox, state->dest, colorbv);
	if (state->shape)
	{
		base_reset_gel(dev->gel, state->scissor);
		if (stroke->dash_len > 0)
			base_flatten_dash_path(dev->gel, path, stroke, ctm, flatness, linewidth);
		else
			base_flatten_stroke_path(dev->gel, path, stroke, ctm, flatness, linewidth);
		base_sort_gel(dev->gel);

		colorbv[0] = 255;
		base_scan_convert(dev->gel, 0, bbox, state->shape, colorbv);
	}

	if (state->blendmode & base_BLEND_KNOCKOUT)
		base_knockout_end(dev);
}

static void
base_draw_clip_path(base_device *devp, base_path *path, base_rect *rect, int even_odd, base_matrix ctm)
{
	base_draw_device *dev = devp->user;
	float expansion = base_matrix_expansion(ctm);
	float flatness = 0.3f / expansion;
	base_bbox bbox;
	base_draw_state *state = push_stack(dev);
	base_colorspace *model = state->dest->colorspace;

	base_reset_gel(dev->gel, state->scissor);
	base_flatten_fill_path(dev->gel, path, ctm, flatness);
	base_sort_gel(dev->gel);

	bbox = base_bound_gel(dev->gel);
	bbox = base_intersect_bbox(bbox, state->scissor);
	if (rect)
		bbox = base_intersect_bbox(bbox, base_bbox_covering_rect(*rect));

	if (base_is_empty_rect(bbox) || base_is_rect_gel(dev->gel))
	{
		state[1].scissor = bbox;
		state[1].mask = NULL;
#ifdef DUMP_GROUP_BLENDS
		dump_spaces(dev->top-1, "Clip (rectangular) begin\n");
#endif
		return;
	}

	state[1].mask = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
	base_clear_pixmap(dev->ctx, state[1].mask);
	state[1].dest = base_new_pixmap_with_bbox(dev->ctx, model, bbox);
	base_clear_pixmap(dev->ctx, state[1].dest);
	if (state[1].shape)
	{
		state[1].shape = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
		base_clear_pixmap(dev->ctx, state[1].shape);
	}

	base_scan_convert(dev->gel, even_odd, bbox, state[1].mask, NULL);

	state[1].blendmode |= base_BLEND_ISOLATED;
	state[1].scissor = bbox;
#ifdef DUMP_GROUP_BLENDS
	dump_spaces(dev->top-1, "Clip (non-rectangular) begin\n");
#endif
}

static void
base_draw_clip_stroke_path(base_device *devp, base_path *path, base_rect *rect, base_stroke_state *stroke, base_matrix ctm)
{
	base_draw_device *dev = devp->user;
	float expansion = base_matrix_expansion(ctm);
	float flatness = 0.3f / expansion;
	float linewidth = stroke->linewidth;
	base_bbox bbox;
	base_draw_state *state = push_stack(dev);
	base_colorspace *model = state->dest->colorspace;

	if (linewidth * expansion < 0.1f)
		linewidth = 1 / expansion;

	base_reset_gel(dev->gel, state->scissor);
	if (stroke->dash_len > 0)
		base_flatten_dash_path(dev->gel, path, stroke, ctm, flatness, linewidth);
	else
		base_flatten_stroke_path(dev->gel, path, stroke, ctm, flatness, linewidth);
	base_sort_gel(dev->gel);

	bbox = base_bound_gel(dev->gel);
	bbox = base_intersect_bbox(bbox, state->scissor);
	if (rect)
		bbox = base_intersect_bbox(bbox, base_bbox_covering_rect(*rect));

	state[1].mask = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
	base_clear_pixmap(dev->ctx, state[1].mask);
	state[1].dest = base_new_pixmap_with_bbox(dev->ctx, model, bbox);
	base_clear_pixmap(dev->ctx, state[1].dest);
	if (state->shape)
	{
		state[1].shape = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
		base_clear_pixmap(dev->ctx, state[1].shape);
	}

	if (!base_is_empty_rect(bbox))
		base_scan_convert(dev->gel, 0, bbox, state[1].mask, NULL);

	state[1].blendmode |= base_BLEND_ISOLATED;
	state[1].scissor = bbox;
#ifdef DUMP_GROUP_BLENDS
	dump_spaces(dev->top-1, "Clip (stroke) begin\n");
#endif
}

static void
draw_glyph(unsigned char *colorbv, base_pixmap *dst, base_pixmap *msk,
	int xorig, int yorig, base_bbox scissor)
{
	unsigned char *dp, *mp;
	base_bbox bbox;
	int x, y, w, h;

	bbox = base_pixmap_bbox_no_ctx(msk);
	bbox.x0 += xorig;
	bbox.y0 += yorig;
	bbox.x1 += xorig;
	bbox.y1 += yorig;

	bbox = base_intersect_bbox(bbox, scissor); 
	x = bbox.x0;
	y = bbox.y0;
	w = bbox.x1 - bbox.x0;
	h = bbox.y1 - bbox.y0;

	mp = msk->samples + ((y - msk->y - yorig) * msk->w + (x - msk->x - xorig));
	dp = dst->samples + ((y - dst->y) * dst->w + (x - dst->x)) * dst->n;

	assert(msk->n == 1);

	while (h--)
	{
		if (dst->colorspace)
			base_paint_span_with_color(dp, mp, dst->n, w, colorbv);
		else
			base_paint_span(dp, mp, 1, w, 255);
		dp += dst->w * dst->n;
		mp += msk->w;
	}
}

static void
my_base_draw_fill_text(base_device *devp, base_text *text, base_matrix ctm,
					 base_colorspace *colorspace, float *color, float alpha, base_bbox org_bbox, base_bbox scissor, double dAngle)
{
	base_draw_device *dev = devp->user;
	unsigned char colorbv[base_MAX_COLORS + 1];
	unsigned char shapebv;
	float colorfv[base_MAX_COLORS];
	base_matrix tm, trm;
	base_pixmap *glyph = NULL;
	int i, x, y, gid;
	base_matrix rotCtm;
	base_matrix myctm;
	int *wid_list;
	int nStartX, nStartY;
	int nStrWid, nStrHei;
	double nFract;
	base_draw_state *state = &dev->stack[dev->top];
	base_colorspace *model = state->dest->colorspace;

	if (state->blendmode & base_BLEND_KNOCKOUT)
		state = base_knockout_begin(dev);

	base_convert_color(dev->ctx, model, colorfv, colorspace, color);
	for (i = 0; i < model->n; i++)
		colorbv[i] = colorfv[i] * 255;
	colorbv[i] = alpha * 255;
	shapebv = 255;

	tm = text->trm;

	nStrWid = 0;
	nStrHei = 0;
	nStartX = 0;

	wid_list = malloc(sizeof(int) * text->len);
	memset(wid_list, 0x0, sizeof(int) * text->len);

	for (i = 0; i < text->len; i++)
	{
		gid = text->items[i].gid;
		if (gid < 0)
			continue;

		if ( i == 0 )
			tm.e = nStartX;
		else
			tm.e = x + glyph->w;

		tm.f = text->items[0].y;
		trm = base_concat(tm, ctm);
		x = floorf(trm.e);
		y = floorf(trm.f);
		trm.e = QUANT(trm.e - floorf(trm.e), HSUBPIX);
		trm.f = QUANT(trm.f - floorf(trm.f), VSUBPIX);

		glyph = base_render_glyph(dev->ctx, text->font, gid, trm, model);
		if (glyph)
		{
			if ( gid == 3 )
				wid_list[i] = 20;
			else
				wid_list[i] = glyph->w + 2;
			nStrWid += wid_list[i];
			if ( glyph->h > nStrHei )
				nStrHei = glyph->h;
			base_drop_pixmap(dev->ctx, glyph);
		}
	}

	nStartX = (scissor.x1 - nStrWid) / 2;
	nStartY = (scissor.y1 - nStrHei ) / 2 ;

	for (i = 0; i < text->len; i++)
	{
		if ( i == 0 )
			text->items[i].x = nStartX;
		else
			text->items[i].x = text->items[i - 1].x + wid_list[i - 1];
		text->items[i].y = nStartY;
	}

	free(wid_list);

	if ( org_bbox.x1 < org_bbox.y1)
		nFract = (double)org_bbox.x1 / nStrWid;
	else
		nFract = (double)org_bbox.y1 / nStrWid;
	myctm = base_scale(nFract, nFract);
	scissor.x1 *= nFract;
	scissor.y1 *= nFract;
	rotCtm = base_rotate(dAngle);
	myctm = base_concat(myctm, rotCtm);
	myctm = base_concat(myctm, base_translate((org_bbox.x1 / 2) - (scissor.x1 / 2) * rotCtm.a - (scissor.y1 / 2) * rotCtm.c, (org_bbox.y1 / 2) - (scissor.x1 / 2) * rotCtm.b - (scissor.y1 / 2) * rotCtm.d));
	myctm = base_concat(ctm, myctm);

	for (i = 0; i < text->len; i++)
	{
		gid = text->items[i].gid;
		if (gid < 0)
			continue;

		tm.e = text->items[i].x;
		tm.f = text->items[i].y;
		trm = base_concat(tm, myctm);
		x = floorf(trm.e);
		y = floorf(trm.f);
		trm.e = QUANT(trm.e - floorf(trm.e), HSUBPIX);
		trm.f = QUANT(trm.f - floorf(trm.f), VSUBPIX);

		glyph = base_render_glyph(dev->ctx, text->font, gid, trm, model);
		if (glyph)
		{
			if (glyph->n == 1)
			{
				draw_glyph(colorbv, state->dest, glyph, x, y, state->scissor);
				if (state->shape)
					draw_glyph(&shapebv, state->shape, glyph, x, y, state->scissor);
			}
			else
			{
				base_matrix ctm = {glyph->w, 0.0, 0.0, glyph->h, x + glyph->x, y + glyph->y};
				base_paint_image(state->dest, state->scissor, state->shape, glyph, ctm, alpha * 255);
			}
			base_drop_pixmap(dev->ctx, glyph);
		}
	}

	if (state->blendmode & base_BLEND_KNOCKOUT)
		base_knockout_end(dev);
}

static void
base_draw_fill_text(base_device *devp, base_text *text, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_draw_device *dev = devp->user;
	unsigned char colorbv[base_MAX_COLORS + 1];
	unsigned char shapebv;
	float colorfv[base_MAX_COLORS];
	base_matrix tm, trm;
	base_pixmap *glyph;
	int i, x, y, gid;
	base_draw_state *state = &dev->stack[dev->top];
	base_colorspace *model = state->dest->colorspace;

	if (state->blendmode & base_BLEND_KNOCKOUT)
		state = base_knockout_begin(dev);

	base_convert_color(dev->ctx, model, colorfv, colorspace, color);
	for (i = 0; i < model->n; i++)
		colorbv[i] = colorfv[i] * 255;
	colorbv[i] = alpha * 255;
	shapebv = 255;

	tm = text->trm;

	for (i = 0; i < text->len; i++)
	{
		gid = text->items[i].gid;
		if (gid < 0)
			continue;

		tm.e = text->items[i].x;
		tm.f = text->items[i].y;
		trm = base_concat(tm, ctm);
		x = floorf(trm.e);
		y = floorf(trm.f);
		trm.e = QUANT(trm.e - floorf(trm.e), HSUBPIX);
		trm.f = QUANT(trm.f - floorf(trm.f), VSUBPIX);

		glyph = base_render_glyph(dev->ctx, text->font, gid, trm, model);
		if (glyph)
		{
			if (glyph->n == 1)
			{
				draw_glyph(colorbv, state->dest, glyph, x, y, state->scissor);
				if (state->shape)
					draw_glyph(&shapebv, state->shape, glyph, x, y, state->scissor);
			}
			else
			{
				base_matrix ctm = {glyph->w, 0.0, 0.0, glyph->h, x + glyph->x, y + glyph->y};
				base_paint_image(state->dest, state->scissor, state->shape, glyph, ctm, alpha * 255);
			}
			base_drop_pixmap(dev->ctx, glyph);
		}
	}

	if (state->blendmode & base_BLEND_KNOCKOUT)
		base_knockout_end(dev);
}

static void
base_draw_stroke_text(base_device *devp, base_text *text, base_stroke_state *stroke, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_draw_device *dev = devp->user;
	unsigned char colorbv[base_MAX_COLORS + 1];
	float colorfv[base_MAX_COLORS];
	base_matrix tm, trm;
	base_pixmap *glyph;
	int i, x, y, gid;
	base_draw_state *state = &dev->stack[dev->top];
	base_colorspace *model = state->dest->colorspace;

	if (state->blendmode & base_BLEND_KNOCKOUT)
		state = base_knockout_begin(dev);

	base_convert_color(dev->ctx, model, colorfv, colorspace, color);
	for (i = 0; i < model->n; i++)
		colorbv[i] = colorfv[i] * 255;
	colorbv[i] = alpha * 255;

	tm = text->trm;

	for (i = 0; i < text->len; i++)
	{
		gid = text->items[i].gid;
		if (gid < 0)
			continue;

		tm.e = text->items[i].x;
		tm.f = text->items[i].y;
		trm = base_concat(tm, ctm);
		x = floorf(trm.e);
		y = floorf(trm.f);
		trm.e = QUANT(trm.e - floorf(trm.e), HSUBPIX);
		trm.f = QUANT(trm.f - floorf(trm.f), VSUBPIX);

		glyph = base_render_stroked_glyph(dev->ctx, text->font, gid, trm, ctm, stroke);
		if (glyph)
		{
			draw_glyph(colorbv, state->dest, glyph, x, y, state->scissor);
			if (state->shape)
				draw_glyph(colorbv, state->shape, glyph, x, y, state->scissor);
			base_drop_pixmap(dev->ctx, glyph);
		}
	}

	if (state->blendmode & base_BLEND_KNOCKOUT)
		base_knockout_end(dev);
}

static void
base_draw_clip_text(base_device *devp, base_text *text, base_matrix ctm, int accumulate)
{
	base_draw_device *dev = devp->user;
	base_bbox bbox;
	base_pixmap *mask, *dest, *shape;
	base_matrix tm, trm;
	base_pixmap *glyph;
	int i, x, y, gid;
	base_draw_state *state;
	base_colorspace *model;

	
	
	

	state = push_stack(dev);
	model = state->dest->colorspace;

	if (accumulate == 0)
	{
		
		bbox = base_bbox_covering_rect(base_bound_text(dev->ctx, text, ctm));
		bbox = base_intersect_bbox(bbox, state->scissor);
	}
	else
	{
		
		bbox = state->scissor;
	}

	if (accumulate == 0 || accumulate == 1)
	{
		mask = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
		base_clear_pixmap(dev->ctx, mask);
		dest = base_new_pixmap_with_bbox(dev->ctx, model, bbox);
		base_clear_pixmap(dev->ctx, dest);
		if (state->shape)
		{
			shape = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
			base_clear_pixmap(dev->ctx, shape);
		}
		else
			shape = NULL;

		state[1].blendmode |= base_BLEND_ISOLATED;
		state[1].scissor = bbox;
		state[1].dest = dest;
		state[1].mask = mask;
		state[1].shape = shape;
#ifdef DUMP_GROUP_BLENDS
		dump_spaces(dev->top-1, "Clip (text) begin\n");
#endif
	}
	else
	{
		mask = state->mask;
		dev->top--;
	}

	if (!base_is_empty_rect(bbox) && mask)
	{
		tm = text->trm;

		for (i = 0; i < text->len; i++)
		{
			gid = text->items[i].gid;
			if (gid < 0)
				continue;

			tm.e = text->items[i].x;
			tm.f = text->items[i].y;
			trm = base_concat(tm, ctm);
			x = floorf(trm.e);
			y = floorf(trm.f);
			trm.e = QUANT(trm.e - floorf(trm.e), HSUBPIX);
			trm.f = QUANT(trm.f - floorf(trm.f), VSUBPIX);

			glyph = base_render_glyph(dev->ctx, text->font, gid, trm, model);
			if (glyph)
			{
				draw_glyph(NULL, mask, glyph, x, y, bbox);
				if (state[1].shape)
					draw_glyph(NULL, state[1].shape, glyph, x, y, bbox);
				base_drop_pixmap(dev->ctx, glyph);
			}
		}
	}
}

static void
base_draw_clip_stroke_text(base_device *devp, base_text *text, base_stroke_state *stroke, base_matrix ctm)
{
	base_draw_device *dev = devp->user;
	base_bbox bbox;
	base_pixmap *mask, *dest, *shape;
	base_matrix tm, trm;
	base_pixmap *glyph;
	int i, x, y, gid;
	base_draw_state *state = push_stack(dev);
	base_colorspace *model = state->dest->colorspace;

	
	bbox = base_bbox_covering_rect(base_bound_text(dev->ctx, text, ctm));
	bbox = base_intersect_bbox(bbox, state->scissor);

	mask = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
	base_clear_pixmap(dev->ctx, mask);
	dest = base_new_pixmap_with_bbox(dev->ctx, model, bbox);
	base_clear_pixmap(dev->ctx, dest);
	if (state->shape)
	{
		shape = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
		base_clear_pixmap(dev->ctx, shape);
	}
	else
		shape = state->shape;

	state[1].blendmode |= base_BLEND_ISOLATED;
	state[1].scissor = bbox;
	state[1].dest = dest;
	state[1].shape = shape;
	state[1].mask = mask;
#ifdef DUMP_GROUP_BLENDS
	dump_spaces(dev->top-1, "Clip (stroke text) begin\n");
#endif

	if (!base_is_empty_rect(bbox))
	{
		tm = text->trm;

		for (i = 0; i < text->len; i++)
		{
			gid = text->items[i].gid;
			if (gid < 0)
				continue;

			tm.e = text->items[i].x;
			tm.f = text->items[i].y;
			trm = base_concat(tm, ctm);
			x = floorf(trm.e);
			y = floorf(trm.f);
			trm.e = QUANT(trm.e - floorf(trm.e), HSUBPIX);
			trm.f = QUANT(trm.f - floorf(trm.f), VSUBPIX);

			glyph = base_render_stroked_glyph(dev->ctx, text->font, gid, trm, ctm, stroke);
			if (glyph)
			{
				draw_glyph(NULL, mask, glyph, x, y, bbox);
				if (shape)
					draw_glyph(NULL, shape, glyph, x, y, bbox);
				base_drop_pixmap(dev->ctx, glyph);
			}
		}
	}
}

static void
base_draw_ignore_text(base_device *dev, base_text *text, base_matrix ctm)
{
}

static void
base_draw_fill_shade(base_device *devp, base_shade *shade, base_matrix ctm, float alpha)
{
	base_draw_device *dev = devp->user;
	base_rect bounds;
	base_bbox bbox, scissor;
	base_pixmap *dest, *shape;
	float colorfv[base_MAX_COLORS];
	unsigned char colorbv[base_MAX_COLORS + 1];
	base_draw_state *state = &dev->stack[dev->top];
	base_colorspace *model = state->dest->colorspace;

	bounds = base_bound_shade(dev->ctx, shade, ctm);
	scissor = state->scissor;
	bbox = base_intersect_bbox(base_bbox_covering_rect(bounds), scissor);

	if (base_is_empty_rect(bbox))
		return;

	if (!model)
	{
		base_warn(dev->ctx, "cannot render shading directly to an alpha mask");
		return;
	}

	if (state->blendmode & base_BLEND_KNOCKOUT)
		state = base_knockout_begin(dev);

	dest = state->dest;
	shape = state->shape;

	if (alpha < 1)
	{
		dest = base_new_pixmap_with_bbox(dev->ctx, state->dest->colorspace, bbox);
		base_clear_pixmap(dev->ctx, dest);
		if (shape)
		{
			shape = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
			base_clear_pixmap(dev->ctx, shape);
		}
	}

	if (shade->use_background)
	{
		unsigned char *s;
		int x, y, n, i;
		base_convert_color(dev->ctx, model, colorfv, shade->colorspace, shade->background);
		for (i = 0; i < model->n; i++)
			colorbv[i] = colorfv[i] * 255;
		colorbv[i] = 255;

		n = dest->n;
		for (y = scissor.y0; y < scissor.y1; y++)
		{
			s = dest->samples + ((scissor.x0 - dest->x) + (y - dest->y) * dest->w) * dest->n;
			for (x = scissor.x0; x < scissor.x1; x++)
			{
				for (i = 0; i < n; i++)
					*s++ = colorbv[i];
			}
		}
		if (shape)
		{
			for (y = scissor.y0; y < scissor.y1; y++)
			{
				s = shape->samples + (scissor.x0 - shape->x) + (y - shape->y) * shape->w;
				for (x = scissor.x0; x < scissor.x1; x++)
				{
					*s++ = 255;
				}
			}
		}
	}

	base_paint_shade(dev->ctx, shade, ctm, dest, bbox);
	if (shape)
		base_clear_pixmap_rect_with_value(dev->ctx, shape, 255, bbox);

	if (alpha < 1)
	{
		base_paint_pixmap(state->dest, dest, alpha * 255);
		base_drop_pixmap(dev->ctx, dest);
		if (shape)
		{
			base_paint_pixmap(state->shape, shape, alpha * 255);
			base_drop_pixmap(dev->ctx, shape);
		}
	}

	if (state->blendmode & base_BLEND_KNOCKOUT)
		base_knockout_end(dev);
}

static base_pixmap *
base_transform_pixmap(base_context *ctx, base_pixmap *image, base_matrix *ctm, int x, int y, int dx, int dy, int gridfit, base_bbox *clip)
{
	base_pixmap *scaled;

	if (ctm->a != 0 && ctm->b == 0 && ctm->c == 0 && ctm->d != 0)
	{
		
		base_matrix m = *ctm;
		if (gridfit)
			base_gridfit_matrix(&m);
		scaled = base_scale_pixmap(ctx, image, m.e, m.f, m.a, m.d, clip);
		if (!scaled)
			return NULL;
		ctm->a = scaled->w;
		ctm->d = scaled->h;
		ctm->e = scaled->x;
		ctm->f = scaled->y;
		return scaled;
	}

	if (ctm->a == 0 && ctm->b != 0 && ctm->c != 0 && ctm->d == 0)
	{
		
		base_matrix m = *ctm;
		base_bbox rclip;
		if (gridfit)
			base_gridfit_matrix(&m);
		if (clip)
		{
			rclip.x0 = clip->y0;
			rclip.y0 = clip->x0;
			rclip.x1 = clip->y1;
			rclip.y1 = clip->x1;
		}
		scaled = base_scale_pixmap(ctx, image, m.f, m.e, m.b, m.c, (clip ? &rclip : 0));
		if (!scaled)
			return NULL;
		ctm->b = scaled->w;
		ctm->c = scaled->h;
		ctm->f = scaled->x;
		ctm->e = scaled->y;
		return scaled;
	}

	
	if (dx > 0 && dy > 0)
	{
		scaled = base_scale_pixmap(ctx, image, 0, 0, (float)dx, (float)dy, NULL);
		return scaled;
	}

	return NULL;
}

static void
base_draw_fill_image(base_device *devp, base_image *image, base_matrix ctm, float alpha)
{
    base_draw_device *dev = devp->user;
	base_pixmap *converted = NULL;
	base_pixmap *scaled = NULL;
	base_pixmap *pixmap;
	base_pixmap *orig_pixmap;
	int after;
	int dx, dy;
	base_context *ctx = dev->ctx;
	base_draw_state *state = &dev->stack[dev->top];
	base_colorspace *model = state->dest->colorspace;
	base_bbox clip = base_pixmap_bbox(ctx, state->dest);
	

	clip = base_intersect_bbox(clip, state->scissor);

	base_var(scaled);

	if (!model)
	{
		base_warn(dev->ctx, "cannot render image directly to an alpha mask");
		return;
	}

	if (image->w == 0 || image->h == 0)
		return;

	dx = sqrtf(ctm.a * ctm.a + ctm.b * ctm.b);
	dy = sqrtf(ctm.c * ctm.c + ctm.d * ctm.d);

	pixmap = base_image_to_pixmap(ctx, image, dx, dy);

	orig_pixmap = pixmap;

	
	
	

	base_try(ctx)
	{
		if (state->blendmode & base_BLEND_KNOCKOUT)
			state = base_knockout_begin(dev);

		after = 0;
		if (pixmap->colorspace == base_device_gray)
			after = 1;

		if (pixmap->colorspace != model && !after)
		{
			converted = base_new_pixmap_with_bbox(ctx, model, base_pixmap_bbox(ctx, pixmap));
			base_convert_pixmap(ctx, converted, pixmap);
			pixmap = converted;
		}

		if (dx < pixmap->w && dy < pixmap->h)
		{
			int gridfit = alpha == 1.0f && !(dev->flags & base_DRAWDEV_FLAGS_TYPE3);
			scaled = base_transform_pixmap(ctx, pixmap, &ctm, state->dest->x, state->dest->y, dx, dy, gridfit, &clip);
			if (!scaled)
			{
				if (dx < 1)
					dx = 1;
				if (dy < 1)
					dy = 1;
				scaled = base_scale_pixmap(ctx, pixmap, pixmap->x, pixmap->y, dx, dy, NULL);
			}
			if (scaled)
				pixmap = scaled;
		}

		if (pixmap->colorspace != model)
		{
			if ((pixmap->colorspace == base_device_gray && model == base_device_rgb) ||
				(pixmap->colorspace == base_device_gray && model == base_device_bgr))
			{
				
			}
			else
			{
				converted = base_new_pixmap_with_bbox(ctx, model, base_pixmap_bbox(ctx, pixmap));
				base_convert_pixmap(ctx, converted, pixmap);
				pixmap = converted;
			}
		}

		base_paint_image(state->dest, state->scissor, state->shape, pixmap, ctm, alpha * 255);

		if (state->blendmode & base_BLEND_KNOCKOUT)
			base_knockout_end(dev);
	}
	base_always(ctx)
	{
		base_drop_pixmap(ctx, scaled);
		base_drop_pixmap(ctx, converted);
		base_drop_pixmap(ctx, orig_pixmap);
	}
	base_catch(ctx)
	{
		base_rethrow(ctx);
	}
}

static void
base_draw_fill_image_mask(base_device *devp, base_image *image, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_draw_device *dev = devp->user;
	unsigned char colorbv[base_MAX_COLORS + 1];
	float colorfv[base_MAX_COLORS];
	base_pixmap *scaled = NULL;
	base_pixmap *pixmap;
	base_pixmap *orig_pixmap;
	int dx, dy;
	int i;
	base_context *ctx = dev->ctx;
	base_draw_state *state = &dev->stack[dev->top];
	base_colorspace *model = state->dest->colorspace;
	base_bbox clip = base_pixmap_bbox(ctx, state->dest);

	clip = base_intersect_bbox(clip, state->scissor);

	if (image->w == 0 || image->h == 0)
		return;

	dx = sqrtf(ctm.a * ctm.a + ctm.b * ctm.b);
	dy = sqrtf(ctm.c * ctm.c + ctm.d * ctm.d);
	pixmap = base_image_to_pixmap(ctx, image, dx, dy);
	orig_pixmap = pixmap;

	base_try(ctx)
	{
		if (state->blendmode & base_BLEND_KNOCKOUT)
			state = base_knockout_begin(dev);

		if (dx < pixmap->w && dy < pixmap->h)
		{
			int gridfit = alpha == 1.0f && !(dev->flags & base_DRAWDEV_FLAGS_TYPE3);
			scaled = base_transform_pixmap(dev->ctx, pixmap, &ctm, state->dest->x, state->dest->y, dx, dy, gridfit, &clip);
			if (!scaled)
			{
				if (dx < 1)
					dx = 1;
				if (dy < 1)
					dy = 1;
				scaled = base_scale_pixmap(dev->ctx, pixmap, pixmap->x, pixmap->y, dx, dy, NULL);
			}
			if (scaled)
				pixmap = scaled;
		}

		base_convert_color(dev->ctx, model, colorfv, colorspace, color);
		for (i = 0; i < model->n; i++)
			colorbv[i] = colorfv[i] * 255;
		colorbv[i] = alpha * 255;

		base_paint_image_with_color(state->dest, state->scissor, state->shape, pixmap, ctm, colorbv);

		if (scaled)
			base_drop_pixmap(dev->ctx, scaled);

		if (state->blendmode & base_BLEND_KNOCKOUT)
			base_knockout_end(dev);
	}
	base_always(ctx)
	{
		base_drop_pixmap(dev->ctx, orig_pixmap);
	}
	base_catch(ctx)
	{
		base_rethrow(ctx);
	}
}

static void
base_draw_clip_image_mask(base_device *devp, base_image *image, base_rect *rect, base_matrix ctm)
{
	base_draw_device *dev = devp->user;
	base_context *ctx = dev->ctx;
	base_bbox bbox;
	base_pixmap *mask = NULL;
	base_pixmap *dest = NULL;
	base_pixmap *shape = NULL;
	base_pixmap *scaled = NULL;
	base_pixmap *pixmap;
	base_pixmap *orig_pixmap;
	int dx, dy;
	base_draw_state *state = push_stack(dev);
	base_colorspace *model = state->dest->colorspace;
	base_bbox clip = base_pixmap_bbox(ctx, state->dest);

	clip = base_intersect_bbox(clip, state->scissor);

	base_var(mask);
	base_var(dest);
	base_var(shape);

	if (image->w == 0 || image->h == 0)
	{
#ifdef DUMP_GROUP_BLENDS
		dump_spaces(dev->top-1, "Clip (image mask) (empty) begin\n");
#endif
		state[1].scissor = base_empty_bbox;
		state[1].mask = NULL;
		return;
	}

#ifdef DUMP_GROUP_BLENDS
	dump_spaces(dev->top-1, "Clip (image mask) begin\n");
#endif

	bbox = base_bbox_covering_rect(base_transform_rect(ctm, base_unit_rect));
	bbox = base_intersect_bbox(bbox, state->scissor);
	if (rect)
		bbox = base_intersect_bbox(bbox, base_bbox_covering_rect(*rect));

	dx = sqrtf(ctm.a * ctm.a + ctm.b * ctm.b);
	dy = sqrtf(ctm.c * ctm.c + ctm.d * ctm.d);
	pixmap = base_image_to_pixmap(ctx, image, dx, dy);
	orig_pixmap = pixmap;

	base_try(ctx)
	{
		mask = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
		base_clear_pixmap(dev->ctx, mask);

		dest = base_new_pixmap_with_bbox(dev->ctx, model, bbox);
		base_clear_pixmap(dev->ctx, dest);
		if (state->shape)
		{
			shape = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
			base_clear_pixmap(dev->ctx, shape);
		}

		if (dx < pixmap->w && dy < pixmap->h)
		{
			int gridfit = !(dev->flags & base_DRAWDEV_FLAGS_TYPE3);
			scaled = base_transform_pixmap(dev->ctx, pixmap, &ctm, state->dest->x, state->dest->y, dx, dy, gridfit, &clip);
			if (!scaled)
			{
				if (dx < 1)
					dx = 1;
				if (dy < 1)
					dy = 1;
				scaled = base_scale_pixmap(dev->ctx, pixmap, pixmap->x, pixmap->y, dx, dy, NULL);
			}
			if (scaled)
				pixmap = scaled;
		}
		base_paint_image(mask, bbox, state->shape, pixmap, ctm, 255);

	}
	base_always(ctx)
	{
		base_drop_pixmap(ctx, scaled);
		base_drop_pixmap(ctx, orig_pixmap);
	}
	base_catch(ctx)
	{
		base_drop_pixmap(ctx, shape);
		base_drop_pixmap(ctx, dest);
		base_drop_pixmap(ctx, mask);
		base_rethrow(ctx);
	}

	state[1].blendmode |= base_BLEND_ISOLATED;
	state[1].scissor = bbox;
	state[1].dest = dest;
	state[1].shape = shape;
	state[1].mask = mask;
}

static void
base_draw_pop_clip(base_device *devp)
{
	base_draw_device *dev = devp->user;
	base_context *ctx = dev->ctx;
	base_draw_state *state;

	if (dev->top == 0)
	{
		base_warn(ctx, "Unexpected pop clip");
		return;
	}
	state = &dev->stack[--dev->top];

	
	if (state[1].mask)
	{
#ifdef DUMP_GROUP_BLENDS
		dump_spaces(dev->top, "");
		base_dump_blend(dev->ctx, state[1].dest, "Clipping ");
		if (state[1].shape)
			base_dump_blend(dev->ctx, state[1].shape, "/");
		base_dump_blend(dev->ctx, state[0].dest, " onto ");
		if (state[0].shape)
			base_dump_blend(dev->ctx, state[0].shape, "/");
		base_dump_blend(dev->ctx, state[1].mask, " with ");
#endif
		base_paint_pixmap_with_mask(state[0].dest, state[1].dest, state[1].mask);
		if (state[0].shape != state[1].shape)
		{
			base_paint_pixmap_with_mask(state[0].shape, state[1].shape, state[1].mask);
			base_drop_pixmap(dev->ctx, state[1].shape);
		}
		base_drop_pixmap(dev->ctx, state[1].mask);
		base_drop_pixmap(dev->ctx, state[1].dest);
#ifdef DUMP_GROUP_BLENDS
		base_dump_blend(dev->ctx, state[0].dest, " to get ");
		if (state[0].shape)
			base_dump_blend(dev->ctx, state[0].shape, "/");
		printf("\n");
#endif
	}
	else
	{
#ifdef DUMP_GROUP_BLENDS
		dump_spaces(dev->top, "Clip end\n");
#endif
	}
}

static void
base_draw_begin_mask(base_device *devp, base_rect rect, int luminosity, base_colorspace *colorspace, float *colorfv)
{
	base_draw_device *dev = devp->user;
	base_pixmap *dest;
	base_bbox bbox;
	base_draw_state *state = push_stack(dev);
	base_pixmap *shape = state->shape;

	bbox = base_bbox_covering_rect(rect);
	bbox = base_intersect_bbox(bbox, state->scissor);
	dest = base_new_pixmap_with_bbox(dev->ctx, base_device_gray, bbox);
	if (state->shape)
	{
		
		shape = NULL;
	}

	if (luminosity)
	{
		float bc;
		if (!colorspace)
			colorspace = base_device_gray;
		base_convert_color(dev->ctx, base_device_gray, &bc, colorspace, colorfv);
		base_clear_pixmap_with_value(dev->ctx, dest, bc * 255);
		if (shape)
			base_clear_pixmap_with_value(dev->ctx, shape, 255);
	}
	else
	{
		base_clear_pixmap(dev->ctx, dest);
		if (shape)
			base_clear_pixmap(dev->ctx, shape);
	}

#ifdef DUMP_GROUP_BLENDS
	dump_spaces(dev->top-1, "Mask begin\n");
#endif
	state[1].scissor = bbox;
	state[1].dest = dest;
	state[1].shape = shape;
	state[1].luminosity = luminosity;
}

static void
base_draw_end_mask(base_device *devp)
{
	base_draw_device *dev = devp->user;
	base_pixmap *temp, *dest;
	base_bbox bbox;
	int luminosity;
	base_context *ctx = dev->ctx;
	base_draw_state *state;

	if (dev->top == 0)
	{
		base_warn(ctx, "Unexpected draw_end_mask");
		return;
	}
	state = &dev->stack[dev->top-1];
	
	luminosity = state[1].luminosity;

#ifdef DUMP_GROUP_BLENDS
	dump_spaces(dev->top-1, "Mask -> Clip\n");
#endif
	
	temp = base_alpha_from_gray(dev->ctx, state[1].dest, luminosity);
	if (state[1].dest != state[0].dest)
		base_drop_pixmap(dev->ctx, state[1].dest);
	state[1].dest = NULL;
	if (state[1].shape != state[0].shape)
		base_drop_pixmap(dev->ctx, state[1].shape);
	state[1].shape = NULL;
	if (state[1].mask != state[0].mask)
		base_drop_pixmap(dev->ctx, state[1].mask);
	state[1].mask = NULL;

	
	bbox = base_pixmap_bbox(ctx, temp);
	dest = base_new_pixmap_with_bbox(dev->ctx, state->dest->colorspace, bbox);
	base_clear_pixmap(dev->ctx, dest);

	
	state[1].mask = temp;
	state[1].dest = dest;
	state[1].blendmode |= base_BLEND_ISOLATED;
	
	if (state[0].shape)
	{
		state[1].shape = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
		base_clear_pixmap(dev->ctx, state[1].shape);
	}
	state[1].scissor = bbox;
}

static void
base_draw_begin_group(base_device *devp, base_rect rect, int isolated, int knockout, int blendmode, float alpha)
{
	base_draw_device *dev = devp->user;
	base_bbox bbox;
	base_pixmap *dest, *shape;
	base_context *ctx = dev->ctx;
	base_draw_state *state = &dev->stack[dev->top];
	base_colorspace *model = state->dest->colorspace;

	if (state->blendmode & base_BLEND_KNOCKOUT)
		base_knockout_begin(dev);

	state = push_stack(dev);
	bbox = base_bbox_covering_rect(rect);
	bbox = base_intersect_bbox(bbox, state->scissor);
	dest = base_new_pixmap_with_bbox(ctx, model, bbox);

#ifndef ATTEMPT_KNOCKOUT_AND_ISOLATED
	knockout = 0;
	isolated = 1;
#endif

	if (isolated)
	{
		base_clear_pixmap(dev->ctx, dest);
	}
	else
	{
		base_copy_pixmap_rect(dev->ctx, dest, state[0].dest, bbox);
	}

	if (blendmode == 0 && alpha == 1.0 && isolated)
	{
		
		shape = state[0].shape;
	}
	else
	{
		base_try(ctx)
		{
			shape = base_new_pixmap_with_bbox(ctx, NULL, bbox);
			base_clear_pixmap(dev->ctx, shape);
		}
		base_catch(ctx)
		{
			base_drop_pixmap(ctx, dest);
			base_rethrow(ctx);
		}
	}

	state[1].alpha = alpha;
#ifdef DUMP_GROUP_BLENDS
	dump_spaces(dev->top-1, "Group begin\n");
#endif

	state[1].scissor = bbox;
	state[1].dest = dest;
	state[1].shape = shape;
	state[1].blendmode = blendmode | (isolated ? base_BLEND_ISOLATED : 0) | (knockout ? base_BLEND_KNOCKOUT : 0);
}

static void
base_draw_end_group(base_device *devp)
{
	base_draw_device *dev = devp->user;
	int blendmode;
	int isolated;
	float alpha;
	base_context *ctx = dev->ctx;
	base_draw_state *state;

	if (dev->top == 0)
	{
		base_warn(ctx, "Unexpected end_group");
		return;
	}

	state = &dev->stack[--dev->top];
	alpha = state[1].alpha;
	blendmode = state[1].blendmode & base_BLEND_MODEMASK;
	isolated = state[1].blendmode & base_BLEND_ISOLATED;
#ifdef DUMP_GROUP_BLENDS
	dump_spaces(dev->top, "");
	base_dump_blend(dev->ctx, state[1].dest, "Group end: blending ");
	if (state[1].shape)
		base_dump_blend(dev->ctx, state[1].shape, "/");
	base_dump_blend(dev->ctx, state[0].dest, " onto ");
	if (state[0].shape)
		base_dump_blend(dev->ctx, state[0].shape, "/");
	if (alpha != 1.0f)
		printf(" (alpha %g)", alpha);
	if (blendmode != 0)
		printf(" (blend %d)", blendmode);
	if (isolated != 0)
		printf(" (isolated)");
	if (state[1].blendmode & base_BLEND_KNOCKOUT)
		printf(" (knockout)");
#endif
	if ((blendmode == 0) && (state[0].shape == state[1].shape))
		base_paint_pixmap(state[0].dest, state[1].dest, alpha * 255);
	else
		base_blend_pixmap(state[0].dest, state[1].dest, alpha * 255, blendmode, isolated, state[1].shape);

	base_drop_pixmap(dev->ctx, state[1].dest);
	if (state[0].shape != state[1].shape)
	{
		if (state[0].shape)
			base_paint_pixmap(state[0].shape, state[1].shape, alpha * 255);
		base_drop_pixmap(dev->ctx, state[1].shape);
	}
#ifdef DUMP_GROUP_BLENDS
	base_dump_blend(dev->ctx, state[0].dest, " to get ");
	if (state[0].shape)
		base_dump_blend(dev->ctx, state[0].shape, "/");
	printf("\n");
#endif

	if (state[0].blendmode & base_BLEND_KNOCKOUT)
		base_knockout_end(dev);
}

static void
base_draw_begin_tile(base_device *devp, base_rect area, base_rect view, float xstep, float ystep, base_matrix ctm)
{
	base_draw_device *dev = devp->user;
	base_pixmap *dest = NULL;
	base_pixmap *shape;
	base_bbox bbox;
	base_context *ctx = dev->ctx;
	base_draw_state *state = &dev->stack[dev->top];
	base_colorspace *model = state->dest->colorspace;

	
	

	if (state->blendmode & base_BLEND_KNOCKOUT)
		base_knockout_begin(dev);

	state = push_stack(dev);
	bbox = base_bbox_covering_rect(base_transform_rect(ctm, view));
	
	dest = base_new_pixmap_with_bbox(dev->ctx, model, bbox);
	base_clear_pixmap(ctx, dest);
	shape = state[0].shape;
	if (shape)
	{
		base_var(shape);
		base_try(ctx)
		{
			shape = base_new_pixmap_with_bbox(dev->ctx, NULL, bbox);
			base_clear_pixmap(ctx, shape);
		}
		base_catch(ctx)
		{
			base_drop_pixmap(ctx, dest);
			base_rethrow(ctx);
		}
	}
	state[1].blendmode |= base_BLEND_ISOLATED;
	state[1].xstep = xstep;
	state[1].ystep = ystep;
	state[1].area = area;
	state[1].ctm = ctm;
#ifdef DUMP_GROUP_BLENDS
	dump_spaces(dev->top-1, "Tile begin\n");
#endif

	state[1].scissor = bbox;
	state[1].dest = dest;
	state[1].shape = shape;
}

static void
base_draw_end_tile(base_device *devp)
{
	base_draw_device *dev = devp->user;
	float xstep, ystep;
	base_matrix ctm, ttm, shapectm;
	base_rect area;
	int x0, y0, x1, y1, x, y;
	base_context *ctx = dev->ctx;
	base_draw_state *state;

	if (dev->top == 0)
	{
		base_warn(ctx, "Unexpected end_tile");
		return;
	}

	state = &dev->stack[--dev->top];
	xstep = state[1].xstep;
	ystep = state[1].ystep;
	area = state[1].area;
	ctm = state[1].ctm;

	x0 = floorf(area.x0 / xstep);
	y0 = floorf(area.y0 / ystep);
	x1 = ceilf(area.x1 / xstep);
	y1 = ceilf(area.y1 / ystep);

	ctm.e = state[1].dest->x;
	ctm.f = state[1].dest->y;
	if (state[1].shape)
	{
		shapectm = ctm;
		shapectm.e = state[1].shape->x;
		shapectm.f = state[1].shape->y;
	}

#ifdef DUMP_GROUP_BLENDS
	dump_spaces(dev->top, "");
	base_dump_blend(dev->ctx, state[1].dest, "Tiling ");
	if (state[1].shape)
		base_dump_blend(dev->ctx, state[1].shape, "/");
	base_dump_blend(dev->ctx, state[0].dest, " onto ");
	if (state[0].shape)
		base_dump_blend(dev->ctx, state[0].shape, "/");
#endif

	for (y = y0; y < y1; y++)
	{
		for (x = x0; x < x1; x++)
		{
			ttm = base_concat(base_translate(x * xstep, y * ystep), ctm);
			state[1].dest->x = ttm.e;
			state[1].dest->y = ttm.f;
			base_paint_pixmap_with_rect(state[0].dest, state[1].dest, 255, state[0].scissor);
			if (state[1].shape)
			{
				ttm = base_concat(base_translate(x * xstep, y * ystep), shapectm);
				state[1].shape->x = ttm.e;
				state[1].shape->y = ttm.f;
				base_paint_pixmap_with_rect(state[0].shape, state[1].shape, 255, state[0].scissor);
			}
		}
	}

	base_drop_pixmap(dev->ctx, state[1].dest);
	base_drop_pixmap(dev->ctx, state[1].shape);
#ifdef DUMP_GROUP_BLENDS
	base_dump_blend(dev->ctx, state[0].dest, " to get ");
	if (state[0].shape)
		base_dump_blend(dev->ctx, state[0].shape, "/");
	printf("\n");
#endif

	if (state->blendmode & base_BLEND_KNOCKOUT)
		base_knockout_end(dev);
}

static void
base_draw_free_user(base_device *devp)
{
	base_draw_device *dev = devp->user;
	base_context *ctx = dev->ctx;
	
	if (dev->top > 0)
	{
		base_draw_state *state = &dev->stack[--dev->top];
		base_warn(ctx, "items left on stack in draw device: %d", dev->top+1);

		do
		{
			if (state[1].mask != state[0].mask)
				base_drop_pixmap(ctx, state[1].mask);
			if (state[1].dest != state[0].dest)
				base_drop_pixmap(ctx, state[1].dest);
			if (state[1].shape != state[0].shape)
				base_drop_pixmap(ctx, state[1].shape);
			state--;
		}
		while(--dev->top > 0);
	}
	if (dev->stack != &dev->init_stack[0])
		base_free(ctx, dev->stack);
	base_free_gel(dev->gel);
	base_free(ctx, dev);
}

/***************************************************************************************/
/* function name:	base_new_draw_device
/* description:		create base_device variable
/* param 1:			pointer to the context
/* param 2:			destination pixmap
/* return:			base_device
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

base_device *
base_new_draw_device(base_context *ctx, base_pixmap *dest)
{
	base_device *dev = NULL;
	base_draw_device *ddev = base_malloc_struct(ctx, base_draw_device);

	base_var(dev);
	base_try(ctx)
	{
		ddev->gel = base_new_gel(ctx);
		ddev->flags = 0;
		ddev->ctx = ctx;
		ddev->top = 0;
		ddev->stack = &ddev->init_stack[0];
		ddev->stack_max = STACK_SIZE;
		ddev->stack[0].dest = dest;
		ddev->stack[0].shape = NULL;
		ddev->stack[0].mask = NULL;
		ddev->stack[0].blendmode = 0;
		ddev->stack[0].scissor.x0 = dest->x;
		ddev->stack[0].scissor.y0 = dest->y;
		ddev->stack[0].scissor.x1 = dest->x + dest->w;
		ddev->stack[0].scissor.y1 = dest->y + dest->h;

		dev = base_new_device(ctx, ddev);
	}
	base_catch(ctx)
	{
		base_free_gel(ddev->gel);
		base_free(ctx, ddev);
		base_rethrow(ctx);
	}
	dev->free_user = base_draw_free_user;

	dev->fill_path = base_draw_fill_path;
	dev->stroke_path = base_draw_stroke_path;
	dev->clip_path = base_draw_clip_path;
	dev->clip_stroke_path = base_draw_clip_stroke_path;

	dev->my_fill_text = my_base_draw_fill_text;
	dev->fill_text = base_draw_fill_text;
	dev->stroke_text = base_draw_stroke_text;
	dev->clip_text = base_draw_clip_text;
	dev->clip_stroke_text = base_draw_clip_stroke_text;
	dev->ignore_text = base_draw_ignore_text;

	dev->fill_image_mask = base_draw_fill_image_mask;
	dev->clip_image_mask = base_draw_clip_image_mask;
	dev->fill_image = base_draw_fill_image;
	dev->fill_shade = base_draw_fill_shade;

	dev->pop_clip = base_draw_pop_clip;

	dev->begin_mask = base_draw_begin_mask;
	dev->end_mask = base_draw_end_mask;
	dev->begin_group = base_draw_begin_group;
	dev->end_group = base_draw_end_group;

	dev->begin_tile = base_draw_begin_tile;
	dev->end_tile = base_draw_end_tile;

	return dev;
}

base_device *
base_new_draw_device_with_bbox(base_context *ctx, base_pixmap *dest, base_bbox clip)
{
	base_device *dev = base_new_draw_device(ctx, dest);
	base_draw_device *ddev = dev->user;

	if (clip.x0 > ddev->stack[0].scissor.x0)
		ddev->stack[0].scissor.x0 = clip.x0;
	if (clip.x1 < ddev->stack[0].scissor.x1)
		ddev->stack[0].scissor.x1 = clip.x1;
	if (clip.y0 > ddev->stack[0].scissor.y0)
		ddev->stack[0].scissor.y0 = clip.y0;
	if (clip.y1 < ddev->stack[0].scissor.y1)
		ddev->stack[0].scissor.y1 = clip.y1;
	return dev;
}

base_device *
base_new_draw_device_type3(base_context *ctx, base_pixmap *dest)
{
	base_device *dev = base_new_draw_device(ctx, dest);
	base_draw_device *ddev = dev->user;
	ddev->flags |= base_DRAWDEV_FLAGS_TYPE3;
	return dev;
}

void base_save_one_image(base_device *devp, 
						 base_image *image, 
						 base_matrix ctm, 
						 float alpha, 
						 wchar_t *filename, 
						 wchar_t *format)
{
#if 0
	base_pixmap *converted = NULL;
	base_pixmap *scaled = NULL;
	base_pixmap *pixmap;
	base_pixmap *orig_pixmap;
	int dx, dy;
	base_context *ctx = devp->ctx;
	base_colorspace *model = base_device_rgb;

	base_var(scaled);

	if (!model)
	{
		base_warn(ctx, "cannot render image directly to an alpha mask");
		return;
	}

	if (image->w == 0 || image->h == 0)
		return;

	dx = sqrtf(ctm.a * ctm.a + ctm.b * ctm.b);
	dy = sqrtf(ctm.c * ctm.c + ctm.d * ctm.d);

	pixmap = base_image_to_pixmap(ctx, image, dx, dy);

	orig_pixmap = pixmap;


	base_try(ctx)
	{
		if (pixmap->colorspace != model)
		{
			converted = base_new_pixmap_with_bbox(ctx, model, base_pixmap_bbox(ctx, pixmap));
			base_convert_pixmap(ctx, converted, pixmap);
			pixmap = converted;
		}

		if (dx < pixmap->w && dy < pixmap->h)
		{
			int gridfit = alpha == 1.0f && !(0 & base_DRAWDEV_FLAGS_TYPE3);
			scaled = base_transform_pixmap(ctx, pixmap, &ctm, 0, 0, dx, dy, gridfit, NULL);
			if (!scaled)
			{
				if (dx < 1)
					dx = 1;
				if (dy < 1)
					dy = 1;
				scaled = base_scale_pixmap(ctx, pixmap, pixmap->x, pixmap->y, dx, dy, NULL);
			}
			if (scaled)
				pixmap = scaled;
		}

		if (wcscmp(format, L"jpg") == 0)
			base_write_jpeg(ctx, pixmap, filename, 0);
		else if (wcscmp(format, L"png") == 0)
			base_write_png(ctx, pixmap, filename, 0);
		else if (wcscmp(format, L"bmp") == 0)
			base_write_bmp(ctx, pixmap, filename);
		else if (wcscmp(format, L"tiff") == 0)
			base_write_tiff(ctx, pixmap, filename);
		else if (wcscmp(format, L"gif") == 0)
			base_write_gif(ctx, pixmap, filename);
		else
			base_throw(ctx, "cannot save image");

	}
	base_always(ctx)
	{
		base_drop_pixmap(ctx, scaled);
		base_drop_pixmap(ctx, converted);
		base_drop_pixmap(ctx, orig_pixmap);
	}
	base_catch(ctx)
	{
		base_rethrow(ctx);
	}
#endif
}

