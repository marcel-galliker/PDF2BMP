

#ifndef __PI_H
#define __PI_H

typedef struct opj_pi_resolution {
  int pdx, pdy;
  int pw, ph;
} opj_pi_resolution_t;

typedef struct opj_pi_comp {
  int dx, dy;
  
  int numresolutions;
  opj_pi_resolution_t *resolutions;
} opj_pi_comp_t;

typedef struct opj_pi_iterator {
	
	char tp_on;
	
	short int *include;
	
	int step_l;
	
	int step_r;	
	
	int step_c;	
	
	int step_p;	
	
	int compno;
	
	int resno;
	
	int precno;
	
	int layno;   
	
	int first;
	
	opj_poc_t poc;
	
	int numcomps;
	
	opj_pi_comp_t *comps;
	int tx0, ty0, tx1, ty1;
	int x, y, dx, dy;
} opj_pi_iterator_t;

opj_pi_iterator_t *pi_initialise_encode(opj_image_t *image, opj_cp_t *cp, int tileno,J2K_T2_MODE t2_mode);

opj_bool pi_create_encode(opj_pi_iterator_t *pi, opj_cp_t *cp,int tileno, int pino,int tpnum, int tppos, J2K_T2_MODE t2_mode,int cur_totnum_tp);

opj_pi_iterator_t *pi_create_decode(opj_image_t * image, opj_cp_t * cp, int tileno);

void pi_destroy(opj_pi_iterator_t *pi, opj_cp_t *cp, int tileno);

opj_bool pi_next(opj_pi_iterator_t * pi);

#endif 
