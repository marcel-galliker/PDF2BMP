#include "pdf-internal.h"
#include "pdfengine-internal.h"

static base_colorspace *
load_icc_based(pdf_document *xref, pdf_obj *dict)
{
	int n;

	n = pdf_to_int(pdf_dict_gets(dict, "N"));

	switch (n)
	{
	case 1: return base_device_gray;
	case 3: return base_device_rgb;
	case 4: return base_device_cmyk;
	}

	base_throw(xref->ctx, "syntaxerror: ICCBased must have 1, 3 or 4 components");
	return NULL; 
}

static inline float fung(float x)
{
	if (x >= 6.0f / 29.0f)
		return x * x * x;
	return (108.0f / 841.0f) * (x - (4.0f / 29.0f));
}

static void
lab_to_rgb(base_context *ctx, base_colorspace *cs, float *lab, float *rgb)
{
	
	float lstar, astar, bstar, l, m, n, x, y, z, r, g, b;
	lstar = lab[0];
	astar = lab[1];
	bstar = lab[2];
	m = (lstar + 16) / 116;
	l = m + astar / 500;
	n = m - bstar / 200;
	x = fung(l);
	y = fung(m);
	z = fung(n);
	r = (3.240449f * x + -1.537136f * y + -0.498531f * z) * 0.830026f;
	g = (-0.969265f * x + 1.876011f * y + 0.041556f * z) * 1.05452f;
	b = (0.055643f * x + -0.204026f * y + 1.057229f * z) * 1.1003f;
	rgb[0] = sqrtf(CLAMP(r, 0, 1));
	rgb[1] = sqrtf(CLAMP(g, 0, 1));
	rgb[2] = sqrtf(CLAMP(b, 0, 1));
}

static void
rgb_to_lab(base_context *ctx, base_colorspace *cs, float *rgb, float *lab)
{
	base_warn(ctx, "cannot convert into L*a*b colorspace");
	lab[0] = rgb[0];
	lab[1] = rgb[1];
	lab[2] = rgb[2];
}

static base_colorspace k_device_lab = { {-1, base_free_colorspace_imp}, 0, "Lab", 3, lab_to_rgb, rgb_to_lab };
static base_colorspace *base_device_lab = &k_device_lab;

struct separation
{
	base_colorspace *base;
	pdf_function *tint;
};

static void
separation_to_rgb(base_context *ctx, base_colorspace *cs, float *color, float *rgb)
{
	struct separation *sep = cs->data;
	float alt[base_MAX_COLORS];
	pdf_eval_function(ctx, sep->tint, color, cs->n, alt, sep->base->n);
	sep->base->to_rgb(ctx, sep->base, alt, rgb);
}

static void
free_separation(base_context *ctx, base_colorspace *cs)
{
	struct separation *sep = cs->data;
	base_drop_colorspace(ctx, sep->base);
	pdf_drop_function(ctx, sep->tint);
	base_free(ctx, sep);
}

static base_colorspace *
load_separation(pdf_document *xref, pdf_obj *array)
{
	base_colorspace *cs;
	struct separation *sep = NULL;
	base_context *ctx = xref->ctx;
	pdf_obj *nameobj = pdf_array_get(array, 1);
	pdf_obj *baseobj = pdf_array_get(array, 2);
	pdf_obj *tintobj = pdf_array_get(array, 3);
	base_colorspace *base;
	pdf_function *tint = NULL;
	int n;

	base_var(tint);
	base_var(sep);

	if (pdf_is_array(nameobj))
		n = pdf_array_len(nameobj);
	else
		n = 1;

	if (n > base_MAX_COLORS)
		base_throw(ctx, "too many components in colorspace");

	base = pdf_load_colorspace(xref, baseobj);
	

	base_try(ctx)
	{
		tint = pdf_load_function(xref, tintobj);
		

		sep = base_malloc_struct(ctx, struct separation);
		sep->base = base;
		sep->tint = tint;

		cs = base_new_colorspace(ctx, n == 1 ? "Separation" : "DeviceN", n);
		cs->to_rgb = separation_to_rgb;
		cs->free_data = free_separation;
		cs->data = sep;
		cs->size += sizeof(struct separation) + (base ? base->size : 0) + pdf_function_size(tint);
	}
	base_catch(ctx)
	{
		base_drop_colorspace(ctx, base);
		pdf_drop_function(ctx, tint);
		base_free(ctx, sep);
		base_rethrow(ctx);
	}

	return cs;
}

struct indexed
{
	base_colorspace *base;
	int high;
	unsigned char *lookup;
};

static void
indexed_to_rgb(base_context *ctx, base_colorspace *cs, float *color, float *rgb)
{
	struct indexed *idx = cs->data;
	float alt[base_MAX_COLORS];
	int i, k;
	i = color[0] * 255;
	i = CLAMP(i, 0, idx->high);
	for (k = 0; k < idx->base->n; k++)
		alt[k] = idx->lookup[i * idx->base->n + k] / 255.0f;
	idx->base->to_rgb(ctx, idx->base, alt, rgb);
}

static void
free_indexed(base_context *ctx, base_colorspace *cs)
{
	struct indexed *idx = cs->data;
	if (idx->base)
		base_drop_colorspace(ctx, idx->base);
	base_free(ctx, idx->lookup);
	base_free(ctx, idx);
}

base_pixmap *
pdf_expand_indexed_pixmap(base_context *ctx, base_pixmap *src)
{
	struct indexed *idx;
	base_pixmap *dst;
	unsigned char *s, *d;
	int y, x, k, n, high;
	unsigned char *lookup;

	assert(src->colorspace->to_rgb == indexed_to_rgb);
	assert(src->n == 2);

	idx = src->colorspace->data;
	high = idx->high;
	lookup = idx->lookup;
	n = idx->base->n;

	dst = base_new_pixmap_with_bbox(ctx, idx->base, base_pixmap_bbox(ctx, src));
	s = src->samples;
	d = dst->samples;

	for (y = 0; y < src->h; y++)
	{
		for (x = 0; x < src->w; x++)
		{
			int v = *s++;
			int a = *s++;
			v = MIN(v, high);
			for (k = 0; k < n; k++)
				*d++ = base_mul255(lookup[v * n + k], a);
			*d++ = a;
		}
	}

	dst->interpolate = src->interpolate;

	return dst;
}

static base_colorspace *
load_indexed(pdf_document *xref, pdf_obj *array)
{
	struct indexed *idx = NULL;
	base_context *ctx = xref->ctx;
	pdf_obj *baseobj = pdf_array_get(array, 1);
	pdf_obj *highobj = pdf_array_get(array, 2);
	pdf_obj *lookup = pdf_array_get(array, 3);
	base_colorspace *base = NULL;
	base_colorspace *cs = NULL;
	int i, n;

	base_var(idx);
	base_var(base);
	base_var(cs);

	base_try(ctx)
	{
		base = pdf_load_colorspace(xref, baseobj);
		

		idx = base_malloc_struct(ctx, struct indexed);
		idx->lookup = NULL;
		idx->base = base;
		idx->high = pdf_to_int(highobj);
		idx->high = CLAMP(idx->high, 0, 255);
		n = base->n * (idx->high + 1);
		idx->lookup = base_malloc_array(ctx, 1, n);

		cs = base_new_colorspace(ctx, "Indexed", 1);
		cs->to_rgb = indexed_to_rgb;
		cs->free_data = free_indexed;
		cs->data = idx;
		cs->size += sizeof(*idx) + n + (base ? base->size : 0);

		if (pdf_is_string(lookup) && pdf_to_str_len(lookup) == n)
		{
			unsigned char *buf = (unsigned char *) pdf_to_str_buf(lookup);
			for (i = 0; i < n; i++)
				idx->lookup[i] = buf[i];
		}
		else if (pdf_is_indirect(lookup))
		{
			base_stream *file = NULL;

			base_try(ctx)
			{
				file = pdf_open_stream(xref, pdf_to_num(lookup), pdf_to_gen(lookup));
			}
			base_catch(ctx)
			{
				base_throw(ctx, "cannot open colorspace lookup table (%d 0 R)", pdf_to_num(lookup));
			}

			i = base_read(file, idx->lookup, n);
			if (i < 0)
			{
				base_close(file);
				base_throw(ctx, "cannot read colorspace lookup table (%d 0 R)", pdf_to_num(lookup));
			}

			base_close(file);
		}
		else
		{
			base_throw(ctx, "cannot parse colorspace lookup table");
		}
	}
	base_catch(ctx)
	{
		if (cs == NULL || cs->data != idx)
		{
			base_drop_colorspace(ctx, base);
			if (idx)
				base_free(ctx, idx->lookup);
			base_free(ctx, idx);
		}
		base_drop_colorspace(ctx, cs);
		base_rethrow(ctx);
	}

	return cs;
}

static base_colorspace *
pdf_load_colorspace_imp(pdf_document *xref, pdf_obj *obj)
{
	if (pdf_is_name(obj))
	{
		if (!strcmp(pdf_to_name(obj), "Pattern"))
			return base_device_gray;
		else if (!strcmp(pdf_to_name(obj), "G"))
			return base_device_gray;
		else if (!strcmp(pdf_to_name(obj), "RGB"))
			return base_device_rgb;
		else if (!strcmp(pdf_to_name(obj), "CMYK"))
			return base_device_cmyk;
		else if (!strcmp(pdf_to_name(obj), "DeviceGray"))
			return base_device_gray;
		else if (!strcmp(pdf_to_name(obj), "DeviceRGB"))
			return base_device_rgb;
		else if (!strcmp(pdf_to_name(obj), "DeviceCMYK"))
			return base_device_cmyk;
		else
			base_throw(xref->ctx, "unknown colorspace: %s", pdf_to_name(obj));
	}

	else if (pdf_is_array(obj))
	{
		pdf_obj *name = pdf_array_get(obj, 0);

		if (pdf_is_name(name))
		{
			
			if (!strcmp(pdf_to_name(name), "Pattern"))
			{
				obj = pdf_array_get(obj, 1);
				if (!obj)
				{
					return base_device_gray;
				}

				return pdf_load_colorspace(xref, obj);
				
			}

			else if (!strcmp(pdf_to_name(name), "G"))
				return base_device_gray;
			else if (!strcmp(pdf_to_name(name), "RGB"))
				return base_device_rgb;
			else if (!strcmp(pdf_to_name(name), "CMYK"))
				return base_device_cmyk;
			else if (!strcmp(pdf_to_name(name), "DeviceGray"))
				return base_device_gray;
			else if (!strcmp(pdf_to_name(name), "DeviceRGB"))
				return base_device_rgb;
			else if (!strcmp(pdf_to_name(name), "DeviceCMYK"))
				return base_device_cmyk;
			else if (!strcmp(pdf_to_name(name), "CalGray"))
				return base_device_gray;
			else if (!strcmp(pdf_to_name(name), "CalRGB"))
				return base_device_rgb;
			else if (!strcmp(pdf_to_name(name), "CalCMYK"))
				return base_device_cmyk;
			else if (!strcmp(pdf_to_name(name), "Lab"))
				return base_device_lab;

			else if (!strcmp(pdf_to_name(name), "ICCBased"))
				return load_icc_based(xref, pdf_array_get(obj, 1));

			else if (!strcmp(pdf_to_name(name), "Indexed"))
				return load_indexed(xref, obj);
			else if (!strcmp(pdf_to_name(name), "I"))
				return load_indexed(xref, obj);

			else if (!strcmp(pdf_to_name(name), "Separation"))
				return load_separation(xref, obj);

			else if (!strcmp(pdf_to_name(name), "DeviceN"))
				return load_separation(xref, obj);

			else
				base_throw(xref->ctx, "syntaxerror: unknown colorspace %s", pdf_to_name(name));
		}
	}

	base_throw(xref->ctx, "syntaxerror: could not parse color space (%d %d R)", pdf_to_num(obj), pdf_to_gen(obj));
	return NULL; 
}

base_colorspace *
pdf_load_colorspace(pdf_document *xref, pdf_obj *obj)
{
	base_context *ctx = xref->ctx;
	base_colorspace *cs;

	if ((cs = pdf_find_item(ctx, base_free_colorspace_imp, obj)))
	{
		return cs;
	}

	cs = pdf_load_colorspace_imp(xref, obj);
	

	pdf_store_item(ctx, obj, cs, cs->size);

	return cs;
}
