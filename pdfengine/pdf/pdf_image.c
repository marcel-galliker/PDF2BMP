#include "pdf-internal.h"
#include "pdfengine-internal.h"

typedef struct pdf_image_key_s pdf_image_key;

struct pdf_image_key_s {
	int refs;
	base_image *image;
	int factor;
};

static void pdf_load_jpx(pdf_document *xref, pdf_obj *dict, pdf_image *image);

static void
pdf_mask_color_key(base_pixmap *pix, int n, int *colorkey)
{
	unsigned char *p = pix->samples;
	int len = pix->w * pix->h;
	int k, t;
	while (len--)
	{
		t = 1;
		for (k = 0; k < n; k++)
			if (p[k] < colorkey[k * 2] || p[k] > colorkey[k * 2 + 1])
				t = 0;
		if (t)
			for (k = 0; k < pix->n; k++)
				p[k] = 0;
		p += pix->n;
	}
}

static int
pdf_make_hash_image_key(base_store_hash *hash, void *key_)
{
	pdf_image_key *key = (pdf_image_key *)key_;

	hash->u.pi.ptr = key->image;
	hash->u.pi.i = key->factor;
	return 1;
}

static void *
pdf_keep_image_key(base_context *ctx, void *key_)
{
	pdf_image_key *key = (pdf_image_key *)key_;

	base_lock(ctx, base_LOCK_ALLOC);
	key->refs++;
	base_unlock(ctx, base_LOCK_ALLOC);

	return (void *)key;
}

static void
pdf_drop_image_key(base_context *ctx, void *key_)
{
	pdf_image_key *key = (pdf_image_key *)key_;
	int drop;

	base_lock(ctx, base_LOCK_ALLOC);
	drop = --key->refs;
	base_unlock(ctx, base_LOCK_ALLOC);
	if (drop == 0)
	{
		base_drop_image(ctx, key->image);
		base_free(ctx, key);
	}
}

static int
pdf_cmp_image_key(void *k0_, void *k1_)
{
	pdf_image_key *k0 = (pdf_image_key *)k0_;
	pdf_image_key *k1 = (pdf_image_key *)k1_;

	return k0->image == k1->image && k0->factor == k1->factor;
}

static void
pdf_debug_image(void *key_)
{
	pdf_image_key *key = (pdf_image_key *)key_;

	printf("(image %d x %d sf=%d) ", key->image->w, key->image->h, key->factor);
}

static base_store_type pdf_image_store_type =
{
	pdf_make_hash_image_key,
	pdf_keep_image_key,
	pdf_drop_image_key,
	pdf_cmp_image_key,
	pdf_debug_image
};

static base_pixmap *
decomp_image_from_stream(base_context *ctx, base_stream *stm, pdf_image *image, int in_line, int indexed, int factor)
{
	base_pixmap *tile = NULL;
	base_pixmap *existing_tile;
	int stride, len, i;
	unsigned char *samples = NULL;
	int w = (image->base.w + (factor-1)) / factor;
	int h = (image->base.h + (factor-1)) / factor;
	pdf_image_key *key;

	base_var(tile);
	base_var(samples);

	base_try(ctx)
	{
		tile = base_new_pixmap(ctx, image->base.colorspace, w, h);
		tile->interpolate = image->interpolate;

		stride = (w * image->n * image->bpc + 7) / 8;

		samples = base_malloc_array(ctx, h, stride);

		len = base_read(stm, samples, h * stride);
		if (len < 0)
		{
			base_throw(ctx, "cannot read image data");
		}

		
		if (in_line)
		{
			unsigned char tbuf[512];
			base_try(ctx)
			{
				int tlen = base_read(stm, tbuf, sizeof tbuf);
				if (tlen > 0)
					base_warn(ctx, "ignoring garbage at end of image");
			}
			base_catch(ctx)
			{
				base_warn(ctx, "ignoring error at end of image");
			}
		}

		
		if (len < stride * h)
		{
			base_warn(ctx, "padding truncated image");
			memset(samples + len, 0, stride * h - len);
		}

		
		if (image->imagemask)
		{
			
			unsigned char *p = samples;
			len = h * stride;
			for (i = 0; i < len; i++)
				p[i] = ~p[i];
		}

		base_unpack_tile(tile, samples, image->n, image->bpc, stride, indexed);

		base_free(ctx, samples);
		samples = NULL;

		if (image->usecolorkey)
			pdf_mask_color_key(tile, image->n, image->colorkey);

		if (indexed)
		{
			base_pixmap *conv;
			base_decode_indexed_tile(tile, image->decode, (1 << image->bpc) - 1);
			conv = pdf_expand_indexed_pixmap(ctx, tile);
			base_drop_pixmap(ctx, tile);
			tile = conv;
		}
		else
		{
			base_decode_tile(tile, image->decode);
		}
	}
	base_always(ctx)
	{
		base_close(stm);
	}
	base_catch(ctx)
	{
		if (tile)
			base_drop_pixmap(ctx, tile);
		base_free(ctx, samples);

		base_rethrow(ctx);
	}

	
	base_try(ctx)
	{
		key = base_malloc_struct(ctx, pdf_image_key);
		key->refs = 1;
		key->image = base_keep_image(ctx, &image->base);
		key->factor = factor;
		existing_tile = base_store_item(ctx, key, tile, base_pixmap_size(ctx, tile), &pdf_image_store_type);
		if (existing_tile)
		{
			
			base_drop_pixmap(ctx, tile);
			tile = existing_tile;
		}
	}
	base_always(ctx)
	{
		pdf_drop_image_key(ctx, key);
	}
	base_catch(ctx)
	{
		
	}

	return tile;
}

static void
pdf_free_image(base_context *ctx, base_storable *image_)
{
	pdf_image *image = (pdf_image *)image_;

	if (image == NULL)
		return;
	base_drop_pixmap(ctx, image->tile);
	base_drop_buffer(ctx, image->buffer);
	base_drop_colorspace(ctx, image->base.colorspace);
	base_drop_image(ctx, image->base.mask);
	base_free(ctx, image);
}

static base_pixmap *
pdf_image_get_pixmap(base_context *ctx, base_image *image_, int w, int h)
{
	pdf_image *image = (pdf_image *)image_;
	base_pixmap *tile;
	base_stream *stm;
	int factor;
	pdf_image_key key;

	
	if (image->buffer == NULL)
	{
		tile = image->tile;
		if (!tile)
			return NULL;
		return base_keep_pixmap(ctx, tile); 
	}

	
	if (w > image->base.w)
		w = image->base.w;
	if (h > image->base.h)
		h = image->base.h;

	
	if (w == 0 || h == 0)
		factor = 1;
	else
		for (factor=1; image->base.w/(2*factor) >= w && image->base.h/(2*factor) >= h && factor < 8; factor *= 2);

	
	key.refs = 1;
	key.image = &image->base;
	key.factor = factor;
	do
	{
		tile = base_find_item(ctx, base_free_pixmap_imp, &key, &pdf_image_store_type);
		if (tile)
			return tile;
		key.factor >>= 1;
	}
	while (key.factor > 0);

	
	stm = pdf_open_image_decomp_stream(ctx, image->buffer, &image->params, &factor);

	return decomp_image_from_stream(ctx, stm, image, 0, 0, factor);
}

static pdf_image *
pdf_load_image_imp(pdf_document *xref, pdf_obj *rdb, pdf_obj *dict, base_stream *cstm, int forcemask)
{
	base_stream *stm = NULL;
	pdf_image *image = NULL;
	pdf_obj *obj, *res;

	int w, h, bpc, n;
	int imagemask;
	int interpolate;
	int indexed;
	base_image *mask = NULL; 
	int usecolorkey;

	int i;
	base_context *ctx = xref->ctx;

	base_var(stm);
	base_var(mask);

	image = base_malloc_struct(ctx, pdf_image);

	base_try(ctx)
	{
		
		if (pdf_is_jpx_image(ctx, dict))
		{
			pdf_load_jpx(xref, dict, image);
			
			if (forcemask)
			{
				base_pixmap *mask_pixmap;
				if (image->n != 2)
					base_throw(ctx, "softmask must be grayscale");
				mask_pixmap = base_alpha_from_gray(ctx, image->tile, 1);
				base_drop_pixmap(ctx, image->tile);
				image->tile = mask_pixmap;
			}
			break; 
		}

		w = pdf_to_int(pdf_dict_getsa(dict, "Width", "W"));
		h = pdf_to_int(pdf_dict_getsa(dict, "Height", "H"));
		bpc = pdf_to_int(pdf_dict_getsa(dict, "BitsPerComponent", "BPC"));
		imagemask = pdf_to_bool(pdf_dict_getsa(dict, "ImageMask", "IM"));
		interpolate = pdf_to_bool(pdf_dict_getsa(dict, "Interpolate", "I"));

		indexed = 0;
		usecolorkey = 0;
		mask = NULL;

		if (imagemask)
			bpc = 1;

		if (w == 0)
			base_throw(ctx, "image width is zero");
		if (h == 0)
			base_throw(ctx, "image height is zero");
		if (bpc == 0)
			base_throw(ctx, "image depth is zero");
		if (bpc > 16)
			base_throw(ctx, "image depth is too large: %d", bpc);
		if (w > (1 << 16))
			base_throw(ctx, "image is too wide");
		if (h > (1 << 16))
			base_throw(ctx, "image is too high");

		obj = pdf_dict_getsa(dict, "ColorSpace", "CS");
		if (obj && !imagemask && !forcemask)
		{
			
			if (pdf_is_name(obj))
			{
				res = pdf_dict_get(pdf_dict_gets(rdb, "ColorSpace"), obj);
				if (res)
					obj = res;
			}

			image->base.colorspace = pdf_load_colorspace(xref, obj);
			

			if (!strcmp(image->base.colorspace->name, "Indexed"))
				indexed = 1;

			n = image->base.colorspace->n;
		}
		else
		{
			n = 1;
		}

		obj = pdf_dict_getsa(dict, "Decode", "D");
		if (obj)
		{
			for (i = 0; i < n * 2; i++)
				image->decode[i] = pdf_to_real(pdf_array_get(obj, i));
		}
		else
		{
			float maxval = indexed ? (1 << bpc) - 1 : 1;
			for (i = 0; i < n * 2; i++)
				image->decode[i] = i & 1 ? maxval : 0;
		}

		obj = pdf_dict_getsa(dict, "SMask", "Mask");
		if (pdf_is_dict(obj))
		{
			
			if (!cstm)
			{
				mask = (base_image *)pdf_load_image_imp(xref, rdb, obj, NULL, 1);
				
			}
		}
		else if (pdf_is_array(obj))
		{
			usecolorkey = 1;
			for (i = 0; i < n * 2; i++)
			{
				if (!pdf_is_int(pdf_array_get(obj, i)))
				{
					base_warn(ctx, "invalid value in color key mask");
					usecolorkey = 0;
				}
				image->colorkey[i] = pdf_to_int(pdf_array_get(obj, i));
			}
		}

		
		image->params.type = PDF_IMAGE_RAW;
		base_INIT_STORABLE(&image->base, 1, pdf_free_image);
		image->base.get_pixmap = pdf_image_get_pixmap;
		image->base.w = w;
		image->base.h = h;
		image->n = n;
		image->bpc = bpc;
		image->interpolate = interpolate;
		image->imagemask = imagemask;
		image->usecolorkey = usecolorkey;
		image->base.mask = mask;
		image->params.colorspace = image->base.colorspace; 
		if (!indexed && !cstm)
		{
			
			image->buffer = pdf_load_image_stream(xref, pdf_to_num(dict), pdf_to_gen(dict), &image->params);
			break; 
		}

		
		if (cstm)
		{
			int stride = (w * image->n * image->bpc + 7) / 8;
			stm = pdf_open_inline_stream(xref, dict, stride * h, cstm, NULL);
		}
		else
		{
			stm = pdf_open_stream(xref, pdf_to_num(dict), pdf_to_gen(dict));
			
		}

		image->tile = decomp_image_from_stream(ctx, stm, image, cstm != NULL, indexed, 1);
	}
	base_catch(ctx)
	{
		base_drop_image(ctx, &image->base);
		base_rethrow(ctx);
	}
	return image;
}

base_image *
pdf_load_inline_image(pdf_document *xref, pdf_obj *rdb, pdf_obj *dict, base_stream *file)
{
	return (base_image *)pdf_load_image_imp(xref, rdb, dict, file, 0);
	
}

int
pdf_is_jpx_image(base_context *ctx, pdf_obj *dict)
{
	pdf_obj *filter;
	int i, n;

	filter = pdf_dict_gets(dict, "Filter");
	if (!strcmp(pdf_to_name(filter), "JPXDecode"))
		return 1;
	n = pdf_array_len(filter);
	for (i = 0; i < n; i++)
		if (!strcmp(pdf_to_name(pdf_array_get(filter, i)), "JPXDecode"))
			return 1;
	return 0;
}

static void
pdf_load_jpx(pdf_document *xref, pdf_obj *dict, pdf_image *image)
{
	base_buffer *buf = NULL;
	base_colorspace *colorspace = NULL;
	base_pixmap *img = NULL;
	pdf_obj *obj;
	base_context *ctx = xref->ctx;
	int indexed = 0;

	base_var(img);
	base_var(buf);
	base_var(colorspace);

	buf = pdf_load_stream(xref, pdf_to_num(dict), pdf_to_gen(dict));
	

	
	base_try(ctx)
	{
		obj = pdf_dict_gets(dict, "ColorSpace");
		if (obj)
		{
			colorspace = pdf_load_colorspace(xref, obj);
			
			indexed = !strcmp(colorspace->name, "Indexed");
		}

		img = base_load_jpx(ctx, buf->data, buf->len, colorspace, indexed);
		

		if (img && colorspace == NULL)
			colorspace = base_keep_colorspace(ctx, img->colorspace);

		base_drop_buffer(ctx, buf);
		buf = NULL;

		obj = pdf_dict_getsa(dict, "SMask", "Mask");
		if (pdf_is_dict(obj))
		{
			image->base.mask = (base_image *)pdf_load_image_imp(xref, NULL, obj, NULL, 1);
			
		}

		obj = pdf_dict_getsa(dict, "Decode", "D");
		if (obj && !indexed)
		{
			float decode[base_MAX_COLORS * 2];
			int i;

			for (i = 0; i < img->n * 2; i++)
				decode[i] = pdf_to_real(pdf_array_get(obj, i));

			base_decode_tile(img, decode);
		}
	}
	base_catch(ctx)
	{
		if (colorspace)
			base_drop_colorspace(ctx, colorspace);
		base_drop_buffer(ctx, buf);
		base_drop_pixmap(ctx, img);
		base_rethrow(ctx);
	}
	image->params.type = PDF_IMAGE_RAW;
	base_INIT_STORABLE(&image->base, 1, pdf_free_image);
	image->base.get_pixmap = pdf_image_get_pixmap;
	image->base.w = img->w;
	image->base.h = img->h;
	image->base.colorspace = colorspace;
	image->tile = img;
	image->n = img->n;
	image->bpc = 8;
	image->interpolate = 0;
	image->imagemask = 0;
	image->usecolorkey = 0;
	image->params.colorspace = colorspace; 
}

static int
pdf_image_size(base_context *ctx, pdf_image *im)
{
	if (im == NULL)
		return 0;
	return sizeof(*im) + base_pixmap_size(ctx, im->tile) + (im->buffer ? im->buffer->cap : 0);
}

base_image *
pdf_load_image(pdf_document *xref, pdf_obj *dict)
{
	base_context *ctx = xref->ctx;
	pdf_image *image;

	if ((image = pdf_find_item(ctx, pdf_free_image, dict)))
	{
		return (base_image *)image;
	}

	image = pdf_load_image_imp(xref, NULL, dict, NULL, 0);
	

	pdf_store_item(ctx, dict, image, pdf_image_size(ctx, image));

	return (base_image *)image;
}
