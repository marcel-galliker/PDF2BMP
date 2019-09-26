

#include "opj_includes.h"

opj_raw_t* raw_create(void) {
	opj_raw_t *raw = (opj_raw_t*)opj_malloc(sizeof(opj_raw_t));
	return raw;
}

void raw_destroy(opj_raw_t *raw) {
	if(raw) {
		opj_free(raw);
	}
}

int raw_numbytes(opj_raw_t *raw) {
	return raw->bp - raw->start;
}

void raw_init_dec(opj_raw_t *raw, unsigned char *bp, int len) {
	raw->start = bp;
	raw->lenmax = len;
	raw->len = 0;
	raw->c = 0;
	raw->ct = 0;
}

int raw_decode(opj_raw_t *raw) {
	int d;
	if (raw->ct == 0) {
		raw->ct = 8;
		if (raw->len == raw->lenmax) {
			raw->c = 0xff;
		} else {
			if (raw->c == 0xff) {
				raw->ct = 7;
			}
			raw->c = *(raw->start + raw->len);
			raw->len++;
		}
	}
	raw->ct--;
	d = (raw->c >> raw->ct) & 0x01;
	
	return d;
}

