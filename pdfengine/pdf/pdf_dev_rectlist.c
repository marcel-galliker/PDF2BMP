#include "pdf-internal.h"

enum { ISOLATED = 1, KNOCKOUT = 2 };

static base_display_node *
base_new_display_node(base_context *ctx, base_display_command cmd, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_display_node *node;
	int i;

	node = base_malloc_struct(ctx, base_display_node);
	node->cmd = cmd;
	node->next = NULL;
	node->rect = base_empty_rect;
	node->item.path = NULL;
	node->stroke = NULL;
	node->flag = 0;
	node->ctm = ctm;
	if (colorspace)
	{
		node->colorspace = base_keep_colorspace(ctx, colorspace);
		if (color)
		{
			for (i = 0; i < node->colorspace->n; i++)
				node->color[i] = color[i];
		}
	}
	else
	{
		node->colorspace = NULL;
	}
	node->alpha = alpha;

	return node;
}

static void
base_append_display_node(base_display_list *list, base_display_node *node)
{
	switch (node->cmd)
	{
	case base_CMD_CLIP_PATH:
	case base_CMD_CLIP_STROKE_PATH:
	case base_CMD_CLIP_IMAGE_MASK:
		if (list->top < STACK_SIZE)
		{
			list->stack[list->top].update = &node->rect;
			list->stack[list->top].rect = base_empty_rect;
		}
		list->top++;
		break;
	case base_CMD_END_MASK:
	case base_CMD_CLIP_TEXT:
	case base_CMD_CLIP_STROKE_TEXT:
		if (list->top < STACK_SIZE)
		{
			list->stack[list->top].update = NULL;
			list->stack[list->top].rect = base_empty_rect;
		}
		list->top++;
		break;
	case base_CMD_BEGIN_TILE:
		list->tiled++;
		if (list->top > 0 && list->top <= STACK_SIZE)
		{
			list->stack[list->top-1].rect = base_infinite_rect;
		}
		break;
	case base_CMD_END_TILE:
		list->tiled--;
		break;
	case base_CMD_END_GROUP:
		break;
	case base_CMD_POP_CLIP:
		if (list->top > STACK_SIZE)
		{
			list->top--;
			node->rect = base_infinite_rect;
		}
		else if (list->top > 0)
		{
			base_rect *update;
			list->top--;
			update = list->stack[list->top].update;
			if (list->tiled == 0)
			{
				if (update)
				{
					*update = base_intersect_rect(*update, list->stack[list->top].rect);
					node->rect = *update;
				}
				else
					node->rect = list->stack[list->top].rect;
			}
			else
				node->rect = base_infinite_rect;
		}
		
	default:
		if (list->top > 0 && list->tiled == 0 && list->top <= STACK_SIZE)
			list->stack[list->top-1].rect = base_union_rect(list->stack[list->top-1].rect, node->rect);
		break;
	}
	if (!list->first)
	{
		list->first = node;
		list->last = node;
	}
	else
	{
		list->last->next = node;
		list->last = node;
	}
}

static void
base_free_display_node(base_context *ctx, base_display_node *node)
{
	switch (node->cmd)
	{
	case base_CMD_FILL_PATH:
	case base_CMD_STROKE_PATH:
	case base_CMD_CLIP_PATH:
	case base_CMD_CLIP_STROKE_PATH:
		base_free_path(ctx, node->item.path);
		break;
	case base_CMD_FILL_TEXT:
	case base_CMD_STROKE_TEXT:
	case base_CMD_CLIP_TEXT:
	case base_CMD_CLIP_STROKE_TEXT:
	case base_CMD_IGNORE_TEXT:
		base_free_text(ctx, node->item.text);
		break;
	case base_CMD_FILL_SHADE:
		base_drop_shade(ctx, node->item.shade);
		break;
	case base_CMD_FILL_IMAGE:
	case base_CMD_FILL_IMAGE_MASK:
	case base_CMD_CLIP_IMAGE_MASK:
		base_drop_image(ctx, node->item.image);
		break;
	case base_CMD_POP_CLIP:
	case base_CMD_BEGIN_MASK:
	case base_CMD_END_MASK:
	case base_CMD_BEGIN_GROUP:
	case base_CMD_END_GROUP:
	case base_CMD_BEGIN_TILE:
	case base_CMD_END_TILE:
		break;
	}
	if (node->stroke)
		base_drop_stroke_state(ctx, node->stroke);
	if (node->colorspace)
		base_drop_colorspace(ctx, node->colorspace);
	base_free(ctx, node);
}

static void
base_rect_list_fill_path(base_device *dev, base_path *path, int even_odd, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_display_node *node;
	base_context *ctx = dev->ctx;
	node = base_new_display_node(ctx, base_CMD_FILL_PATH, ctm, colorspace, color, alpha);
	base_try(ctx)
	{
		node->rect = base_bound_path(dev->ctx, path, NULL, ctm);
		node->item.path = base_clone_path(dev->ctx, path);
		node->flag = even_odd;
	}
	base_catch(ctx)
	{
		base_free_display_node(ctx, node);
		base_rethrow(ctx);
	}
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_stroke_path(base_device *dev, base_path *path, base_stroke_state *stroke, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_display_node *node;
	base_context *ctx = dev->ctx;
	node = base_new_display_node(ctx, base_CMD_STROKE_PATH, ctm, colorspace, color, alpha);
	base_try(ctx)
	{
		node->rect = base_bound_path(dev->ctx, path, stroke, ctm);
		node->item.path = base_clone_path(dev->ctx, path);
		node->stroke = base_keep_stroke_state(dev->ctx, stroke);
	}
	base_catch(ctx)
	{
		base_free_display_node(ctx, node);
		base_rethrow(ctx);
	}
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_clip_path(base_device *dev, base_path *path, base_rect *rect, int even_odd, base_matrix ctm)
{
	base_display_node *node;
	base_context *ctx = dev->ctx;
	node = base_new_display_node(ctx, base_CMD_CLIP_PATH, ctm, NULL, NULL, 0);
	base_try(ctx)
	{
		node->rect = base_bound_path(dev->ctx, path, NULL, ctm);
		if (rect)
			node->rect = base_intersect_rect(node->rect, *rect);
		node->item.path = base_clone_path(dev->ctx, path);
		node->flag = even_odd;
	}
	base_catch(ctx)
	{
		base_free_display_node(ctx, node);
		base_rethrow(ctx);
	}
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_clip_stroke_path(base_device *dev, base_path *path, base_rect *rect, base_stroke_state *stroke, base_matrix ctm)
{
	base_display_node *node;
	base_context *ctx = dev->ctx;
	node = base_new_display_node(ctx, base_CMD_CLIP_STROKE_PATH, ctm, NULL, NULL, 0);
	base_try(ctx)
	{
		node->rect = base_bound_path(dev->ctx, path, stroke, ctm);
		if (rect)
			node->rect = base_intersect_rect(node->rect, *rect);
		node->item.path = base_clone_path(dev->ctx, path);
		node->stroke = base_keep_stroke_state(dev->ctx, stroke);
	}
	base_catch(ctx)
	{
		base_free_display_node(ctx, node);
		base_rethrow(ctx);
	}
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_fill_text(base_device *dev, base_text *text, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_display_node *node;
	base_context *ctx = dev->ctx;
	node = base_new_display_node(ctx, base_CMD_FILL_TEXT, ctm, colorspace, color, alpha);
	base_try(ctx)
	{
		node->rect = base_bound_text(dev->ctx, text, ctm);
		node->item.text = base_clone_text(dev->ctx, text);
	}
	base_catch(ctx)
	{
		base_free_display_node(ctx, node);
		base_rethrow(ctx);
	}
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_stroke_text(base_device *dev, base_text *text, base_stroke_state *stroke, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_display_node *node;
	base_context *ctx = dev->ctx;
	node = base_new_display_node(ctx, base_CMD_STROKE_TEXT, ctm, colorspace, color, alpha);
	node->item.text = NULL;
	base_try(ctx)
	{
		node->rect = base_bound_text(dev->ctx, text, ctm);
		node->item.text = base_clone_text(dev->ctx, text);
		node->stroke = base_keep_stroke_state(dev->ctx, stroke);
	}
	base_catch(ctx)
	{
		base_free_display_node(ctx, node);
		base_rethrow(ctx);
	}
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_clip_text(base_device *dev, base_text *text, base_matrix ctm, int accumulate)
{
	base_display_node *node;
	base_context *ctx = dev->ctx;
	node = base_new_display_node(ctx, base_CMD_CLIP_TEXT, ctm, NULL, NULL, 0);
	base_try(ctx)
	{
		node->rect = base_bound_text(dev->ctx, text, ctm);
		node->item.text = base_clone_text(dev->ctx, text);
		node->flag = accumulate;
		
		if (accumulate)
			node->rect = base_infinite_rect;
	}
	base_catch(ctx)
	{
		base_free_display_node(ctx, node);
		base_rethrow(ctx);
	}
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_clip_stroke_text(base_device *dev, base_text *text, base_stroke_state *stroke, base_matrix ctm)
{
	base_display_node *node;
	base_context *ctx = dev->ctx;
	node = base_new_display_node(ctx, base_CMD_CLIP_STROKE_TEXT, ctm, NULL, NULL, 0);
	base_try(ctx)
	{
		node->rect = base_bound_text(dev->ctx, text, ctm);
		node->item.text = base_clone_text(dev->ctx, text);
		node->stroke = base_keep_stroke_state(dev->ctx, stroke);
	}
	base_catch(ctx)
	{
		base_free_display_node(ctx, node);
		base_rethrow(ctx);
	}
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_ignore_text(base_device *dev, base_text *text, base_matrix ctm)
{
	base_display_node *node;
	base_context *ctx = dev->ctx;
	node = base_new_display_node(ctx, base_CMD_IGNORE_TEXT, ctm, NULL, NULL, 0);
	base_try(ctx)
	{
		node->rect = base_bound_text(dev->ctx, text, ctm);
		node->item.text = base_clone_text(dev->ctx, text);
	}
	base_catch(ctx)
	{
		base_free_display_node(ctx, node);
		base_rethrow(ctx);
	}
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_pop_clip(base_device *dev)
{
	base_display_node *node;
	node = base_new_display_node(dev->ctx, base_CMD_POP_CLIP, base_identity, NULL, NULL, 0);
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_fill_shade(base_device *dev, base_shade *shade, base_matrix ctm, float alpha)
{
	base_display_node *node;
	base_context *ctx = dev->ctx;
	node = base_new_display_node(ctx, base_CMD_FILL_SHADE, ctm, NULL, NULL, alpha);
	node->rect = base_bound_shade(ctx, shade, ctm);
	node->item.shade = base_keep_shade(ctx, shade);
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_fill_image(base_device *dev, base_image *image, base_matrix ctm, float alpha)
{
	base_display_node *node;
	node = base_new_display_node(dev->ctx, base_CMD_FILL_IMAGE, ctm, NULL, NULL, alpha);
	node->rect = base_transform_rect(ctm, base_unit_rect);
	node->item.image = base_keep_image(dev->ctx, image);
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_fill_image_mask(base_device *dev, base_image *image, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_display_node *node;
	node = base_new_display_node(dev->ctx, base_CMD_FILL_IMAGE_MASK, ctm, colorspace, color, alpha);
	node->rect = base_transform_rect(ctm, base_unit_rect);
	node->item.image = base_keep_image(dev->ctx, image);
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_clip_image_mask(base_device *dev, base_image *image, base_rect *rect, base_matrix ctm)
{
	base_display_node *node;
	node = base_new_display_node(dev->ctx, base_CMD_CLIP_IMAGE_MASK, ctm, NULL, NULL, 0);
	node->rect = base_transform_rect(ctm, base_unit_rect);
	if (rect)
		node->rect = base_intersect_rect(node->rect, *rect);
	node->item.image = base_keep_image(dev->ctx, image);
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_begin_mask(base_device *dev, base_rect rect, int luminosity, base_colorspace *colorspace, float *color)
{
	base_display_node *node;
	node = base_new_display_node(dev->ctx, base_CMD_BEGIN_MASK, base_identity, colorspace, color, 0);
	node->rect = rect;
	node->flag = luminosity;
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_end_mask(base_device *dev)
{
	base_display_node *node;
	node = base_new_display_node(dev->ctx, base_CMD_END_MASK, base_identity, NULL, NULL, 0);
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_begin_group(base_device *dev, base_rect rect, int isolated, int knockout, int blendmode, float alpha)
{
	base_display_node *node;
	node = base_new_display_node(dev->ctx, base_CMD_BEGIN_GROUP, base_identity, NULL, NULL, alpha);
	node->rect = rect;
	node->item.blendmode = blendmode;
	node->flag |= isolated ? ISOLATED : 0;
	node->flag |= knockout ? KNOCKOUT : 0;
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_end_group(base_device *dev)
{
	base_display_node *node;
	node = base_new_display_node(dev->ctx, base_CMD_END_GROUP, base_identity, NULL, NULL, 0);
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_begin_tile(base_device *dev, base_rect area, base_rect view, float xstep, float ystep, base_matrix ctm)
{
	base_display_node *node;
	node = base_new_display_node(dev->ctx, base_CMD_BEGIN_TILE, ctm, NULL, NULL, 0);
	node->rect = area;
	node->color[0] = xstep;
	node->color[1] = ystep;
	node->color[2] = view.x0;
	node->color[3] = view.y0;
	node->color[4] = view.x1;
	node->color[5] = view.y1;
	base_append_display_node(dev->user, node);
}

static void
base_rect_list_end_tile(base_device *dev)
{
	base_display_node *node;
	node = base_new_display_node(dev->ctx, base_CMD_END_TILE, base_identity, NULL, NULL, 0);
	base_append_display_node(dev->user, node);
}

base_device *
base_new_rect_list_device(base_context *ctx, base_display_list *list)
{
	base_device *dev = base_new_device(ctx, list);

	dev->fill_path = base_rect_list_fill_path;
	dev->stroke_path = base_rect_list_stroke_path;
	dev->clip_path = base_rect_list_clip_path;
	dev->clip_stroke_path = base_rect_list_clip_stroke_path;

	dev->fill_text = NULL;
	dev->stroke_text = NULL;
	dev->clip_text = NULL;
	dev->clip_stroke_text = NULL;
	dev->ignore_text = NULL;

	dev->fill_shade = NULL;
	dev->fill_image = NULL;
	dev->fill_image_mask = NULL;
	dev->clip_image_mask = NULL;

	dev->pop_clip = NULL;

	dev->begin_mask = NULL;
	dev->end_mask = NULL;
	dev->begin_group = NULL;
	dev->end_group = NULL;

	dev->begin_tile = NULL;
	dev->end_tile = NULL;

	return dev;
}

base_display_list *
base_new_display_rect_list(base_context *ctx)
{
	base_display_list *list = base_malloc_struct(ctx, base_display_list);
	list->first = NULL;
	list->last = NULL;
	list->top = 0;
	list->tiled = 0;
	return list;
}

void
base_free_display_rect_list(base_context *ctx, base_display_list *list)
{
	base_display_node *node;

	if (list == NULL)
		return;
	node = list->first;
	while (node)
	{
		base_display_node *next = node->next;
		base_free_display_node(ctx, node);
		node = next;
	}
	base_free(ctx, list);
}

void
base_run_display_rect_list(base_display_list *list, base_device *dev, base_matrix top_ctm, base_bbox scissor, base_cookie *cookie)
{
	base_display_node *node;
	base_matrix ctm;
	base_rect rect;
	base_bbox bbox;
	int clipped = 0;
	int tiled = 0;
	int empty;
	int progress = 0;

	if (cookie)
	{
		cookie->progress_max = list->last - list->first;
		cookie->progress = 0;
	}

	for (node = list->first; node; node = node->next)
	{
		
		if (cookie)
		{
			if (cookie->abort)
				break;
			cookie->progress = progress++;
		}

		

		if (tiled || node->cmd == base_CMD_BEGIN_TILE || node->cmd == base_CMD_END_TILE)
		{
			empty = 0;
		}
		else
		{
			bbox = base_bbox_covering_rect(base_transform_rect(top_ctm, node->rect));
			bbox = base_intersect_bbox(bbox, scissor);
			empty = base_is_empty_bbox(bbox);
		}

		if (clipped || empty)
		{
			switch (node->cmd)
			{
			case base_CMD_CLIP_PATH:
			case base_CMD_CLIP_STROKE_PATH:
			case base_CMD_CLIP_STROKE_TEXT:
			case base_CMD_CLIP_IMAGE_MASK:
			case base_CMD_BEGIN_MASK:
			case base_CMD_BEGIN_GROUP:
				clipped++;
				continue;
			case base_CMD_CLIP_TEXT:
				
				if (node->flag != 2)
					clipped++;
				continue;
			case base_CMD_POP_CLIP:
			case base_CMD_END_GROUP:
				if (!clipped)
					goto visible;
				clipped--;
				continue;
			case base_CMD_END_MASK:
				if (!clipped)
					goto visible;
				continue;
			default:
				continue;
			}
		}

visible:
		ctm = base_concat(node->ctm, top_ctm);

		switch (node->cmd)
		{
		case base_CMD_FILL_PATH:
			base_fill_path(dev, node->item.path, node->flag, ctm,
				node->colorspace, node->color, node->alpha);
			break;
		case base_CMD_STROKE_PATH:
			base_stroke_path(dev, node->item.path, node->stroke, ctm,
				node->colorspace, node->color, node->alpha);
			break;
		case base_CMD_CLIP_PATH:
		{
			base_rect trect = base_transform_rect(top_ctm, node->rect);
			base_clip_path(dev, node->item.path, &trect, node->flag, ctm);
			break;
		}
		case base_CMD_CLIP_STROKE_PATH:
		{
			base_rect trect = base_transform_rect(top_ctm, node->rect);
			base_clip_stroke_path(dev, node->item.path, &trect, node->stroke, ctm);
			break;
		}
		case base_CMD_FILL_TEXT:
			base_fill_text(dev, node->item.text, ctm,
				node->colorspace, node->color, node->alpha);
			break;
		case base_CMD_STROKE_TEXT:
			base_stroke_text(dev, node->item.text, node->stroke, ctm,
				node->colorspace, node->color, node->alpha);
			break;
		case base_CMD_CLIP_TEXT:
			base_clip_text(dev, node->item.text, ctm, node->flag);
			break;
		case base_CMD_CLIP_STROKE_TEXT:
			base_clip_stroke_text(dev, node->item.text, node->stroke, ctm);
			break;
		case base_CMD_IGNORE_TEXT:
			base_ignore_text(dev, node->item.text, ctm);
			break;
		case base_CMD_FILL_SHADE:
			base_fill_shade(dev, node->item.shade, ctm, node->alpha);
			break;
		case base_CMD_FILL_IMAGE:
			base_fill_image(dev, node->item.image, ctm, node->alpha);
			break;
		case base_CMD_FILL_IMAGE_MASK:
			base_fill_image_mask(dev, node->item.image, ctm,
				node->colorspace, node->color, node->alpha);
			break;
		case base_CMD_CLIP_IMAGE_MASK:
		{
			base_rect trect = base_transform_rect(top_ctm, node->rect);
			base_clip_image_mask(dev, node->item.image, &trect, ctm);
			break;
		}
		case base_CMD_POP_CLIP:
			base_pop_clip(dev);
			break;
		case base_CMD_BEGIN_MASK:
			rect = base_transform_rect(top_ctm, node->rect);
			base_begin_mask(dev, rect, node->flag, node->colorspace, node->color);
			break;
		case base_CMD_END_MASK:
			base_end_mask(dev);
			break;
		case base_CMD_BEGIN_GROUP:
			rect = base_transform_rect(top_ctm, node->rect);
			base_begin_group(dev, rect,
				(node->flag & ISOLATED) != 0, (node->flag & KNOCKOUT) != 0,
				node->item.blendmode, node->alpha);
			break;
		case base_CMD_END_GROUP:
			base_end_group(dev);
			break;
		case base_CMD_BEGIN_TILE:
			tiled++;
			rect.x0 = node->color[2];
			rect.y0 = node->color[3];
			rect.x1 = node->color[4];
			rect.y1 = node->color[5];
			base_begin_tile(dev, node->rect, rect,
				node->color[0], node->color[1], ctm);
			break;
		case base_CMD_END_TILE:
			tiled--;
			base_end_tile(dev);
			break;
		}
	}
}
