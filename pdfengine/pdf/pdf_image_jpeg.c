#include "pdf-internal.h"

#include <jpeglib.h>
#include <setjmp.h>

struct jpeg_error_mgr_jmp
{
	struct jpeg_error_mgr super;
	jmp_buf env;
	char msg[JMSG_LENGTH_MAX];
};

static void error_exit(j_common_ptr cinfo)
{
	struct jpeg_error_mgr_jmp *err = (struct jpeg_error_mgr_jmp *)cinfo->err;
	cinfo->err->format_message(cinfo, err->msg);
	longjmp(err->env, 1);
}

static void init_source(j_decompress_ptr cinfo)
{
	
}

static void term_source(j_decompress_ptr cinfo)
{
	
}

static boolean fill_input_buffer(j_decompress_ptr cinfo)
{
	static unsigned char eoi[2] = { 0xFF, JPEG_EOI };
	struct jpeg_source_mgr *src = cinfo->src;
	src->next_input_byte = eoi;
	src->bytes_in_buffer = 2;
	return 1;
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	struct jpeg_source_mgr *src = cinfo->src;
	if (num_bytes > 0)
	{
		src->next_input_byte += num_bytes;
		src->bytes_in_buffer -= num_bytes;
	}
}

base_pixmap *
base_load_jpeg(base_context *ctx, unsigned char *rbuf, int rlen)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr_jmp err;
	struct jpeg_source_mgr src;
	unsigned char *row[1], *sp, *dp;
	base_colorspace *colorspace;
	unsigned int x;
	int k;

	base_pixmap *image = NULL;

	if (setjmp(err.env))
	{
		if (image)
			base_drop_pixmap(ctx, image);
		base_throw(ctx, "jpeg error: %s", err.msg);
	}

	cinfo.err = jpeg_std_error(&err.super);
	err.super.error_exit = error_exit;

	jpeg_create_decompress(&cinfo);

	cinfo.src = &src;
	src.init_source = init_source;
	src.fill_input_buffer = fill_input_buffer;
	src.skip_input_data = skip_input_data;
	src.resync_to_restart = jpeg_resync_to_restart;
	src.term_source = term_source;
	src.next_input_byte = rbuf;
	src.bytes_in_buffer = rlen;

	jpeg_read_header(&cinfo, 1);

	jpeg_start_decompress(&cinfo);

	if (cinfo.output_components == 1)
		colorspace = base_device_gray;
	else if (cinfo.output_components == 3)
		colorspace = base_device_rgb;
	else if (cinfo.output_components == 4)
		colorspace = base_device_cmyk;
	else
		base_throw(ctx, "bad number of components in jpeg: %d", cinfo.output_components);

	base_try(ctx)
	{
		image = base_new_pixmap(ctx, colorspace, cinfo.output_width, cinfo.output_height);
	}
	base_catch(ctx)
	{
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		base_throw(ctx, "out of memory");
	}

	if (cinfo.density_unit == 1)
	{
		image->xres = cinfo.X_density;
		image->yres = cinfo.Y_density;
	}
	else if (cinfo.density_unit == 2)
	{
		image->xres = cinfo.X_density * 254 / 100;
		image->yres = cinfo.Y_density * 254 / 100;
	}

	if (image->xres <= 0) image->xres = 72;
	if (image->yres <= 0) image->yres = 72;

	base_clear_pixmap(ctx, image);

	row[0] = base_malloc(ctx, cinfo.output_components * cinfo.output_width);
	dp = image->samples;
	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo, row, 1);
		sp = row[0];
		for (x = 0; x < cinfo.output_width; x++)
		{
			for (k = 0; k < cinfo.output_components; k++)
				*dp++ = *sp++;
			*dp++ = 255;
		}
	}
	base_free(ctx, row[0]);

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return image;
}

/***************************************************************************************/
/* function name:	base_write_jpeg
/* description:		save pixmap as jpeg file
/* param 1:			pointer to the context
/* param 2:			pixmap to save
/* param 3:			filepath where to save
/* param 4:			quality value
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_write_jpeg(base_context *ctx, base_pixmap *pixmap, wchar_t *filename, int quality)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr	jerr;
	FILE * outfile;

	JSAMPROW row_pointer[1];
	int row_stride;
	int counter;

	cinfo.err	= jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	outfile = _wfopen(filename, L"wb");

	if (outfile == NULL)
		base_throw(ctx, "File Open Error : %s", filename);

	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = pixmap->w;
	cinfo.image_height = pixmap->h;
	cinfo.input_components = 3; 
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	

	jpeg_start_compress(&cinfo, TRUE);

	row_stride = pixmap->w * pixmap->n;

	row_pointer[0] = base_malloc(ctx, cinfo.input_components * cinfo.image_width);

	while (cinfo.next_scanline < cinfo.image_height) {
		for (counter = 0; counter <(int) cinfo.image_width; counter++)
			memcpy(row_pointer[0] + counter * 3, & pixmap->samples[cinfo.next_scanline * row_stride + counter * pixmap->n], 3);
		
		
		(void) jpeg_write_scanlines(&cinfo,	row_pointer, 1);
	}

	base_free(ctx, row_pointer[0]);

	jpeg_finish_compress(&cinfo);
	fclose(outfile);
	jpeg_destroy_compress(&cinfo);
}
