#include "pdf-internal.h"

#ifndef OPJ_STATIC
	#define OPJ_STATIC
#endif
#include <openjpeg.h>

static void base_opj_error_callback(const char *msg, void *client_data)
{
	base_context *ctx = (base_context *)client_data;
	base_warn(ctx, "openjpeg error: %s", msg);
}

static void base_opj_warning_callback(const char *msg, void *client_data)
{
	base_context *ctx = (base_context *)client_data;
	base_warn(ctx, "openjpeg warning: %s", msg);
}

static void base_opj_info_callback(const char *msg, void *client_data)
{
	
}

base_pixmap *
base_load_jpx(base_context *ctx, unsigned char *data, int size, base_colorspace *defcs, int indexed)
{
	base_pixmap *img;
	opj_event_mgr_t evtmgr;
	opj_dparameters_t params;
	opj_dinfo_t *info;
	opj_cio_t *cio;
	opj_image_t *jpx;
	base_colorspace *colorspace;
	unsigned char *p;
	int format;
	int a, n, w, h, depth, sgnd;
	int x, y, k, v;

	if (size < 2)
		base_throw(ctx, "not enough data to determine image format");

	
	if (data[0] == 0xFF && data[1] == 0x4F)
		format = CODEC_J2K;
	else
		format = CODEC_JP2;

	memset(&evtmgr, 0, sizeof(evtmgr));
	evtmgr.error_handler = base_opj_error_callback;
	evtmgr.warning_handler = base_opj_warning_callback;
	evtmgr.info_handler = base_opj_info_callback;

	opj_set_default_decoder_parameters(&params);
	if (indexed)
		params.flags |= OPJ_DPARAMETERS_IGNORE_PCLR_CMAP_CDEF_FLAG;

	info = opj_create_decompress(format);
	opj_set_event_mgr((opj_common_ptr)info, &evtmgr, ctx);
	opj_setup_decoder(info, &params);

	cio = opj_cio_open((opj_common_ptr)info, data, size);

	jpx = opj_decode(info, cio);

	opj_cio_close(cio);
	opj_destroy_decompress(info);

	if (!jpx)
		base_throw(ctx, "opj_decode failed");

	for (k = 1; k < jpx->numcomps; k++)
	{
		if (jpx->comps[k].w != jpx->comps[0].w)
			base_throw(ctx, "image components have different width");
		if (jpx->comps[k].h != jpx->comps[0].h)
			base_throw(ctx, "image components have different height");
		if (jpx->comps[k].prec != jpx->comps[0].prec)
			base_throw(ctx, "image components have different precision");
	}

	n = jpx->numcomps;
	w = jpx->comps[0].w;
	h = jpx->comps[0].h;
	depth = jpx->comps[0].prec;
	sgnd = jpx->comps[0].sgnd;

	if (jpx->color_space == CLRSPC_SRGB && n == 4) { n = 3; a = 1; }
	else if (jpx->color_space == CLRSPC_SYCC && n == 4) { n = 3; a = 1; }
	else if (n == 2) { n = 1; a = 1; }
	else if (n > 4) { n = 4; a = 1; }
	else { a = 0; }

	if (defcs)
	{
		if (defcs->n == n)
		{
			colorspace = defcs;
		}
		else
		{
			base_warn(ctx, "jpx file and dict colorspaces do not match");
			defcs = NULL;
		}
	}

	if (!defcs)
	{
		switch (n)
		{
		case 1: colorspace = base_device_gray; break;
		case 3: colorspace = base_device_rgb; break;
		case 4: colorspace = base_device_cmyk; break;
		}
	}

	base_try(ctx)
	{
		img = base_new_pixmap(ctx, colorspace, w, h);
	}
	base_catch(ctx)
	{
		opj_image_destroy(jpx);
		base_throw(ctx, "out of memory");
	}

	p = img->samples;
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			for (k = 0; k < n + a; k++)
			{
				v = jpx->comps[k].data[y * w + x];
				if (sgnd)
					v = v + (1 << (depth - 1));
				if (depth > 8)
					v = v >> (depth - 8);
				*p++ = v;
			}
			if (!a)
				*p++ = 255;
		}
	}

	if (a)
	{
		if (n == 4)
		{
			base_pixmap *tmp = base_new_pixmap(ctx, base_device_rgb, w, h);
			base_convert_pixmap(ctx, tmp, img);
			base_drop_pixmap(ctx, img);
			img = tmp;
		}
		base_premultiply_pixmap(ctx, img);
	}

	opj_image_destroy(jpx);

	return img;
}
