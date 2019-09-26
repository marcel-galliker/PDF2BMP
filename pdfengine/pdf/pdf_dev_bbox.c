#include "pdf-internal.h"

static void
base_bbox_fill_path(base_device *dev, base_path *path, int even_odd, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_bbox *result = dev->user;
	base_bbox bbox = base_bbox_covering_rect(base_bound_path(dev->ctx, path, NULL, ctm));
	*result = base_union_bbox(*result, bbox);
}

static void
base_bbox_stroke_path(base_device *dev, base_path *path, base_stroke_state *stroke, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_bbox *result = dev->user;
	base_bbox bbox = base_bbox_covering_rect(base_bound_path(dev->ctx, path, stroke, ctm));
	*result = base_union_bbox(*result, bbox);
}

static void
base_bbox_fill_text(base_device *dev, base_text *text, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_bbox *result = dev->user;
	base_bbox bbox = base_bbox_covering_rect(base_bound_text(dev->ctx, text, ctm));
	*result = base_union_bbox(*result, bbox);
}

static void
base_bbox_stroke_text(base_device *dev, base_text *text, base_stroke_state *stroke, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_bbox *result = dev->user;
	base_bbox bbox = base_bbox_covering_rect(base_bound_text(dev->ctx, text, ctm));
	*result = base_union_bbox(*result, bbox);
}

static void
base_bbox_fill_shade(base_device *dev, base_shade *shade, base_matrix ctm, float alpha)
{
	base_bbox *result = dev->user;
	base_bbox bbox = base_bbox_covering_rect(base_bound_shade(dev->ctx, shade, ctm));
	*result = base_union_bbox(*result, bbox);
}

static void
base_bbox_fill_image(base_device *dev, base_image *image, base_matrix ctm, float alpha)
{
	base_bbox *result = dev->user;
	base_bbox bbox = base_bbox_covering_rect(base_transform_rect(ctm, base_unit_rect));
	*result = base_union_bbox(*result, bbox);
}

static void
base_bbox_fill_image_mask(base_device *dev, base_image *image, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_bbox_fill_image(dev, image, ctm, alpha);
}

base_device *
base_new_bbox_device(base_context *ctx, base_bbox *result)
{
	base_device *dev;

	dev = base_new_device(ctx, result);

	dev->fill_path = base_bbox_fill_path;
	dev->stroke_path = base_bbox_stroke_path;
	dev->fill_text = base_bbox_fill_text;
	dev->stroke_text = base_bbox_stroke_text;
	dev->fill_shade = base_bbox_fill_shade;
	dev->fill_image = base_bbox_fill_image;
	dev->fill_image_mask = base_bbox_fill_image_mask;

	*result = base_empty_bbox;

	return dev;
}
