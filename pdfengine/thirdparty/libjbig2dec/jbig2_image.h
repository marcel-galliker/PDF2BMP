

#ifndef _JBIG2_IMAGE_H
#define _JBIG2_IMAGE_H

int jbig2_image_get_pixel(Jbig2Image *image, int x, int y);
int jbig2_image_set_pixel(Jbig2Image *image, int x, int y, bool value);

#include <stdio.h>

int jbig2_image_write_pbm_file(Jbig2Image *image, char *filename);
int jbig2_image_write_pbm(Jbig2Image *image, FILE *out);
Jbig2Image *jbig2_image_read_pbm_file(Jbig2Ctx *ctx, char *filename);
Jbig2Image *jbig2_image_read_pbm(Jbig2Ctx *ctx, FILE *in);

#ifdef HAVE_LIBPNG
int jbig2_image_write_png_file(Jbig2Image *image, char *filename);
int jbig2_image_write_png(Jbig2Image *image, FILE *out);
#endif

#endif 
