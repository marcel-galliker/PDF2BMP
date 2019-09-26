
#ifndef __TCD_H
#define __TCD_H

typedef struct opj_tcd_seg {
  unsigned char** data;
  int dataindex;
  int numpasses;
  int len;
  int maxpasses;
  int numnewpasses;
  int newlen;
} opj_tcd_seg_t;

typedef struct opj_tcd_pass {
  int rate;
  double distortiondec;
  int term, len;
} opj_tcd_pass_t;

typedef struct opj_tcd_layer {
  int numpasses;		
  int len;			
  double disto;			
  unsigned char *data;		
} opj_tcd_layer_t;

typedef struct opj_tcd_cblk_enc {
  unsigned char* data;	
  opj_tcd_layer_t* layers;	
  opj_tcd_pass_t* passes;	
  int x0, y0, x1, y1;		
  int numbps;
  int numlenbits;
  int numpasses;		
  int numpassesinlayers;	
  int totalpasses;		
} opj_tcd_cblk_enc_t;

typedef struct opj_tcd_cblk_dec {
  unsigned char* data;	
  opj_tcd_seg_t* segs;		
	int x0, y0, x1, y1;		
  int numbps;
  int numlenbits;
  int len;			
  int numnewpasses;		
  int numsegs;			
} opj_tcd_cblk_dec_t;

typedef struct opj_tcd_precinct {
  int x0, y0, x1, y1;		
  int cw, ch;			
  union{		
	  opj_tcd_cblk_enc_t* enc;
	  opj_tcd_cblk_dec_t* dec;
  } cblks;
  opj_tgt_tree_t *incltree;		
  opj_tgt_tree_t *imsbtree;		
} opj_tcd_precinct_t;

typedef struct opj_tcd_band {
  int x0, y0, x1, y1;		
  int bandno;
  opj_tcd_precinct_t *precincts;	
  int numbps;
  float stepsize;
} opj_tcd_band_t;

typedef struct opj_tcd_resolution {
  int x0, y0, x1, y1;		
  int pw, ph;
  int numbands;			
  opj_tcd_band_t bands[3];		
} opj_tcd_resolution_t;

typedef struct opj_tcd_tilecomp {
  int x0, y0, x1, y1;		
  int numresolutions;		
  opj_tcd_resolution_t *resolutions;	
  int *data;			
  int numpix;			
} opj_tcd_tilecomp_t;

typedef struct opj_tcd_tile {
  int x0, y0, x1, y1;		
  int numcomps;			
  opj_tcd_tilecomp_t *comps;	
  int numpix;			
  double distotile;		
  double distolayer[100];	
  
  int packno;
} opj_tcd_tile_t;

typedef struct opj_tcd_image {
  int tw, th;			
  opj_tcd_tile_t *tiles;		
} opj_tcd_image_t;

typedef struct opj_tcd {
	
	int tp_pos;
	
	int tp_num;
	
	int cur_tp_num;
	
	int cur_totnum_tp;
	
	int cur_pino;
	
	opj_common_ptr cinfo;

	
	opj_tcd_image_t *tcd_image;
	
	opj_image_t *image;
	
	opj_cp_t *cp;
	
	opj_tcd_tile_t *tcd_tile;
	
	opj_tcp_t *tcp;
	
	int tcd_tileno;
	
	double encoding_time;
} opj_tcd_t;

void tcd_dump(FILE *fd, opj_tcd_t *tcd, opj_tcd_image_t *img);

opj_tcd_t* tcd_create(opj_common_ptr cinfo);

void tcd_destroy(opj_tcd_t *tcd);

void tcd_malloc_encode(opj_tcd_t *tcd, opj_image_t * image, opj_cp_t * cp, int curtileno);

void tcd_free_encode(opj_tcd_t *tcd);

void tcd_init_encode(opj_tcd_t *tcd, opj_image_t * image, opj_cp_t * cp, int curtileno);

void tcd_malloc_decode(opj_tcd_t *tcd, opj_image_t * image, opj_cp_t * cp);
void tcd_malloc_decode_tile(opj_tcd_t *tcd, opj_image_t * image, opj_cp_t * cp, int tileno, opj_codestream_info_t *cstr_info);
void tcd_makelayer_fixed(opj_tcd_t *tcd, int layno, int final);
void tcd_rateallocate_fixed(opj_tcd_t *tcd);
void tcd_makelayer(opj_tcd_t *tcd, int layno, double thresh, int final);
opj_bool tcd_rateallocate(opj_tcd_t *tcd, unsigned char *dest, int len, opj_codestream_info_t *cstr_info);

int tcd_encode_tile(opj_tcd_t *tcd, int tileno, unsigned char *dest, int len, opj_codestream_info_t *cstr_info);

opj_bool tcd_decode_tile(opj_tcd_t *tcd, unsigned char *src, int len, int tileno, opj_codestream_info_t *cstr_info);

void tcd_free_decode(opj_tcd_t *tcd);
void tcd_free_decode_tile(opj_tcd_t *tcd, int tileno);

#endif 
