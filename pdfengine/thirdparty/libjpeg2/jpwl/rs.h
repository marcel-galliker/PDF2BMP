

#ifdef USE_JPWL

#ifndef __RS_HEADER__
#define __RS_HEADER__

#define MM  8		

#define	NN ((1 << MM) - 1)

#if (MM <= 8)
typedef unsigned char dtype;
#else
typedef unsigned int dtype;
#endif

void init_rs(int);

void generate_gf(void);	
void gen_poly(void);	

int encode_rs(dtype data[], dtype bb[]);

int eras_dec_rs(dtype data[], int eras_pos[], int no_eras);

#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif 

#endif 

#endif 
