

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "os_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#include "jbig2.h"
#include "jbig2_priv.h"
#include "jbig2_image.h"

static void
jbig2_png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    png_size_t check;

    check = fwrite(data, 1, length, (png_FILE_p)png_ptr->io_ptr);
    if (check != length) {
      png_error(png_ptr, "Write Error");
    }
}

static void
jbig2_png_flush(png_structp png_ptr)
{
    png_FILE_p io_ptr;
    io_ptr = (png_FILE_p)CVT_PTR((png_ptr->io_ptr));
    if (io_ptr != NULL)
        fflush(io_ptr);
}

int jbig2_image_write_png_file(Jbig2Image *image, char *filename)
{
    FILE *out;
    int	error;

    if ((out = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "unable to open '%s' for writing\n", filename);
		return 1;
    }

    error = jbig2_image_write_png(image, out);

    fclose(out);
    return (error);
}

int jbig2_image_write_png(Jbig2Image *image, FILE *out)
{
	int		i;
	png_structp	png;
	png_infop	info;
	png_bytep	rowpointer;

	png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	if (png == NULL) {
		fprintf(stderr, "unable to create png structure\n");
		return 2;
	}

	info = png_create_info_struct(png);
	if (info == NULL) {
            fprintf(stderr, "unable to create png info structure\n");
            png_destroy_write_struct(&png,  (png_infopp)NULL);
            return 3;
	}

	
	if (setjmp(png_jmpbuf(png))) {
		
		fprintf(stderr, "internal error in libpng saving file\n");
		png_destroy_write_struct(&png, &info);
		return 4;
	}

        
	
        png_set_write_fn(png, (png_voidp)out, jbig2_png_write_data,
            jbig2_png_flush);

	
	png_set_IHDR(png, info, image->width, image->height,
		1, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png, info);

	
	png_set_invert_mono(png);

	
	rowpointer = (png_bytep)image->data;
	for(i = 0; i < image->height; i++) {
		png_write_row(png, rowpointer);
		rowpointer += image->stride;
	}

	
	png_write_end(png, info);
	png_destroy_write_struct(&png, &info);

	return 0;
}
