#include "pdf-internal.h"

base_pixmap *
base_keep_pixmap(base_context *ctx, base_pixmap *pix)
{
	return (base_pixmap *)base_keep_storable(ctx, &pix->storable);
}

/***************************************************************************************/
/* function name:	base_drop_pixmap
/* description:		free pixmap
/* param 1:			pointer to the context
/* param 2:			pixmap to free
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_drop_pixmap(base_context *ctx, base_pixmap *pix)
{
	base_drop_storable(ctx, &pix->storable);
}

void
base_free_pixmap_imp(base_context *ctx, base_storable *pix_)
{
	base_pixmap *pix = (base_pixmap *)pix_;

	if (pix->colorspace)
		base_drop_colorspace(ctx, pix->colorspace);
	if (pix->free_samples)
		base_free(ctx, pix->samples);
	base_free(ctx, pix);
}

base_pixmap *
base_new_pixmap_with_data(base_context *ctx, base_colorspace *colorspace, int w, int h, unsigned char *samples)
{
	base_pixmap *pix;

	pix = base_malloc_struct(ctx, base_pixmap);
	base_INIT_STORABLE(pix, 1, base_free_pixmap_imp);
	pix->x = 0;
	pix->y = 0;
	pix->w = w;
	pix->h = h;
	pix->interpolate = 1;
	pix->xres = 96;
	pix->yres = 96;
	pix->colorspace = NULL;
	pix->n = 1;

	if (colorspace)
	{
		pix->colorspace = base_keep_colorspace(ctx, colorspace);
		pix->n = 1 + colorspace->n;
	}

	pix->samples = samples;
	if (samples)
	{
		pix->free_samples = 0;
	}
	else
	{
		base_try(ctx)
		{
			if (pix->w + pix->n - 1 > INT_MAX / pix->n)
				base_throw(ctx, "overly wide image");
			pix->samples = base_malloc_array(ctx, pix->h, pix->w * pix->n);
		}
		base_catch(ctx)
		{
			if (colorspace)
				base_drop_colorspace(ctx, colorspace);
			base_free(ctx, pix);
			base_rethrow(ctx);
		}
		pix->free_samples = 1;
	}

	return pix;
}

base_pixmap *
base_new_pixmap(base_context *ctx, base_colorspace *colorspace, int w, int h)
{
	return base_new_pixmap_with_data(ctx, colorspace, w, h, NULL);
}

/***************************************************************************************/
/* function name:	base_new_pixmap_with_bbox
/* description:		create new pixmap of specified size
/* param 1:			pointer to the context
/* param 2:			colorspace of pixmap to create with
/* param 3:			bound box area of pixmap to create
/* return:			value to fill with
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

base_pixmap *
base_new_pixmap_with_bbox(base_context *ctx, base_colorspace *colorspace, base_bbox r)
{
	base_pixmap *pixmap;
	pixmap = base_new_pixmap(ctx, colorspace, r.x1 - r.x0, r.y1 - r.y0);
	pixmap->x = r.x0;
	pixmap->y = r.y0;
	return pixmap;
}

base_pixmap *
base_new_pixmap_with_bbox_and_data(base_context *ctx, base_colorspace *colorspace, base_bbox r, unsigned char *samples)
{
	base_pixmap *pixmap;
	pixmap = base_new_pixmap_with_data(ctx, colorspace, r.x1 - r.x0, r.y1 - r.y0, samples);
	pixmap->x = r.x0;
	pixmap->y = r.y0;
	return pixmap;
}

/***************************************************************************************/
/* function name:	base_pixmap_bbox
/* description:		get the size of pixmap
/* param 1:			pointer to the context
/* param 2:			pinter to the pixmap
/* return:			size of pixmap
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

base_bbox
base_pixmap_bbox(base_context *ctx, base_pixmap *pix)
{
	base_bbox bbox;
	bbox.x0 = pix->x;
	bbox.y0 = pix->y;
	bbox.x1 = pix->x + pix->w;
	bbox.y1 = pix->y + pix->h;
	return bbox;
}

base_bbox
base_pixmap_bbox_no_ctx(base_pixmap *pix)
{
	base_bbox bbox;
	bbox.x0 = pix->x;
	bbox.y0 = pix->y;
	bbox.x1 = pix->x + pix->w;
	bbox.y1 = pix->y + pix->h;
	return bbox;
}

/***************************************************************************************/
/* function name:	base_pixmap_width
/* description:		get the width of pixmap
/* param 1:			pointer to the context
/* param 2:			pixmap to get width
/* return:			width of the pixmap
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

int
base_pixmap_width(base_context *ctx, base_pixmap *pix)
{
	return pix->w;
}

/***************************************************************************************/
/* function name:	base_pixmap_height
/* description:		get the height of pixmap
/* param 1:			pointer to the context
/* param 2:			pixmap to get height
/* return:			height of the pixmap
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

int
base_pixmap_height(base_context *ctx, base_pixmap *pix)
{
	return pix->h;
}

void
base_clear_pixmap(base_context *ctx, base_pixmap *pix)
{
	memset(pix->samples, 0, pix->w * pix->h * pix->n);
}

/***************************************************************************************/
/* function name:	base_clear_pixmap_with_value
/* description:		fill the pixmap with specified value
/* param 1:			pointer to the context
/* param 2:			pixmap to fill
/* return:			value to fill with
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_clear_pixmap_with_value(base_context *ctx, base_pixmap *pix, int value)
{
	if (value == 255)
		memset(pix->samples, 255, pix->w * pix->h * pix->n);
	else
	{
		int k, x, y;
		unsigned char *s = pix->samples;
		for (y = 0; y < pix->h; y++)
		{
			for (x = 0; x < pix->w; x++)
			{
				for (k = 0; k < pix->n - 1; k++)
					*s++ = value;
				*s++ = 255;
			}
		}
	}
}

void
base_copy_pixmap_rect(base_context *ctx, base_pixmap *dest, base_pixmap *src, base_bbox r)
{
	const unsigned char *srcp;
	unsigned char *destp;
	int y, w, destspan, srcspan;

	r = base_intersect_bbox(r, base_pixmap_bbox(ctx, dest));
	r = base_intersect_bbox(r, base_pixmap_bbox(ctx, src));
	w = r.x1 - r.x0;
	y = r.y1 - r.y0;
	if (w <= 0 || y <= 0)
		return;

	w *= src->n;
	srcspan = src->w * src->n;
	srcp = src->samples + srcspan * (r.y0 - src->y) + src->n * (r.x0 - src->x);
	destspan = dest->w * dest->n;
	destp = dest->samples + destspan * (r.y0 - dest->y) + dest->n * (r.x0 - dest->x);
	do
	{
		memcpy(destp, srcp, w);
		srcp += srcspan;
		destp += destspan;
	}
	while (--y);
}

void
base_clear_pixmap_rect_with_value(base_context *ctx, base_pixmap *dest, int value, base_bbox r)
{
	unsigned char *destp;
	int x, y, w, k, destspan;

	r = base_intersect_bbox(r, base_pixmap_bbox(ctx, dest));
	w = r.x1 - r.x0;
	y = r.y1 - r.y0;
	if (w <= 0 || y <= 0)
		return;

	destspan = dest->w * dest->n;
	destp = dest->samples + destspan * (r.y0 - dest->y) + dest->n * (r.x0 - dest->x);
	if (value == 255)
		do
		{
			memset(destp, 255, w * dest->n);
			destp += destspan;
		}
		while (--y);
	else
		do
		{
			unsigned char *s = destp;
			for (x = 0; x < w; x++)
			{
				for (k = 0; k < dest->n - 1; k++)
					*s++ = value;
				*s++ = 255;
			}
			destp += destspan;
		}
		while (--y);
}

void
base_premultiply_pixmap(base_context *ctx, base_pixmap *pix)
{
	unsigned char *s = pix->samples;
	unsigned char a;
	int k, x, y;

	for (y = 0; y < pix->h; y++)
	{
		for (x = 0; x < pix->w; x++)
		{
			a = s[pix->n - 1];
			for (k = 0; k < pix->n - 1; k++)
				s[k] = base_mul255(s[k], a);
			s += pix->n;
		}
	}
}

void
base_unmultiply_pixmap(base_context *ctx, base_pixmap *pix)
{
	unsigned char *s = pix->samples;
	int a, inva;
	int k, x, y;

	for (y = 0; y < pix->h; y++)
	{
		for (x = 0; x < pix->w; x++)
		{
			a = s[pix->n - 1];
			inva = a ? 255 * 256 / a : 0;
			for (k = 0; k < pix->n - 1; k++)
				s[k] = (s[k] * inva) >> 8;
			s += pix->n;
		}
	}
}

base_pixmap *
base_alpha_from_gray(base_context *ctx, base_pixmap *gray, int luminosity)
{
	base_pixmap *alpha;
	unsigned char *sp, *dp;
	int len;

	assert(gray->n == 2);

	alpha = base_new_pixmap_with_bbox(ctx, NULL, base_pixmap_bbox(ctx, gray));
	dp = alpha->samples;
	sp = gray->samples;
	if (!luminosity)
		sp ++;

	len = gray->w * gray->h;
	while (len--)
	{
		*dp++ = sp[0];
		sp += 2;
	}

	return alpha;
}

void
base_invert_pixmap(base_context *ctx, base_pixmap *pix)
{
	unsigned char *s = pix->samples;
	int k, x, y;

	for (y = 0; y < pix->h; y++)
	{
		for (x = 0; x < pix->w; x++)
		{
			for (k = 0; k < pix->n - 1; k++)
				s[k] = 255 - s[k];
			s += pix->n;
		}
	}
}

void base_invert_pixmap_rect(base_pixmap *image, base_bbox rect)
{
	unsigned char *p;
	int x, y, n;

	int x0 = CLAMP(rect.x0 - image->x, 0, image->w - 1);
	int x1 = CLAMP(rect.x1 - image->x, 0, image->w - 1);
	int y0 = CLAMP(rect.y0 - image->y, 0, image->h - 1);
	int y1 = CLAMP(rect.y1 - image->y, 0, image->h - 1);

	for (y = y0; y < y1; y++)
	{
		p = image->samples + (y * image->w + x0) * image->n;
		for (x = x0; x < x1; x++)
		{
			for (n = image->n; n > 0; n--, p++)
				*p = 255 - *p;
		}
	}
}

void
base_gamma_pixmap(base_context *ctx, base_pixmap *pix, float gamma)
{
	unsigned char gamma_map[256];
	unsigned char *s = pix->samples;
	int k, x, y;

	for (k = 0; k < 256; k++)
		gamma_map[k] = pow(k / 255.0f, gamma) * 255;

	for (y = 0; y < pix->h; y++)
	{
		for (x = 0; x < pix->w; x++)
		{
			for (k = 0; k < pix->n - 1; k++)
				s[k] = gamma_map[s[k]];
			s += pix->n;
		}
	}
}

void
base_write_pnm(base_context *ctx, base_pixmap *pixmap, char *filename)
{
	FILE *fp;
	unsigned char *p;
	int len;

	if (pixmap->n != 1 && pixmap->n != 2 && pixmap->n != 4)
		base_throw(ctx, "pixmap must be grayscale or rgb to write as pnm");

	fp = fopen(filename, "wb");
	if (!fp)
		base_throw(ctx, "cannot open file '%s': %s", filename, strerror(errno));

	if (pixmap->n == 1 || pixmap->n == 2)
		fprintf(fp, "P5\n");
	if (pixmap->n == 4)
		fprintf(fp, "P6\n");
	fprintf(fp, "%d %d\n", pixmap->w, pixmap->h);
	fprintf(fp, "255\n");

	len = pixmap->w * pixmap->h;
	p = pixmap->samples;

	switch (pixmap->n)
	{
	case 1:
		fwrite(p, 1, len, fp);
		break;
	case 2:
		while (len--)
		{
			putc(p[0], fp);
			p += 2;
		}
		break;
	case 4:
		while (len--)
		{
			putc(p[0], fp);
			putc(p[1], fp);
			putc(p[2], fp);
			p += 4;
		}
	}

	fclose(fp);
}

void
base_write_pam(base_context *ctx, base_pixmap *pixmap, char *filename, int savealpha)
{
	unsigned char *sp;
	int y, w, k;
	FILE *fp;

	int sn = pixmap->n;
	int dn = pixmap->n;
	if (!savealpha && dn > 1)
		dn--;

	fp = fopen(filename, "wb");
	if (!fp)
		base_throw(ctx, "cannot open file '%s': %s", filename, strerror(errno));

	fprintf(fp, "P7\n");
	fprintf(fp, "WIDTH %d\n", pixmap->w);
	fprintf(fp, "HEIGHT %d\n", pixmap->h);
	fprintf(fp, "DEPTH %d\n", dn);
	fprintf(fp, "MAXVAL 255\n");
	if (pixmap->colorspace)
		fprintf(fp, "# COLORSPACE %s\n", pixmap->colorspace->name);
	switch (dn)
	{
	case 1: fprintf(fp, "TUPLTYPE GRAYSCALE\n"); break;
	case 2: if (sn == 2) fprintf(fp, "TUPLTYPE GRAYSCALE_ALPHA\n"); break;
	case 3: if (sn == 4) fprintf(fp, "TUPLTYPE RGB\n"); break;
	case 4: if (sn == 4) fprintf(fp, "TUPLTYPE RGB_ALPHA\n"); break;
	}
	fprintf(fp, "ENDHDR\n");

	sp = pixmap->samples;
	for (y = 0; y < pixmap->h; y++)
	{
		w = pixmap->w;
		while (w--)
		{
			for (k = 0; k < dn; k++)
				putc(sp[k], fp);
			sp += sn;
		}
	}

	fclose(fp);
}

#include <zlib.h>

static inline void big32(unsigned char *buf, unsigned int v)
{
	buf[0] = (v >> 24) & 0xff;
	buf[1] = (v >> 16) & 0xff;
	buf[2] = (v >> 8) & 0xff;
	buf[3] = (v) & 0xff;
}

static inline void put32(unsigned int v, FILE *fp)
{
	putc(v >> 24, fp);
	putc(v >> 16, fp);
	putc(v >> 8, fp);
	putc(v, fp);
}

static void putchunk(char *tag, unsigned char *data, int size, FILE *fp)
{
	unsigned int sum;
	put32(size, fp);
	fwrite(tag, 1, 4, fp);
	fwrite(data, 1, size, fp);
	sum = crc32(0, NULL, 0);
	sum = crc32(sum, (unsigned char*)tag, 4);
	sum = crc32(sum, data, size);
	put32(sum, fp);
}

/***************************************************************************************/
/* function name:	base_write_png
/* description:		save pixmap as jpeg file
/* param 1:			pointer to the context
/* param 2:			pixmap to save
/* param 3:			file path where to save
/* param 4:			alpha value to save with
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_write_png(base_context *ctx, base_pixmap *pixmap, char *filename, int savealpha)
{
	static const unsigned char pngsig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	FILE *fp;
	unsigned char head[13];
	unsigned char *udata = NULL;
	unsigned char *cdata = NULL;
	unsigned char *sp, *dp;
	uLong usize, csize;
	int y, x, k, sn, dn;
	int color;
	int err;

	base_var(udata);
	base_var(cdata);

	if (pixmap->n != 1 && pixmap->n != 2 && pixmap->n != 4)
		base_throw(ctx, "pixmap must be grayscale or rgb to write as png");

	sn = pixmap->n;
	dn = pixmap->n;
	if (!savealpha && dn > 1)
		dn--;

	switch (dn)
	{
	default:
	case 1: color = 0; break;
	case 2: color = 4; break;
	case 3: color = 2; break;
	case 4: color = 6; break;
	}

	usize = (pixmap->w * dn + 1) * pixmap->h;
	csize = compressBound(usize);
	base_try(ctx)
	{
		udata = base_malloc(ctx, usize);
		cdata = base_malloc(ctx, csize);
	}
	base_catch(ctx)
	{
		base_free(ctx, udata);
		base_rethrow(ctx);
	}

	sp = pixmap->samples;
	dp = udata;
	for (y = 0; y < pixmap->h; y++)
	{
		*dp++ = 1; 
		for (x = 0; x < pixmap->w; x++)
		{
			for (k = 0; k < dn; k++)
			{
				if (x == 0)
					dp[k] = sp[k];
				else
					dp[k] = sp[k] - sp[k-sn];
			}
			sp += sn;
			dp += dn;
		}
	}

	err = compress(cdata, &csize, udata, usize);
	if (err != Z_OK)
	{
		base_free(ctx, udata);
		base_free(ctx, cdata);
		base_throw(ctx, "cannot compress image data");
	}

	fp = fopen(filename, "wb");

	if (!fp)
	{
		base_free(ctx, udata);
		base_free(ctx, cdata);
		base_throw(ctx, "cannot open file '%s': %s", filename, strerror(errno));
	}

	big32(head+0, pixmap->w);
	big32(head+4, pixmap->h);
	head[8] = 8; 
	head[9] = color;
	head[10] = 0; 
	head[11] = 0; 
	head[12] = 0; 

	fwrite(pngsig, 1, 8, fp);
	putchunk("IHDR", head, 13, fp);
	putchunk("IDAT", cdata, csize, fp);
	putchunk("IEND", head, 0, fp);
	fclose(fp);

	base_free(ctx, udata);
	base_free(ctx, cdata);
}

unsigned int
base_pixmap_size(base_context *ctx, base_pixmap * pix)
{
	if (pix == NULL)
		return 0;
	return sizeof(*pix) + pix->n * pix->w * pix->h;
}

base_pixmap *
base_image_to_pixmap(base_context *ctx, base_image *image, int w, int h)
{
	if (image == NULL)
		return NULL;
	return image->get_pixmap(ctx, image, w, h);
}

base_image *
base_keep_image(base_context *ctx, base_image *image)
{
	return (base_image *)base_keep_storable(ctx, &image->storable);
}

void
base_drop_image(base_context *ctx, base_image *image)
{
	base_drop_storable(ctx, &image->storable);
}
