

#include "opj_includes.h"

static void bio_putbit(opj_bio_t *bio, int b);

static int bio_getbit(opj_bio_t *bio);

static int bio_byteout(opj_bio_t *bio);

static int bio_bytein(opj_bio_t *bio);

static int bio_byteout(opj_bio_t *bio) {
	bio->buf = (bio->buf << 8) & 0xffff;
	bio->ct = bio->buf == 0xff00 ? 7 : 8;
	if (bio->bp >= bio->end) {
		return 1;
	}
	*bio->bp++ = bio->buf >> 8;
	return 0;
}

static int bio_bytein(opj_bio_t *bio) {
	bio->buf = (bio->buf << 8) & 0xffff;
	bio->ct = bio->buf == 0xff00 ? 7 : 8;
	if (bio->bp >= bio->end) {
		return 1;
	}
	bio->buf |= *bio->bp++;
	return 0;
}

static void bio_putbit(opj_bio_t *bio, int b) {
	if (bio->ct == 0) {
		bio_byteout(bio);
	}
	bio->ct--;
	bio->buf |= b << bio->ct;
}

static int bio_getbit(opj_bio_t *bio) {
	if (bio->ct == 0) {
		bio_bytein(bio);
	}
	bio->ct--;
	return (bio->buf >> bio->ct) & 1;
}

opj_bio_t* bio_create(void) {
	opj_bio_t *bio = (opj_bio_t*)opj_malloc(sizeof(opj_bio_t));
	return bio;
}

void bio_destroy(opj_bio_t *bio) {
	if(bio) {
		opj_free(bio);
	}
}

int bio_numbytes(opj_bio_t *bio) {
	return (bio->bp - bio->start);
}

void bio_init_enc(opj_bio_t *bio, unsigned char *bp, int len) {
	bio->start = bp;
	bio->end = bp + len;
	bio->bp = bp;
	bio->buf = 0;
	bio->ct = 8;
}

void bio_init_dec(opj_bio_t *bio, unsigned char *bp, int len) {
	bio->start = bp;
	bio->end = bp + len;
	bio->bp = bp;
	bio->buf = 0;
	bio->ct = 0;
}

void bio_write(opj_bio_t *bio, int v, int n) {
	int i;
	for (i = n - 1; i >= 0; i--) {
		bio_putbit(bio, (v >> i) & 1);
	}
}

int bio_read(opj_bio_t *bio, int n) {
	int i, v;
	v = 0;
	for (i = n - 1; i >= 0; i--) {
		v += bio_getbit(bio) << i;
	}
	return v;
}

int bio_flush(opj_bio_t *bio) {
	bio->ct = 0;
	if (bio_byteout(bio)) {
		return 1;
	}
	if (bio->ct == 7) {
		bio->ct = 0;
		if (bio_byteout(bio)) {
			return 1;
		}
	}
	return 0;
}

int bio_inalign(opj_bio_t *bio) {
	bio->ct = 0;
	if ((bio->buf & 0xff) == 0xff) {
		if (bio_bytein(bio)) {
			return 1;
		}
		bio->ct = 0;
	}
	return 0;
}
