#include "pdf-internal.h"

base_bitmap *
base_new_bitmap(base_context *ctx, int w, int h, int n)
{
	base_bitmap *bit;

	bit = base_malloc_struct(ctx, base_bitmap);
	bit->refs = 1;
	bit->w = w;
	bit->h = h;
	bit->n = n;
	
	bit->stride = ((n * w + 31) & ~31) >> 3;

	bit->samples = base_malloc_array(ctx, h, bit->stride);

	return bit;
}

base_bitmap *
base_keep_bitmap(base_context *ctx, base_bitmap *bit)
{
	if (bit)
		bit->refs++;
	return bit;
}

void
base_drop_bitmap(base_context *ctx, base_bitmap *bit)
{
	if (bit && --bit->refs == 0)
	{
		base_free(ctx, bit->samples);
		base_free(ctx, bit);
	}
}

void
base_clear_bitmap(base_context *ctx, base_bitmap *bit)
{
	memset(bit->samples, 0, bit->stride * bit->h);
}

void
base_write_pbm(base_context *ctx, base_bitmap *bitmap, char *filename)
{
	FILE *fp;
	unsigned char *p;
	int h, bytestride;

	fp = fopen(filename, "wb");
	if (!fp)
		base_throw(ctx, "cannot open file '%s': %s", filename, strerror(errno));

	assert(bitmap->n == 1);

	fprintf(fp, "P4\n%d %d\n", bitmap->w, bitmap->h);

	p = bitmap->samples;

	h = bitmap->h;
	bytestride = (bitmap->w + 7) >> 3;
	while (h--)
	{
		fwrite(p, 1, bytestride, fp);
		p += bitmap->stride;
	}

	fclose(fp);
}

base_colorspace *base_pixmap_colorspace(base_context *ctx, base_pixmap *pix)
{
	if (!pix)
		return NULL;
	return pix->colorspace;
}

int base_pixmap_components(base_context *ctx, base_pixmap *pix)
{
	if (!pix)
		return 0;
	return pix->n;
}

unsigned char *base_pixmap_samples(base_context *ctx, base_pixmap *pix)
{
	if (!pix)
		return NULL;
	return pix->samples;
}

void base_bitmap_details(base_bitmap *bit, int *w, int *h, int *n, int *stride)
{
	if (!bit)
	{
		if (w)
			*w = 0;
		if (h)
			*h = 0;
		if (n)
			*n = 0;
		if (stride)
			*stride = 0;
		return;
	}
	if (w)
		*w = bit->w;
	if (h)
		*h = bit->h;
	if (n)
		*n = bit->n;
	if (stride)
		*w = bit->stride;
}
