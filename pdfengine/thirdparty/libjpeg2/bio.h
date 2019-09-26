

#ifndef __BIO_H
#define __BIO_H

typedef struct opj_bio {
	
	unsigned char *start;
	
	unsigned char *end;
	
	unsigned char *bp;
	
	unsigned int buf;
	
	int ct;
} opj_bio_t;

opj_bio_t* bio_create(void);

void bio_destroy(opj_bio_t *bio);

int bio_numbytes(opj_bio_t *bio);

void bio_init_enc(opj_bio_t *bio, unsigned char *bp, int len);

void bio_init_dec(opj_bio_t *bio, unsigned char *bp, int len);

void bio_write(opj_bio_t *bio, int v, int n);

int bio_read(opj_bio_t *bio, int n);

int bio_flush(opj_bio_t *bio);

int bio_inalign(opj_bio_t *bio);

#endif 

