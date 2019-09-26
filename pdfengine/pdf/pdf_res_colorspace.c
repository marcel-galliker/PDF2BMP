#include "pdf-internal.h"

#define SLOWCMYK

void
base_free_colorspace_imp(base_context *ctx, base_storable *cs_)
{
	base_colorspace *cs = (base_colorspace *)cs_;

	if (cs->free_data && cs->data)
		cs->free_data(ctx, cs);
	base_free(ctx, cs);
}

base_colorspace *
base_new_colorspace(base_context *ctx, char *name, int n)
{
	base_colorspace *cs = base_malloc(ctx, sizeof(base_colorspace));
	base_INIT_STORABLE(cs, 1, base_free_colorspace_imp);
	cs->size = sizeof(base_colorspace);
	base_strlcpy(cs->name, name, sizeof cs->name);
	cs->n = n;
	cs->to_rgb = NULL;
	cs->from_rgb = NULL;
	cs->free_data = NULL;
	cs->data = NULL;
	return cs;
}

base_colorspace *
base_keep_colorspace(base_context *ctx, base_colorspace *cs)
{
	return (base_colorspace *)base_keep_storable(ctx, &cs->storable);
}

void
base_drop_colorspace(base_context *ctx, base_colorspace *cs)
{
	base_drop_storable(ctx, &cs->storable);
}

static void gray_to_rgb(base_context *ctx, base_colorspace *cs, float *gray, float *rgb)
{
	rgb[0] = gray[0];
	rgb[1] = gray[0];
	rgb[2] = gray[0];
}

static void rgb_to_gray(base_context *ctx, base_colorspace *cs, float *rgb, float *gray)
{
	float r = rgb[0];
	float g = rgb[1];
	float b = rgb[2];
	gray[0] = r * 0.3f + g * 0.59f + b * 0.11f;
}

static void rgb_to_rgb(base_context *ctx, base_colorspace *cs, float *rgb, float *xyz)
{
	xyz[0] = rgb[0];
	xyz[1] = rgb[1];
	xyz[2] = rgb[2];
}

static void bgr_to_rgb(base_context *ctx, base_colorspace *cs, float *bgr, float *rgb)
{
	rgb[0] = bgr[2];
	rgb[1] = bgr[1];
	rgb[2] = bgr[0];
}

static void rgb_to_bgr(base_context *ctx, base_colorspace *cs, float *rgb, float *bgr)
{
	bgr[0] = rgb[2];
	bgr[1] = rgb[1];
	bgr[2] = rgb[0];
}

static void cmyk_to_rgb(base_context *ctx, base_colorspace *cs, float *cmyk, float *rgb)
{
#ifdef SLOWCMYK 
	float c = cmyk[0], m = cmyk[1], y = cmyk[2], k = cmyk[3];
	float c1 = 1 - c, m1 = 1 - m, y1 = 1 - y, k1 = 1 - k;
	float r, g, b, x;

	
	x = c1 * m1 * y1 * k1;	
	r = g = b = x;
	x = c1 * m1 * y1 * k;	
	r += 0.1373 * x;
	g += 0.1216 * x;
	b += 0.1255 * x;
	x = c1 * m1 * y * k1;	
	r += x;
	g += 0.9490 * x;
	x = c1 * m1 * y * k;	
	r += 0.1098 * x;
	g += 0.1020 * x;
	x = c1 * m * y1 * k1;	
	r += 0.9255 * x;
	b += 0.5490 * x;
	x = c1 * m * y1 * k;	
	r += 0.1412 * x;
	x = c1 * m * y * k1;	
	r += 0.9294 * x;
	g += 0.1098 * x;
	b += 0.1412 * x;
	x = c1 * m * y * k;	
	r += 0.1333 * x;
	x = c * m1 * y1 * k1;	
	g += 0.6784 * x;
	b += 0.9373 * x;
	x = c * m1 * y1 * k;	
	g += 0.0588 * x;
	b += 0.1412 * x;
	x = c * m1 * y * k1;	
	g += 0.6510 * x;
	b += 0.3137 * x;
	x = c * m1 * y * k;	
	g += 0.0745 * x;
	x = c * m * y1 * k1;	
	r += 0.1804 * x;
	g += 0.1922 * x;
	b += 0.5725 * x;
	x = c * m * y1 * k;	
	b += 0.0078 * x;
	x = c * m * y * k1;	
	r += 0.2118 * x;
	g += 0.2119 * x;
	b += 0.2235 * x;

	rgb[0] = CLAMP(r, 0, 1);
	rgb[1] = CLAMP(g, 0, 1);
	rgb[2] = CLAMP(b, 0, 1);
#else
	rgb[0] = 1 - MIN(1, cmyk[0] + cmyk[3]);
	rgb[1] = 1 - MIN(1, cmyk[1] + cmyk[3]);
	rgb[2] = 1 - MIN(1, cmyk[2] + cmyk[3]);
#endif
}

static void rgb_to_cmyk(base_context *ctx, base_colorspace *cs, float *rgb, float *cmyk)
{
	float c, m, y, k;
	c = 1 - rgb[0];
	m = 1 - rgb[1];
	y = 1 - rgb[2];
	k = MIN(c, MIN(m, y));
	cmyk[0] = c - k;
	cmyk[1] = m - k;
	cmyk[2] = y - k;
	cmyk[3] = k;
}

static base_colorspace k_device_gray = { {-1, base_free_colorspace_imp}, 0, "DeviceGray", 1, gray_to_rgb, rgb_to_gray };
static base_colorspace k_device_rgb = { {-1, base_free_colorspace_imp}, 0, "DeviceRGB", 3, rgb_to_rgb, rgb_to_rgb };
static base_colorspace k_device_bgr = { {-1, base_free_colorspace_imp}, 0, "DeviceRGB", 3, bgr_to_rgb, rgb_to_bgr };
static base_colorspace k_device_cmyk = { {-1, base_free_colorspace_imp}, 0, "DeviceCMYK", 4, cmyk_to_rgb, rgb_to_cmyk };

base_colorspace *base_device_gray = &k_device_gray;
base_colorspace *base_device_rgb = &k_device_rgb;
base_colorspace *base_device_bgr = &k_device_bgr;
base_colorspace *base_device_cmyk = &k_device_cmyk;

base_colorspace *
base_find_device_colorspace(base_context *ctx, char *name)
{
	if (!strcmp(name, "DeviceGray"))
		return base_device_gray;
	if (!strcmp(name, "DeviceRGB"))
		return base_device_rgb;
	if (!strcmp(name, "DeviceBGR"))
		return base_device_bgr;
	if (!strcmp(name, "DeviceCMYK"))
		return base_device_cmyk;
	assert(!"unknown device colorspace");
	return NULL;
}

static void fast_gray_to_rgb(base_pixmap *dst, base_pixmap *src)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		d[0] = s[0];
		d[1] = s[0];
		d[2] = s[0];
		d[3] = s[1];
		s += 2;
		d += 4;
	}
}

static void fast_gray_to_cmyk(base_pixmap *dst, base_pixmap *src)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		d[0] = 0;
		d[1] = 0;
		d[2] = 0;
		d[3] = s[0];
		d[4] = s[1];
		s += 2;
		d += 5;
	}
}

static void fast_rgb_to_gray(base_pixmap *dst, base_pixmap *src)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		d[0] = ((s[0]+1) * 77 + (s[1]+1) * 150 + (s[2]+1) * 28) >> 8;
		d[1] = s[3];
		s += 4;
		d += 2;
	}
}

static void fast_bgr_to_gray(base_pixmap *dst, base_pixmap *src)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		d[0] = ((s[0]+1) * 28 + (s[1]+1) * 150 + (s[2]+1) * 77) >> 8;
		d[1] = s[3];
		s += 4;
		d += 2;
	}
}

static void fast_rgb_to_cmyk(base_pixmap *dst, base_pixmap *src)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		unsigned char c = 255 - s[0];
		unsigned char m = 255 - s[1];
		unsigned char y = 255 - s[2];
		unsigned char k = MIN(c, MIN(m, y));
		d[0] = c - k;
		d[1] = m - k;
		d[2] = y - k;
		d[3] = k;
		d[4] = s[3];
		s += 4;
		d += 5;
	}
}

static void fast_bgr_to_cmyk(base_pixmap *dst, base_pixmap *src)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		unsigned char c = 255 - s[2];
		unsigned char m = 255 - s[1];
		unsigned char y = 255 - s[0];
		unsigned char k = MIN(c, MIN(m, y));
		d[0] = c - k;
		d[1] = m - k;
		d[2] = y - k;
		d[3] = k;
		d[4] = s[3];
		s += 4;
		d += 5;
	}
}

static void fast_cmyk_to_gray(base_pixmap *dst, base_pixmap *src)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		unsigned char c = base_mul255(s[0], 77);
		unsigned char m = base_mul255(s[1], 150);
		unsigned char y = base_mul255(s[2], 28);
		d[0] = 255 - MIN(c + m + y + s[3], 255);
		d[1] = s[4];
		s += 5;
		d += 2;
	}
}

static void fast_cmyk_to_rgb(base_context *ctx, base_pixmap *dst, base_pixmap *src)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
#ifdef SLOWCMYK
		float cmyk[4], rgb[3];
		cmyk[0] = s[0] / 255.0f;
		cmyk[1] = s[1] / 255.0f;
		cmyk[2] = s[2] / 255.0f;
		cmyk[3] = s[3] / 255.0f;
		cmyk_to_rgb(ctx, NULL, cmyk, rgb);
		d[0] = rgb[0] * 255;
		d[1] = rgb[1] * 255;
		d[2] = rgb[2] * 255;
#else
		d[0] = 255 - MIN(s[0] + s[3], 255);
		d[1] = 255 - MIN(s[1] + s[3], 255);
		d[2] = 255 - MIN(s[2] + s[3], 255);
#endif
		d[3] = s[4];
		s += 5;
		d += 4;
	}
}

static void fast_cmyk_to_bgr(base_context *ctx, base_pixmap *dst, base_pixmap *src)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
#ifdef SLOWCMYK
		float cmyk[4], rgb[3];
		cmyk[0] = s[0] / 255.0f;
		cmyk[1] = s[1] / 255.0f;
		cmyk[2] = s[2] / 255.0f;
		cmyk[3] = s[3] / 255.0f;
		cmyk_to_rgb(ctx, NULL, cmyk, rgb);
		d[0] = rgb[2] * 255;
		d[1] = rgb[1] * 255;
		d[2] = rgb[0] * 255;
#else
		d[0] = 255 - MIN(s[2] + s[3], 255);
		d[1] = 255 - MIN(s[1] + s[3], 255);
		d[2] = 255 - MIN(s[0] + s[3], 255);
#endif
		d[3] = s[4];
		s += 5;
		d += 4;
	}
}

static void fast_rgb_to_bgr(base_pixmap *dst, base_pixmap *src)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		d[0] = s[2];
		d[1] = s[1];
		d[2] = s[0];
		d[3] = s[3];
		s += 4;
		d += 4;
	}
}

static void
base_std_conv_pixmap(base_context *ctx, base_pixmap *dst, base_pixmap *src)
{
	float srcv[base_MAX_COLORS];
	float dstv[base_MAX_COLORS];
	int srcn, dstn;
	int y, x, k, i;

	base_colorspace *ss = src->colorspace;
	base_colorspace *ds = dst->colorspace;

	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;

	assert(src->w == dst->w && src->h == dst->h);
	assert(src->n == ss->n + 1);
	assert(dst->n == ds->n + 1);

	srcn = ss->n;
	dstn = ds->n;

	
	if (!strcmp(ss->name, "Lab") && srcn == 3)
	{
		for (y = 0; y < src->h; y++)
		{
			for (x = 0; x < src->w; x++)
			{
				srcv[0] = *s++ / 255.0f * 100;
				srcv[1] = *s++ - 128;
				srcv[2] = *s++ - 128;

				base_convert_color(ctx, ds, dstv, ss, srcv);

				for (k = 0; k < dstn; k++)
					*d++ = dstv[k] * 255;

				*d++ = *s++;
			}
		}
	}

	
	else if (src->w * src->h < 256)
	{
		for (y = 0; y < src->h; y++)
		{
			for (x = 0; x < src->w; x++)
			{
				for (k = 0; k < srcn; k++)
					srcv[k] = *s++ / 255.0f;

				base_convert_color(ctx, ds, dstv, ss, srcv);

				for (k = 0; k < dstn; k++)
					*d++ = dstv[k] * 255;

				*d++ = *s++;
			}
		}
	}

	
	else if (srcn == 1)
	{
		unsigned char lookup[base_MAX_COLORS * 256];

		for (i = 0; i < 256; i++)
		{
			srcv[0] = i / 255.0f;
			base_convert_color(ctx, ds, dstv, ss, srcv);
			for (k = 0; k < dstn; k++)
				lookup[i * dstn + k] = dstv[k] * 255;
		}

		for (y = 0; y < src->h; y++)
		{
			for (x = 0; x < src->w; x++)
			{
				i = *s++;
				for (k = 0; k < dstn; k++)
					*d++ = lookup[i * dstn + k];
				*d++ = *s++;
			}
		}
	}

	
	else
	{
		base_hash_table *lookup;
		unsigned char *color;

		lookup = base_new_hash_table(ctx, 509, srcn, -1);

		for (y = 0; y < src->h; y++)
		{
			for (x = 0; x < src->w; x++)
			{
				color = base_hash_find(ctx, lookup, s);
				if (color)
				{
					memcpy(d, color, dstn);
					s += srcn;
					d += dstn;
					*d++ = *s++;
				}
				else
				{
					for (k = 0; k < srcn; k++)
						srcv[k] = *s++ / 255.0f;
					base_convert_color(ctx, ds, dstv, ss, srcv);
					for (k = 0; k < dstn; k++)
						*d++ = dstv[k] * 255;

					base_hash_insert(ctx, lookup, s - srcn, d - dstn);

					*d++ = *s++;
				}
			}
		}

		base_free_hash(ctx, lookup);
	}
}

/***************************************************************************************/
/* function name:	base_convert_pixmap
/* description:		performs the color conversion of pixmap
/* param 1:			pointer to the context
/* param 2:			destination pixmap
/* param 3:			source pixmap
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_convert_pixmap(base_context *ctx, base_pixmap *dp, base_pixmap *sp)
{
	base_colorspace *ss = sp->colorspace;
	base_colorspace *ds = dp->colorspace;

	assert(ss && ds);

	dp->interpolate = sp->interpolate;

	if (ss == base_device_gray)
	{
		if (ds == base_device_rgb) fast_gray_to_rgb(dp, sp);
		else if (ds == base_device_bgr) fast_gray_to_rgb(dp, sp); 
		else if (ds == base_device_cmyk) fast_gray_to_cmyk(dp, sp);
		else base_std_conv_pixmap(ctx, dp, sp);
	}

	else if (ss == base_device_rgb)
	{
		if (ds == base_device_gray) fast_rgb_to_gray(dp, sp);
		else if (ds == base_device_bgr) fast_rgb_to_bgr(dp, sp);
		else if (ds == base_device_cmyk) fast_rgb_to_cmyk(dp, sp);
		else base_std_conv_pixmap(ctx, dp, sp);
	}

	else if (ss == base_device_bgr)
	{
		if (ds == base_device_gray) fast_bgr_to_gray(dp, sp);
		else if (ds == base_device_rgb) fast_rgb_to_bgr(dp, sp); 
		else if (ds == base_device_cmyk) fast_bgr_to_cmyk(sp, dp);
		else base_std_conv_pixmap(ctx, dp, sp);
	}

	else if (ss == base_device_cmyk)
	{
		if (ds == base_device_gray) fast_cmyk_to_gray(dp, sp);
		else if (ds == base_device_bgr) fast_cmyk_to_bgr(ctx, dp, sp);
		else if (ds == base_device_rgb) fast_cmyk_to_rgb(ctx, dp, sp);
		else base_std_conv_pixmap(ctx, dp, sp);
	}

	else base_std_conv_pixmap(ctx, dp, sp);
}

static void
base_std_conv_color(base_context *ctx, base_colorspace *srcs, float *srcv, base_colorspace *dsts, float *dstv)
{
	float rgb[3];
	int i;

	if (srcs != dsts)
	{
		assert(srcs->to_rgb && dsts->from_rgb);
		srcs->to_rgb(ctx, srcs, srcv, rgb);
		dsts->from_rgb(ctx, dsts, rgb, dstv);
		for (i = 0; i < dsts->n; i++)
			dstv[i] = CLAMP(dstv[i], 0, 1);
	}
	else
	{
		for (i = 0; i < srcs->n; i++)
			dstv[i] = srcv[i];
	}
}

/***************************************************************************************/
/* function name:	base_convert_color
/* description:		performs the conversion of color
/* param 1:			pointer to the context
/* param 2:			destination colorspace
/* param 3:			destination value
/* param 4:			source colorspace
/* param 5:			source value
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_convert_color(base_context *ctx, base_colorspace *ds, float *dv, base_colorspace *ss, float *sv)
{
	if (ss == base_device_gray)
	{
		if ((ds == base_device_rgb) || (ds == base_device_bgr))
		{
			dv[0] = sv[0];
			dv[1] = sv[0];
			dv[2] = sv[0];
		}
		else if (ds == base_device_cmyk)
		{
			dv[0] = 0;
			dv[1] = 0;
			dv[2] = 0;
			dv[3] = sv[0];
		}
		else
			base_std_conv_color(ctx, ss, sv, ds, dv);
	}

	else if (ss == base_device_rgb)
	{
		if (ds == base_device_gray)
		{
			dv[0] = sv[0] * 0.3f + sv[1] * 0.59f + sv[2] * 0.11f;
		}
		else if (ds == base_device_bgr)
		{
			dv[0] = sv[2];
			dv[1] = sv[1];
			dv[2] = sv[0];
		}
		else if (ds == base_device_cmyk)
		{
			float c = 1 - sv[0];
			float m = 1 - sv[1];
			float y = 1 - sv[2];
			float k = MIN(c, MIN(m, y));
			dv[0] = c - k;
			dv[1] = m - k;
			dv[2] = y - k;
			dv[3] = k;
		}
		else
			base_std_conv_color(ctx, ss, sv, ds, dv);
	}

	else if (ss == base_device_bgr)
	{
		if (ds == base_device_gray)
		{
			dv[0] = sv[0] * 0.11f + sv[1] * 0.59f + sv[2] * 0.3f;
		}
		else if (ds == base_device_bgr)
		{
			dv[0] = sv[2];
			dv[1] = sv[1];
			dv[2] = sv[0];
		}
		else if (ds == base_device_cmyk)
		{
			float c = 1 - sv[2];
			float m = 1 - sv[1];
			float y = 1 - sv[0];
			float k = MIN(c, MIN(m, y));
			dv[0] = c - k;
			dv[1] = m - k;
			dv[2] = y - k;
			dv[3] = k;
		}
		else
			base_std_conv_color(ctx, ss, sv, ds, dv);
	}

	else if (ss == base_device_cmyk)
	{
		if (ds == base_device_gray)
		{
			float c = sv[0] * 0.3f;
			float m = sv[1] * 0.59f;
			float y = sv[2] * 0.11f;
			dv[0] = 1 - MIN(c + m + y + sv[3], 1);
		}
		else if (ds == base_device_rgb)
		{
#ifdef SLOWCMYK
			cmyk_to_rgb(ctx, NULL, sv, dv);
#else
			dv[0] = 1 - MIN(sv[0] + sv[3], 1);
			dv[1] = 1 - MIN(sv[1] + sv[3], 1);
			dv[2] = 1 - MIN(sv[2] + sv[3], 1);
#endif
		}
		else if (ds == base_device_bgr)
		{
#ifdef SLOWCMYK
			float rgb[3];
			cmyk_to_rgb(ctx, NULL, sv, rgb);
			dv[0] = rgb[2];
			dv[1] = rgb[1];
			dv[2] = rgb[0];
#else
			dv[0] = 1 - MIN(sv[2] + sv[3], 1);
			dv[1] = 1 - MIN(sv[1] + sv[3], 1);
			dv[2] = 1 - MIN(sv[0] + sv[3], 1);
#endif
		}
		else
			base_std_conv_color(ctx, ss, sv, ds, dv);
	}

	else
		base_std_conv_color(ctx, ss, sv, ds, dv);
}
