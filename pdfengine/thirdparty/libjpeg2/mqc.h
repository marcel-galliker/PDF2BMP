

#ifndef __MQC_H
#define __MQC_H

typedef struct opj_mqc_state {
	
	unsigned int qeval;
	
	int mps;
	
	struct opj_mqc_state *nmps;
	
	struct opj_mqc_state *nlps;
} opj_mqc_state_t;

#define MQC_NUMCTXS 19

typedef struct opj_mqc {
	unsigned int c;
	unsigned int a;
	unsigned int ct;
	unsigned char *bp;
	unsigned char *start;
	unsigned char *end;
	opj_mqc_state_t *ctxs[MQC_NUMCTXS];
	opj_mqc_state_t **curctx;
#ifdef MQC_PERF_OPT
	unsigned char *buffer;
#endif
} opj_mqc_t;

opj_mqc_t* mqc_create(void);

void mqc_destroy(opj_mqc_t *mqc);

int mqc_numbytes(opj_mqc_t *mqc);

void mqc_resetstates(opj_mqc_t *mqc);

void mqc_setstate(opj_mqc_t *mqc, int ctxno, int msb, int prob);

void mqc_init_enc(opj_mqc_t *mqc, unsigned char *bp);

#define mqc_setcurctx(mqc, ctxno)	(mqc)->curctx = &(mqc)->ctxs[(int)(ctxno)]

void mqc_encode(opj_mqc_t *mqc, int d);

void mqc_flush(opj_mqc_t *mqc);

void mqc_bypass_init_enc(opj_mqc_t *mqc);

void mqc_bypass_enc(opj_mqc_t *mqc, int d);

int mqc_bypass_flush_enc(opj_mqc_t *mqc);

void mqc_reset_enc(opj_mqc_t *mqc);

int mqc_restart_enc(opj_mqc_t *mqc);

void mqc_restart_init_enc(opj_mqc_t *mqc);

void mqc_erterm_enc(opj_mqc_t *mqc);

void mqc_segmark_enc(opj_mqc_t *mqc);

void mqc_init_dec(opj_mqc_t *mqc, unsigned char *bp, int len);

int mqc_decode(opj_mqc_t *const mqc);

#endif 
