
#ifndef __T2_H
#define __T2_H

typedef struct opj_t2 {
	
	opj_common_ptr cinfo;

	
	opj_image_t *image;
	
	opj_cp_t *cp;
} opj_t2_t;

int t2_encode_packets(opj_t2_t* t2,int tileno, opj_tcd_tile_t *tile, int maxlayers, unsigned char *dest, int len, opj_codestream_info_t *cstr_info,int tpnum, int tppos,int pino,J2K_T2_MODE t2_mode,int cur_totnum_tp);

int t2_decode_packets(opj_t2_t *t2, unsigned char *src, int len, int tileno, opj_tcd_tile_t *tile, opj_codestream_info_t *cstr_info);

opj_t2_t* t2_create(opj_common_ptr cinfo, opj_image_t *image, opj_cp_t *cp);

void t2_destroy(opj_t2_t *t2);

#endif 
