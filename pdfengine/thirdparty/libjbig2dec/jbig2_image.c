

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "os_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

#include "jbig2.h"
#include "jbig2_priv.h"
#include "jbig2_image.h"

Jbig2Image* jbig2_image_new(Jbig2Ctx *ctx, int width, int height)
{
	Jbig2Image	*image;
	int		stride;

	image = jbig2_new(ctx, Jbig2Image, 1);
	if (image == NULL) {
		jbig2_error(ctx, JBIG2_SEVERITY_FATAL, -1,
            "could not allocate image structure in jbig2_image_new");
		return NULL;
	}

	stride = ((width - 1) >> 3) + 1; 
	image->data = jbig2_new(ctx, uint8_t, stride*height);
	if (image->data == NULL) {
        jbig2_error(ctx, JBIG2_SEVERITY_FATAL, -1,
            "could not allocate image data buffer! [%d bytes]\n", stride*height);
		jbig2_free(ctx->allocator, image);
		return NULL;
	}

	image->width = width;
	image->height = height;
	image->stride = stride;
	image->refcount = 1;

	return image;
}

Jbig2Image* jbig2_image_clone(Jbig2Ctx *ctx, Jbig2Image *image)
{
	if (image)
		image->refcount++;
	return image;
}

void jbig2_image_release(Jbig2Ctx *ctx, Jbig2Image *image)
{
	if (image == NULL)
		return;
	image->refcount--;
	if (!image->refcount) jbig2_image_free(ctx, image);
}

void jbig2_image_free(Jbig2Ctx *ctx, Jbig2Image *image)
{
	if (image)
		jbig2_free(ctx->allocator, image->data);
	jbig2_free(ctx->allocator, image);
}

Jbig2Image *jbig2_image_resize(Jbig2Ctx *ctx, Jbig2Image *image,
				int width, int height)
{
	if (width == image->width) {
	    
	    image->data = jbig2_renew(ctx, image->data, uint8_t, image->stride*height);
            if (image->data == NULL) {
                jbig2_error(ctx, JBIG2_SEVERITY_FATAL, -1,
                    "could not resize image buffer!");
		return NULL;
            }
	    if (height > image->height) {
		memset(image->data + image->height*image->stride,
			0, (height - image->height)*image->stride);
	    }
            image->height = height;

	} else {
	    
	    jbig2_error(ctx, JBIG2_SEVERITY_WARNING, -1,
		"jbig2_image_resize called with a different width (NYI)");
	}

	return NULL;
}

int jbig2_image_compose_unopt(Jbig2Ctx *ctx,
			Jbig2Image *dst, Jbig2Image *src,
                        int x, int y, Jbig2ComposeOp op)
{
    int i, j;
    int sw = src->width;
    int sh = src->height;
    int sx = 0;
    int sy = 0;

    
    if (x < 0) { sx += -x; sw -= -x; x = 0; }
    if (y < 0) { sy += -y; sh -= -y; y = 0; }
    if (x + sw >= dst->width) sw = dst->width - x;
    if (y + sh >= dst->height) sh = dst->height - y;

    switch (op) {
	case JBIG2_COMPOSE_OR:
	    for (j = 0; j < sh; j++) {
		for (i = 0; i < sw; i++) {
		    jbig2_image_set_pixel(dst, i+x, j+y,
			jbig2_image_get_pixel(src, i+sx, j+sy) |
			jbig2_image_get_pixel(dst, i+x, j+y));
		}
    	    }
	    break;
	case JBIG2_COMPOSE_AND:
	    for (j = 0; j < sh; j++) {
		for (i = 0; i < sw; i++) {
		    jbig2_image_set_pixel(dst, i+x, j+y,
			jbig2_image_get_pixel(src, i+sx, j+sy) &
			jbig2_image_get_pixel(dst, i+x, j+y));
		}
    	    }
	    break;
	case JBIG2_COMPOSE_XOR:
	    for (j = 0; j < sh; j++) {
		for (i = 0; i < sw; i++) {
		    jbig2_image_set_pixel(dst, i+x, j+y,
			jbig2_image_get_pixel(src, i+sx, j+sy) ^
			jbig2_image_get_pixel(dst, i+x, j+y));
		}
    	    }
	    break;
	case JBIG2_COMPOSE_XNOR:
	    for (j = 0; j < sh; j++) {
		for (i = 0; i < sw; i++) {
		    jbig2_image_set_pixel(dst, i+x, j+y,
			~(jbig2_image_get_pixel(src, i+sx, j+sy) ^
			jbig2_image_get_pixel(dst, i+x, j+y)));
		}
    	    }
	    break;
	case JBIG2_COMPOSE_REPLACE:
	    for (j = 0; j < sh; j++) {
		for (i = 0; i < sw; i++) {
		    jbig2_image_set_pixel(dst, i+x, j+y,
			jbig2_image_get_pixel(src, i+sx, j+sy));
		}
    	    }
	    break;
    }

    return 0;
}

int jbig2_image_compose(Jbig2Ctx *ctx, Jbig2Image *dst, Jbig2Image *src,
			int x, int y, Jbig2ComposeOp op)
{
    int i, j;
    int w, h;
    int leftbyte, rightbyte;
    int shift;
    uint8_t *s, *ss;
    uint8_t *d, *dd;
    uint8_t mask, rightmask;

    if (op != JBIG2_COMPOSE_OR) {
	
	return jbig2_image_compose_unopt(ctx, dst, src, x, y, op);
    }

    
    w = src->width;
    h = src->height;
    ss = src->data;

    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    w = (x + w < dst->width) ? w : dst->width - x;
    h = (y + h < dst->height) ? h : dst->height - y;
#ifdef JBIG2_DEBUG
    jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, -1,
      "compositing %dx%d at (%d, %d) after clipping\n",
        w, h, x, y);
#endif

    
    if ((w <= 0) || (h <= 0))
    {
#ifdef JBIG2_DEBUG
        jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, -1, "zero clipping region");
#endif
        return 0;
    }

#if 0
    
    
    if ((x == 0) && (w == src->width)) {
        memcpy(dst->data + y*dst->stride, src->data, h*src->stride);
        return 0;
    }
#endif

    leftbyte = x >> 3;
    rightbyte = (x + w - 1) >> 3;
    shift = x & 7;

    
    s = ss;
    d = dd = dst->data + y*dst->stride + leftbyte;
    if (leftbyte == rightbyte) {
	mask = 0x100 - (0x100 >> w);
        for (j = 0; j < h; j++) {
            *d |= (*s & mask) >> shift;
            d += dst->stride;
            s += src->stride;
        }
    } else if (shift == 0) {
	rightmask = (w & 7) ? 0x100 - (1 << (8 - (w & 7))) : 0xFF;
        for (j = 0; j < h; j++) {
	    for (i = leftbyte; i < rightbyte; i++)
		*d++ |= *s++;
	    *d |= *s & rightmask;
            d = (dd += dst->stride);
            s = (ss += src->stride);
	}
    } else {
	bool overlap = (((w + 7) >> 3) < ((x + w + 7) >> 3) - (x >> 3));
	mask = 0x100 - (1 << shift);
	if (overlap)
	    rightmask = (0x100 - (0x100 >> ((x + w) & 7))) >> (8 - shift);
	else
	    rightmask = 0x100 - (0x100 >> (w & 7));
        for (j = 0; j < h; j++) {
	    *d++ |= (*s & mask) >> shift;
            for(i = leftbyte; i < rightbyte - 1; i++) {
		*d |= ((*s++ & ~mask) << (8 - shift));
		*d++ |= ((*s & mask) >> shift);
	    }
	    if (overlap)
		*d |= (*s & rightmask) << (8 - shift);
	    else
		*d |= ((s[0] & ~mask) << (8 - shift)) |
		    ((s[1] & rightmask) >> shift);
	    d = (dd += dst->stride);
	    s = (ss += src->stride);
	}
    }

    return 0;
}

void jbig2_image_clear(Jbig2Ctx *ctx, Jbig2Image *image, int value)
{
    const uint8_t fill = value ? 0xFF : 0x00;

    memset(image->data, fill, image->stride*image->height);
}

int jbig2_image_get_pixel(Jbig2Image *image, int x, int y)
{
  const int w = image->width;
  const int h = image->height;
  const int byte = (x >> 3) + y*image->stride;
  const int bit = 7 - (x & 7);

  if ((x < 0) || (x >= w)) return 0;
  if ((y < 0) || (y >= h)) return 0;

  return ((image->data[byte]>>bit) & 1);
}

int jbig2_image_set_pixel(Jbig2Image *image, int x, int y, bool value)
{
  const int w = image->width;
  const int h = image->height;
  int scratch, mask;
  int bit, byte;

  if ((x < 0) || (x >= w)) return 0;
  if ((y < 0) || (y >= h)) return 0;

  byte = (x >> 3) + y*image->stride;
  bit = 7 - (x & 7);
  mask = (1 << bit) ^ 0xff;

  scratch = image->data[byte] & mask;
  image->data[byte] = scratch | (value << bit);

  return 1;
}
