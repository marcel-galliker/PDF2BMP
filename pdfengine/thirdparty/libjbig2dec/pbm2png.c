

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "config_types.h"
#elif _WIN32
#include "config_win32.h"
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jbig2.h"
#include "jbig2_image.h"

int main(int argc, char *argv[])
{
    Jbig2Ctx *ctx;
    Jbig2Image *image;
    int error;

    
    ctx = jbig2_ctx_new(NULL, 0, NULL, NULL, NULL);

    if (argc != 3) {
        fprintf(stderr, "usage: %s <in.pbm> <out.png>\n\n", argv[0]);
        return 1;
    }

    image = jbig2_image_read_pbm_file(ctx, argv[1]);
    if(image == NULL) {
        fprintf(stderr, "error reading pbm file '%s'\n", argv[1]);
        return 1;
    } else {
        fprintf(stderr, "converting %dx%d image to png format\n", image->width, image->height);
    }

    error = jbig2_image_write_png_file(image, argv[2]);
    if (error) {
        fprintf(stderr, "error writing png file '%s' error %d\n", argv[2], error);
    }

    return (error);
}
