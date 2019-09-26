
#ifndef __T1_H
#define __T1_H

#define T1_NMSEDEC_BITS 7

#define T1_SIG_NE 0x0001	
#define T1_SIG_SE 0x0002	
#define T1_SIG_SW 0x0004	
#define T1_SIG_NW 0x0008	
#define T1_SIG_N 0x0010		
#define T1_SIG_E 0x0020		
#define T1_SIG_S 0x0040		
#define T1_SIG_W 0x0080		
#define T1_SIG_OTH (T1_SIG_N|T1_SIG_NE|T1_SIG_E|T1_SIG_SE|T1_SIG_S|T1_SIG_SW|T1_SIG_W|T1_SIG_NW)
#define T1_SIG_PRIM (T1_SIG_N|T1_SIG_E|T1_SIG_S|T1_SIG_W)

#define T1_SGN_N 0x0100
#define T1_SGN_E 0x0200
#define T1_SGN_S 0x0400
#define T1_SGN_W 0x0800
#define T1_SGN (T1_SGN_N|T1_SGN_E|T1_SGN_S|T1_SGN_W)

#define T1_SIG 0x1000
#define T1_REFINE 0x2000
#define T1_VISIT 0x4000

#define T1_NUMCTXS_ZC 9
#define T1_NUMCTXS_SC 5
#define T1_NUMCTXS_MAG 3
#define T1_NUMCTXS_AGG 1
#define T1_NUMCTXS_UNI 1

#define T1_CTXNO_ZC 0
#define T1_CTXNO_SC (T1_CTXNO_ZC+T1_NUMCTXS_ZC)
#define T1_CTXNO_MAG (T1_CTXNO_SC+T1_NUMCTXS_SC)
#define T1_CTXNO_AGG (T1_CTXNO_MAG+T1_NUMCTXS_MAG)
#define T1_CTXNO_UNI (T1_CTXNO_AGG+T1_NUMCTXS_AGG)
#define T1_NUMCTXS (T1_CTXNO_UNI+T1_NUMCTXS_UNI)

#define T1_NMSEDEC_FRACBITS (T1_NMSEDEC_BITS-1)

#define T1_TYPE_MQ 0	
#define T1_TYPE_RAW 1	

typedef short flag_t;

typedef struct opj_t1 {
	
	opj_common_ptr cinfo;

	
	opj_mqc_t *mqc;
	
	opj_raw_t *raw;

	int *data;
	flag_t *flags;
	int w;
	int h;
	int datasize;
	int flagssize;
	int flags_stride;
} opj_t1_t;

#define MACRO_t1_flags(x,y) t1->flags[((x)*(t1->flags_stride))+(y)]

opj_t1_t* t1_create(opj_common_ptr cinfo);

void t1_destroy(opj_t1_t *t1);

void t1_encode_cblks(opj_t1_t *t1, opj_tcd_tile_t *tile, opj_tcp_t *tcp);

void t1_decode_cblks(opj_t1_t* t1, opj_tcd_tilecomp_t* tilec, opj_tccp_t* tccp);

#endif 
