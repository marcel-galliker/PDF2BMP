#include "pdf-internal.h"

typedef struct base_html_device_s base_html_device;

struct base_html_device_s
{
	base_device *textdev;
	base_device *drawdev;
};

static void
base_html_fill_path(base_device *dev, base_path *path, int even_odd, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->fill_path)
		devp->drawdev->fill_path(devp->drawdev, path, even_odd, ctm, colorspace, color, alpha);
}

static void
base_html_stroke_path(base_device *dev, base_path *path, base_stroke_state *stroke, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->stroke_path)
		devp->drawdev->stroke_path(devp->drawdev, path, stroke, ctm, colorspace, color, alpha);

}

static void
base_html_clip_path(base_device *dev, base_path *path, base_rect *rect, int even_odd, base_matrix ctm)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->clip_path)
		devp->drawdev->clip_path(devp->drawdev, path, rect, even_odd, ctm);
}

static void
base_html_clip_stroke_path(base_device *dev, base_path *path, base_rect *rect, base_stroke_state *stroke, base_matrix ctm)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->clip_stroke_path)
		devp->drawdev->clip_stroke_path(devp->drawdev, path, rect, stroke, ctm);
}

static void
base_html_fill_text(base_device *dev, base_text *text, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_html_device *devp = dev->user;
	if (devp->textdev && devp->textdev->fill_text)
		devp->textdev->fill_text(devp->textdev, text, ctm, colorspace, color, alpha);
}

static void
base_html_stroke_text(base_device *dev, base_text *text, base_stroke_state *stroke, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_html_device *devp = dev->user;
	if (devp->textdev && devp->textdev->stroke_text)
		devp->textdev->stroke_text(devp->textdev, text, stroke, ctm, colorspace, color, alpha);
}

static void
base_html_clip_text(base_device *dev, base_text *text, base_matrix ctm, int accumulate)
{
	base_html_device *devp = dev->user;
	if (devp->textdev && devp->textdev->clip_text)
		devp->textdev->clip_text(devp->textdev, text, ctm, accumulate);
}

static void
base_html_clip_stroke_text(base_device *dev, base_text *text, base_stroke_state *stroke, base_matrix ctm)
{
	base_html_device *devp = dev->user;
	if (devp->textdev && devp->textdev->clip_stroke_text)
		devp->textdev->clip_stroke_text(devp->textdev, text, stroke, ctm);
}

static void
base_html_ignore_text(base_device *dev, base_text *text, base_matrix ctm)
{
	base_html_device *devp = dev->user;
	if (devp->textdev && devp->textdev->ignore_text)
		devp->textdev->ignore_text(devp->textdev, text, ctm);
}

static void
base_html_fill_shade(base_device *dev, base_shade *shade, base_matrix ctm, float alpha)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->fill_shade)
		devp->drawdev->fill_shade(devp->drawdev, shade, ctm, alpha);
}

static void
base_html_fill_image(base_device *dev, base_image *image, base_matrix ctm, float alpha)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->fill_image)
		devp->drawdev->fill_image(devp->drawdev, image, ctm, alpha);
}

static void
base_html_fill_image_mask(base_device *dev, base_image *image, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->fill_image_mask)
		devp->drawdev->fill_image_mask(devp->drawdev, image, ctm, colorspace, color, alpha);
}

static void
base_html_clip_image_mask(base_device *dev, base_image *image, base_rect *rect, base_matrix ctm)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->clip_image_mask)
		devp->drawdev->clip_image_mask(devp->drawdev, image, rect, ctm);
}

static void
base_html_pop_clip(base_device *dev)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->pop_clip)
		devp->drawdev->pop_clip(devp->drawdev);
}

static void
base_html_begin_mask(base_device *dev, base_rect rect, int luminosity, base_colorspace *colorspace, float *colorfv)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->begin_mask)
		devp->drawdev->begin_mask(devp->drawdev, rect, luminosity, colorspace, colorfv);
}

static void
base_html_end_mask(base_device *dev)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->end_mask)
		devp->drawdev->end_mask(devp->drawdev);
}

static void
base_html_begin_group(base_device *dev, base_rect rect, int isolated, int knockout, int blendmode, float alpha)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->begin_group)
		devp->drawdev->begin_group(devp->drawdev, rect, isolated, knockout, blendmode, alpha);
}

static void
base_html_end_group(base_device *dev)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->end_group)
		devp->drawdev->end_group(devp->drawdev);
}

static void
base_html_begin_tile(base_device *dev, base_rect area, base_rect view, float xstep, float ystep, base_matrix ctm)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->begin_tile)
		devp->drawdev->begin_tile(devp->drawdev, area, view, xstep, ystep, ctm);
}

static void
base_html_end_tile(base_device *dev)
{
	base_html_device *devp = dev->user;
	if (devp->drawdev && devp->drawdev->end_tile)
		devp->drawdev->end_tile(devp->drawdev);
}

static void
base_html_free_user(base_device *dev)
{
	base_html_device *htmldev = dev->user;
	base_context *ctx = dev->ctx;

	if (htmldev == NULL || ctx == NULL)
		return;

	base_free_device(htmldev->textdev);
	base_free_device(htmldev->drawdev);
	base_free(ctx, htmldev);
}

static void
base_html_flush_user(base_device *dev)
{
	base_html_device *htmldev = dev->user;

	base_flush_device(htmldev->textdev);
}

/***************************************************************************************/
/* function name:	base_new_html_device
/* description:		create base_device variable
/* param 1:			pointer to the context
/* param 2:			base_text_sheet
/* param 3:			base_text_page
/* param 4:			destination pixmap
/* return:			base_device
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

base_device *
base_new_html_device(base_context *ctx, base_text_sheet *sheet, base_text_page *page, base_pixmap *dest)
{
	base_device *dev;

	base_html_device *htmldev = base_malloc_struct(ctx, base_html_device);

	htmldev->textdev = base_new_text_device(ctx, sheet, page);
	htmldev->drawdev = base_new_draw_device(ctx, dest);

	dev = base_new_device(ctx, htmldev);

	dev->free_user = base_html_free_user;
	dev->flush_user = base_html_flush_user;

	dev->fill_path = base_html_fill_path;
	dev->stroke_path = base_html_stroke_path;
	dev->clip_path = base_html_clip_path;
	dev->clip_stroke_path = base_html_clip_stroke_path;

	dev->fill_text = base_html_fill_text;
	dev->stroke_text = base_html_stroke_text;
	dev->clip_text = NULL;
	dev->clip_stroke_text = NULL;
	dev->ignore_text = NULL;
	//dev->clip_text = base_html_clip_text;
	//dev->clip_stroke_text = base_html_clip_stroke_text;
	//dev->ignore_text = base_html_ignore_text;

	dev->fill_image_mask = base_html_fill_image_mask;
	dev->clip_image_mask = base_html_clip_image_mask;
	dev->fill_image = base_html_fill_image;
	dev->fill_shade = base_html_fill_shade;

	dev->pop_clip = base_html_pop_clip;

	dev->begin_mask = base_html_begin_mask;
	dev->end_mask = base_html_end_mask;
	dev->begin_group = base_html_begin_group;
	dev->end_group = base_html_end_group;

	dev->begin_tile = base_html_begin_tile;
	dev->end_tile = base_html_end_tile;

	return dev;
}
