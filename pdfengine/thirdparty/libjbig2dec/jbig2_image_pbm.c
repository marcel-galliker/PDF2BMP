

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "os_types.h"

#include <stdio.h>
#include <ctype.h>

#include "jbig2.h"
#include "jbig2_priv.h"
#include "jbig2_image.h"

int jbig2_image_write_pbm_file(Jbig2Image *image, char *filename)
{
    FILE *out;
    int	error;

    if ((out = fopen(filename, "wb")) == NULL) {
        fprintf(stderr, "unable to open '%s' for writing", filename);
        return 1;
    }

    error = jbig2_image_write_pbm(image, out);

    fclose(out);
    return (error);
}

int jbig2_image_write_pbm(Jbig2Image *image, FILE *out)
{
        
        fprintf(out, "P4\n%d %d\n", image->width, image->height);

        
        fwrite(image->data, 1, image->height*image->stride, out);

        
	return 0;
}

Jbig2Image *jbig2_image_read_pbm_file(Jbig2Ctx *ctx, char *filename)
{
    FILE *in;
    Jbig2Image *image;

    if ((in = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "unable to open '%s' for reading\n", filename);
		return NULL;
    }

    image = jbig2_image_read_pbm(ctx, in);

    fclose(in);

    return (image);
}

Jbig2Image *jbig2_image_read_pbm(Jbig2Ctx *ctx, FILE *in)
{
    int i, dim[2];
    int done;
    Jbig2Image *image;
    int c;
    char buf[32];

    
    while ((c = fgetc(in)) != 'P') {
        if (feof(in)) return NULL;
    }
    if ((c = fgetc(in)) != '4') {
        fprintf(stderr, "not a binary pbm file.\n");
        return NULL;
    }
    
    done = 0;
    i = 0;
    while (done < 2) {
        c = fgetc(in);
        
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        
        if (c == '#') {
            while ((c = fgetc(in)) != '\n');
            continue;
        }
        
        if (c == EOF) {
           fprintf(stderr, "end-of-file parsing pbm header\n");
           return NULL;
        }
        if (isdigit(c)) {
            buf[i++] = c;
            while (isdigit(c = fgetc(in))) {
                if (i >= 32) {
                    fprintf(stderr, "pbm parsing error\n");
                    return NULL;
                }
                buf[i++] = c;
            }
            buf[i] = '\0';
            if (sscanf(buf, "%d", &dim[done]) != 1) {
                fprintf(stderr, "couldn't read pbm image dimensions\n");
                return NULL;
            }
            i = 0;
            done++;
        }
    }
    
    image = jbig2_image_new(ctx, dim[0], dim[1]);
    if (image == NULL) {
        fprintf(stderr, "could not allocate %dx%d image for pbm file\n", dim[0], dim[1]);
        return NULL;
    }
    
    fread(image->data, 1, image->height*image->stride, in);
    if (feof(in)) {
        fprintf(stderr, "unexpected end of pbm file.\n");
        jbig2_image_release(ctx, image);
        return NULL;
    }
    
    return image;
}
