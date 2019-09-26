

#ifndef __RAW_H
#define __RAW_H

typedef struct opj_raw {
	
	unsigned char c;
	
	unsigned int ct;
	
	unsigned int lenmax;
	
	unsigned int len;
	
	unsigned char *bp;
	
	unsigned char *start;
	
	unsigned char *end;
} opj_raw_t;

opj_raw_t* raw_create(void);

void raw_destroy(opj_raw_t *raw);

int raw_numbytes(opj_raw_t *raw);

void raw_init_dec(opj_raw_t *raw, unsigned char *bp, int len);

int raw_decode(opj_raw_t *raw);

#endif 
