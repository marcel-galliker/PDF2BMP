

#include "opj_includes.h"

opj_image_t* opj_image_create0(void) {
	opj_image_t *image = (opj_image_t*)opj_calloc(1, sizeof(opj_image_t));
	return image;
}

opj_image_t* OPJ_CALLCONV opj_image_create(int numcmpts, opj_image_cmptparm_t *cmptparms, OPJ_COLOR_SPACE clrspc) {
	int compno;
	opj_image_t *image = NULL;

	image = (opj_image_t*) opj_calloc(1, sizeof(opj_image_t));
	if(image) {
		image->color_space = clrspc;
		image->numcomps = numcmpts;
		
		image->comps = (opj_image_comp_t*)opj_malloc(image->numcomps * sizeof(opj_image_comp_t));
		if(!image->comps) {
			fprintf(stderr,"Unable to allocate memory for image.\n");
			opj_image_destroy(image);
			return NULL;
		}
		
		for(compno = 0; compno < numcmpts; compno++) {
			opj_image_comp_t *comp = &image->comps[compno];
			comp->dx = cmptparms[compno].dx;
			comp->dy = cmptparms[compno].dy;
			comp->w = cmptparms[compno].w;
			comp->h = cmptparms[compno].h;
			comp->x0 = cmptparms[compno].x0;
			comp->y0 = cmptparms[compno].y0;
			comp->prec = cmptparms[compno].prec;
			comp->bpp = cmptparms[compno].bpp;
			comp->sgnd = cmptparms[compno].sgnd;
			comp->data = (int*) opj_calloc(comp->w * comp->h, sizeof(int));
			if(!comp->data) {
				fprintf(stderr,"Unable to allocate memory for image.\n");
				opj_image_destroy(image);
				return NULL;
			}
		}
	}

	return image;
}

void OPJ_CALLCONV opj_image_destroy(opj_image_t *image) {
	int i;
	if(image) {
		if(image->comps) {
			
			for(i = 0; i < image->numcomps; i++) {
				opj_image_comp_t *image_comp = &image->comps[i];
				if(image_comp->data) {
					opj_free(image_comp->data);
				}
			}
			opj_free(image->comps);
		}
		opj_free(image);
	}
}

