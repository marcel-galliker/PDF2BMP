#include "pdf-internal.h"

base_device *
base_new_device(base_context *ctx, void *user)
{
	base_device *dev = base_malloc_struct(ctx, base_device);
	dev->hints = 0;
	dev->flags = 0;
	dev->user = user;
	dev->ctx = ctx;
	return dev;
}

/***************************************************************************************/
/* function name:	base_free_device
/* description:		free base_device variable
/* param 1:			base_device to free
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_free_device(base_device *dev)
{
	if (dev == NULL)
		return;
	if (dev->free_user)
		dev->free_user(dev);
	base_free(dev->ctx, dev);
}

/***************************************************************************************/
/* function name:	base_flush_device
/* description:		flush the base_device
/* param 1:			base_device
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_flush_device(base_device *dev)
{
	if (dev == NULL)
		return;
	if (dev->flush_user)
		dev->flush_user(dev);
}

void
base_fill_path(base_device *dev, base_path *path, int even_odd, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	if (dev->fill_path)
		dev->fill_path(dev, path, even_odd, ctm, colorspace, color, alpha);
}

void
base_stroke_path(base_device *dev, base_path *path, base_stroke_state *stroke, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	if (dev->stroke_path)
		dev->stroke_path(dev, path, stroke, ctm, colorspace, color, alpha);
}

void
base_clip_path(base_device *dev, base_path *path, base_rect *rect, int even_odd, base_matrix ctm)
{
	if (dev->clip_path)
		dev->clip_path(dev, path, rect, even_odd, ctm);
}

void
base_clip_stroke_path(base_device *dev, base_path *path, base_rect *rect, base_stroke_state *stroke, base_matrix ctm)
{
	if (dev->clip_stroke_path)
		dev->clip_stroke_path(dev, path, rect, stroke, ctm);
}

void
base_fill_text(base_device *dev, base_text *text, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	if (dev->fill_text)
		dev->fill_text(dev, text, ctm, colorspace, color, alpha);
}

void
my_base_fill_text(base_device *dev, base_text *text, base_matrix ctm,
				base_colorspace *colorspace, float *color, float alpha, base_bbox org_bbox, base_bbox scissor, double dAngle)
{
	if (dev->my_fill_text)
		dev->my_fill_text(dev, text, ctm, colorspace, color, alpha, org_bbox, scissor, dAngle);
}

void
base_stroke_text(base_device *dev, base_text *text, base_stroke_state *stroke, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	if (dev->stroke_text)
		dev->stroke_text(dev, text, stroke, ctm, colorspace, color, alpha);
}

void
base_clip_text(base_device *dev, base_text *text, base_matrix ctm, int accumulate)
{
	if (dev->clip_text)
		dev->clip_text(dev, text, ctm, accumulate);
}

void
base_clip_stroke_text(base_device *dev, base_text *text, base_stroke_state *stroke, base_matrix ctm)
{
	if (dev->clip_stroke_text)
		dev->clip_stroke_text(dev, text, stroke, ctm);
}

void
base_ignore_text(base_device *dev, base_text *text, base_matrix ctm)
{
	if (dev->ignore_text)
		dev->ignore_text(dev, text, ctm);
}

void
base_pop_clip(base_device *dev)
{
	if (dev->pop_clip)
		dev->pop_clip(dev);
}

void
base_fill_shade(base_device *dev, base_shade *shade, base_matrix ctm, float alpha)
{
	if (dev->fill_shade)
		dev->fill_shade(dev, shade, ctm, alpha);
}

void
base_fill_image(base_device *dev, base_image *image, base_matrix ctm, float alpha)
{
	if (dev->fill_image)
		dev->fill_image(dev, image, ctm, alpha);
}

void
base_fill_image_mask(base_device *dev, base_image *image, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	if (dev->fill_image_mask)
		dev->fill_image_mask(dev, image, ctm, colorspace, color, alpha);
}

void
base_clip_image_mask(base_device *dev, base_image *image, base_rect *rect, base_matrix ctm)
{
	if (dev->clip_image_mask)
		dev->clip_image_mask(dev, image, rect, ctm);
}

void
base_begin_mask(base_device *dev, base_rect area, int luminosity, base_colorspace *colorspace, float *bc)
{
	if (dev->begin_mask)
		dev->begin_mask(dev, area, luminosity, colorspace, bc);
}

void
base_end_mask(base_device *dev)
{
	if (dev->end_mask)
		dev->end_mask(dev);
}

void
base_begin_group(base_device *dev, base_rect area, int isolated, int knockout, int blendmode, float alpha)
{
	if (dev->begin_group)
		dev->begin_group(dev, area, isolated, knockout, blendmode, alpha);
}

void
base_end_group(base_device *dev)
{
	if (dev->end_group)
		dev->end_group(dev);
}

void
base_begin_tile(base_device *dev, base_rect area, base_rect view, float xstep, float ystep, base_matrix ctm)
{
	if (dev->begin_tile)
		dev->begin_tile(dev, area, view, xstep, ystep, ctm);
}

void
base_end_tile(base_device *dev)
{
	if (dev->end_tile)
		dev->end_tile(dev);
}
