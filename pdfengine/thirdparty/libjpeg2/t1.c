

#include "opj_includes.h"
#include "t1_luts.h"

static INLINE char t1_getctxno_zc(int f, int orient);
static char t1_getctxno_sc(int f);
static INLINE int t1_getctxno_mag(int f);
static char t1_getspb(int f);
static short t1_getnmsedec_sig(int x, int bitpos);
static short t1_getnmsedec_ref(int x, int bitpos);
static void t1_updateflags(flag_t *flagsp, int s, int stride);

static void t1_enc_sigpass_step(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int bpno,
		int one,
		int *nmsedec,
		char type,
		int vsc);

static INLINE void t1_dec_sigpass_step_raw(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int oneplushalf,
		int vsc);
static INLINE void t1_dec_sigpass_step_mqc(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int oneplushalf);
static INLINE void t1_dec_sigpass_step_mqc_vsc(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int oneplushalf,
		int vsc);

static void t1_enc_sigpass(
		opj_t1_t *t1,
		int bpno,
		int orient,
		int *nmsedec,
		char type,
		int cblksty);

static void t1_dec_sigpass_raw(
		opj_t1_t *t1,
		int bpno,
		int orient,
		int cblksty);
static void t1_dec_sigpass_mqc(
		opj_t1_t *t1,
		int bpno,
		int orient);
static void t1_dec_sigpass_mqc_vsc(
		opj_t1_t *t1,
		int bpno,
		int orient);

static void t1_enc_refpass_step(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int bpno,
		int one,
		int *nmsedec,
		char type,
		int vsc);

static INLINE void t1_dec_refpass_step_raw(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int poshalf,
		int neghalf,
		int vsc);
static INLINE void t1_dec_refpass_step_mqc(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int poshalf,
		int neghalf);
static INLINE void t1_dec_refpass_step_mqc_vsc(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int poshalf,
		int neghalf,
		int vsc);

static void t1_enc_refpass(
		opj_t1_t *t1,
		int bpno,
		int *nmsedec,
		char type,
		int cblksty);

static void t1_dec_refpass_raw(
		opj_t1_t *t1,
		int bpno,
		int cblksty);
static void t1_dec_refpass_mqc(
		opj_t1_t *t1,
		int bpno);
static void t1_dec_refpass_mqc_vsc(
		opj_t1_t *t1,
		int bpno);

static void t1_enc_clnpass_step(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int bpno,
		int one,
		int *nmsedec,
		int partial,
		int vsc);

static void t1_dec_clnpass_step_partial(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int oneplushalf);
static void t1_dec_clnpass_step(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int oneplushalf);
static void t1_dec_clnpass_step_vsc(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int oneplushalf,
		int partial,
		int vsc);

static void t1_enc_clnpass(
		opj_t1_t *t1,
		int bpno,
		int orient,
		int *nmsedec,
		int cblksty);

static void t1_dec_clnpass(
		opj_t1_t *t1,
		int bpno,
		int orient,
		int cblksty);
static double t1_getwmsedec(
		int nmsedec,
		int compno,
		int level,
		int orient,
		int bpno,
		int qmfbid,
		double stepsize,
		int numcomps,
		int mct);

static void t1_encode_cblk(
		opj_t1_t *t1,
		opj_tcd_cblk_enc_t* cblk,
		int orient,
		int compno,
		int level,
		int qmfbid,
		double stepsize,
		int cblksty,
		int numcomps,
		int mct,
		opj_tcd_tile_t * tile);

static void t1_decode_cblk(
		opj_t1_t *t1,
		opj_tcd_cblk_dec_t* cblk,
		int orient,
		int roishift,
		int cblksty);

static char t1_getctxno_zc(int f, int orient) {
	return lut_ctxno_zc[(orient << 8) | (f & T1_SIG_OTH)];
}

static char t1_getctxno_sc(int f) {
	return lut_ctxno_sc[(f & (T1_SIG_PRIM | T1_SGN)) >> 4];
}

static int t1_getctxno_mag(int f) {
	int tmp1 = (f & T1_SIG_OTH) ? T1_CTXNO_MAG + 1 : T1_CTXNO_MAG;
	int tmp2 = (f & T1_REFINE) ? T1_CTXNO_MAG + 2 : tmp1;
	return (tmp2);
}

static char t1_getspb(int f) {
	return lut_spb[(f & (T1_SIG_PRIM | T1_SGN)) >> 4];
}

static short t1_getnmsedec_sig(int x, int bitpos) {
	if (bitpos > T1_NMSEDEC_FRACBITS) {
		return lut_nmsedec_sig[(x >> (bitpos - T1_NMSEDEC_FRACBITS)) & ((1 << T1_NMSEDEC_BITS) - 1)];
	}
	
	return lut_nmsedec_sig0[x & ((1 << T1_NMSEDEC_BITS) - 1)];
}

static short t1_getnmsedec_ref(int x, int bitpos) {
	if (bitpos > T1_NMSEDEC_FRACBITS) {
		return lut_nmsedec_ref[(x >> (bitpos - T1_NMSEDEC_FRACBITS)) & ((1 << T1_NMSEDEC_BITS) - 1)];
	}

    return lut_nmsedec_ref0[x & ((1 << T1_NMSEDEC_BITS) - 1)];
}

static void t1_updateflags(flag_t *flagsp, int s, int stride) {
	flag_t *np = flagsp - stride;
	flag_t *sp = flagsp + stride;

	static const flag_t mod[] = {
		T1_SIG_S, T1_SIG_S|T1_SGN_S,
		T1_SIG_E, T1_SIG_E|T1_SGN_E,
		T1_SIG_W, T1_SIG_W|T1_SGN_W,
		T1_SIG_N, T1_SIG_N|T1_SGN_N
	};

	np[-1] |= T1_SIG_SE;
	np[0]  |= mod[s];
	np[1]  |= T1_SIG_SW;

	flagsp[-1] |= mod[s+2];
	flagsp[0]  |= T1_SIG;
	flagsp[1]  |= mod[s+4];

	sp[-1] |= T1_SIG_NE;
	sp[0]  |= mod[s+6];
	sp[1]  |= T1_SIG_NW;
}

static void t1_enc_sigpass_step(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int bpno,
		int one,
		int *nmsedec,
		char type,
		int vsc)
{
	int v, flag;
	
	opj_mqc_t *mqc = t1->mqc;	
	
	flag = vsc ? ((*flagsp) & (~(T1_SIG_S | T1_SIG_SE | T1_SIG_SW | T1_SGN_S))) : (*flagsp);
	if ((flag & T1_SIG_OTH) && !(flag & (T1_SIG | T1_VISIT))) {
		v = int_abs(*datap) & one ? 1 : 0;
		mqc_setcurctx(mqc, t1_getctxno_zc(flag, orient));	
		if (type == T1_TYPE_RAW) {	
			mqc_bypass_enc(mqc, v);
		} else {
			mqc_encode(mqc, v);
		}
		if (v) {
			v = *datap < 0 ? 1 : 0;
			*nmsedec +=	t1_getnmsedec_sig(int_abs(*datap), bpno + T1_NMSEDEC_FRACBITS);
			mqc_setcurctx(mqc, t1_getctxno_sc(flag));	
			if (type == T1_TYPE_RAW) {	
				mqc_bypass_enc(mqc, v);
			} else {
				mqc_encode(mqc, v ^ t1_getspb(flag));
			}
			t1_updateflags(flagsp, v, t1->flags_stride);
		}
		*flagsp |= T1_VISIT;
	}
}

static INLINE void t1_dec_sigpass_step_raw(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int oneplushalf,
		int vsc)
{
	int v, flag;
	opj_raw_t *raw = t1->raw;	
	
	OPJ_ARG_NOT_USED(orient);
	
	flag = vsc ? ((*flagsp) & (~(T1_SIG_S | T1_SIG_SE | T1_SIG_SW | T1_SGN_S))) : (*flagsp);
	if ((flag & T1_SIG_OTH) && !(flag & (T1_SIG | T1_VISIT))) {
			if (raw_decode(raw)) {
				v = raw_decode(raw);	
				*datap = v ? -oneplushalf : oneplushalf;
				t1_updateflags(flagsp, v, t1->flags_stride);
			}
		*flagsp |= T1_VISIT;
	}
}				

static INLINE void t1_dec_sigpass_step_mqc(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int oneplushalf)
{
	int v, flag;
	
	opj_mqc_t *mqc = t1->mqc;	
	
	flag = *flagsp;
	if ((flag & T1_SIG_OTH) && !(flag & (T1_SIG | T1_VISIT))) {
			mqc_setcurctx(mqc, t1_getctxno_zc(flag, orient));
			if (mqc_decode(mqc)) {
				mqc_setcurctx(mqc, t1_getctxno_sc(flag));
				v = mqc_decode(mqc) ^ t1_getspb(flag);
				*datap = v ? -oneplushalf : oneplushalf;
				t1_updateflags(flagsp, v, t1->flags_stride);
			}
		*flagsp |= T1_VISIT;
	}
}				

static INLINE void t1_dec_sigpass_step_mqc_vsc(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int oneplushalf,
		int vsc)
{
	int v, flag;
	
	opj_mqc_t *mqc = t1->mqc;	
	
	flag = vsc ? ((*flagsp) & (~(T1_SIG_S | T1_SIG_SE | T1_SIG_SW | T1_SGN_S))) : (*flagsp);
	if ((flag & T1_SIG_OTH) && !(flag & (T1_SIG | T1_VISIT))) {
		mqc_setcurctx(mqc, t1_getctxno_zc(flag, orient));
		if (mqc_decode(mqc)) {
			mqc_setcurctx(mqc, t1_getctxno_sc(flag));
			v = mqc_decode(mqc) ^ t1_getspb(flag);
			*datap = v ? -oneplushalf : oneplushalf;
			t1_updateflags(flagsp, v, t1->flags_stride);
		}
		*flagsp |= T1_VISIT;
	}
}				

static void t1_enc_sigpass(
		opj_t1_t *t1,
		int bpno,
		int orient,
		int *nmsedec,
		char type,
		int cblksty)
{
	int i, j, k, one, vsc;
	*nmsedec = 0;
	one = 1 << (bpno + T1_NMSEDEC_FRACBITS);
	for (k = 0; k < t1->h; k += 4) {
		for (i = 0; i < t1->w; ++i) {
			for (j = k; j < k + 4 && j < t1->h; ++j) {
				vsc = ((cblksty & J2K_CCP_CBLKSTY_VSC) && (j == k + 3 || j == t1->h - 1)) ? 1 : 0;
				t1_enc_sigpass_step(
						t1,
						&t1->flags[((j+1) * t1->flags_stride) + i + 1],
						&t1->data[(j * t1->w) + i],
						orient,
						bpno,
						one,
						nmsedec,
						type,
						vsc);
			}
		}
	}
}

static void t1_dec_sigpass_raw(
		opj_t1_t *t1,
		int bpno,
		int orient,
		int cblksty)
{
	int i, j, k, one, half, oneplushalf, vsc;
	one = 1 << bpno;
	half = one >> 1;
	oneplushalf = one | half;
	for (k = 0; k < t1->h; k += 4) {
		for (i = 0; i < t1->w; ++i) {
			for (j = k; j < k + 4 && j < t1->h; ++j) {
				vsc = ((cblksty & J2K_CCP_CBLKSTY_VSC) && (j == k + 3 || j == t1->h - 1)) ? 1 : 0;
				t1_dec_sigpass_step_raw(
						t1,
						&t1->flags[((j+1) * t1->flags_stride) + i + 1],
						&t1->data[(j * t1->w) + i],
						orient,
						oneplushalf,
						vsc);
			}
		}
	}
}				

static void t1_dec_sigpass_mqc(
		opj_t1_t *t1,
		int bpno,
		int orient)
{
	int i, j, k, one, half, oneplushalf;
	int *data1 = t1->data;
	flag_t *flags1 = &t1->flags[1];
	one = 1 << bpno;
	half = one >> 1;
	oneplushalf = one | half;
	for (k = 0; k < (t1->h & ~3); k += 4) {
		for (i = 0; i < t1->w; ++i) {
			int *data2 = data1 + i;
			flag_t *flags2 = flags1 + i;
			flags2 += t1->flags_stride;
			t1_dec_sigpass_step_mqc(t1, flags2, data2, orient, oneplushalf);
			data2 += t1->w;
			flags2 += t1->flags_stride;
			t1_dec_sigpass_step_mqc(t1, flags2, data2, orient, oneplushalf);
			data2 += t1->w;
			flags2 += t1->flags_stride;
			t1_dec_sigpass_step_mqc(t1, flags2, data2, orient, oneplushalf);
			data2 += t1->w;
			flags2 += t1->flags_stride;
			t1_dec_sigpass_step_mqc(t1, flags2, data2, orient, oneplushalf);
			data2 += t1->w;
		}
		data1 += t1->w << 2;
		flags1 += t1->flags_stride << 2;
	}
	for (i = 0; i < t1->w; ++i) {
		int *data2 = data1 + i;
		flag_t *flags2 = flags1 + i;
		for (j = k; j < t1->h; ++j) {
			flags2 += t1->flags_stride;
			t1_dec_sigpass_step_mqc(t1, flags2, data2, orient, oneplushalf);
			data2 += t1->w;
		}
	}
}				

static void t1_dec_sigpass_mqc_vsc(
		opj_t1_t *t1,
		int bpno,
		int orient)
{
	int i, j, k, one, half, oneplushalf, vsc;
	one = 1 << bpno;
	half = one >> 1;
	oneplushalf = one | half;
	for (k = 0; k < t1->h; k += 4) {
		for (i = 0; i < t1->w; ++i) {
			for (j = k; j < k + 4 && j < t1->h; ++j) {
				vsc = (j == k + 3 || j == t1->h - 1) ? 1 : 0;
				t1_dec_sigpass_step_mqc_vsc(
						t1,
						&t1->flags[((j+1) * t1->flags_stride) + i + 1],
						&t1->data[(j * t1->w) + i],
						orient,
						oneplushalf,
						vsc);
			}
		}
	}
}				

static void t1_enc_refpass_step(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int bpno,
		int one,
		int *nmsedec,
		char type,
		int vsc)
{
	int v, flag;
	
	opj_mqc_t *mqc = t1->mqc;	
	
	flag = vsc ? ((*flagsp) & (~(T1_SIG_S | T1_SIG_SE | T1_SIG_SW | T1_SGN_S))) : (*flagsp);
	if ((flag & (T1_SIG | T1_VISIT)) == T1_SIG) {
		*nmsedec += t1_getnmsedec_ref(int_abs(*datap), bpno + T1_NMSEDEC_FRACBITS);
		v = int_abs(*datap) & one ? 1 : 0;
		mqc_setcurctx(mqc, t1_getctxno_mag(flag));	
		if (type == T1_TYPE_RAW) {	
			mqc_bypass_enc(mqc, v);
		} else {
			mqc_encode(mqc, v);
		}
		*flagsp |= T1_REFINE;
	}
}

static INLINE void t1_dec_refpass_step_raw(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int poshalf,
		int neghalf,
		int vsc)
{
	int v, t, flag;
	
	opj_raw_t *raw = t1->raw;	
	
	flag = vsc ? ((*flagsp) & (~(T1_SIG_S | T1_SIG_SE | T1_SIG_SW | T1_SGN_S))) : (*flagsp);
	if ((flag & (T1_SIG | T1_VISIT)) == T1_SIG) {
			v = raw_decode(raw);
		t = v ? poshalf : neghalf;
		*datap += *datap < 0 ? -t : t;
		*flagsp |= T1_REFINE;
	}
}				

static INLINE void t1_dec_refpass_step_mqc(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int poshalf,
		int neghalf)
{
	int v, t, flag;
	
	opj_mqc_t *mqc = t1->mqc;	
	
	flag = *flagsp;
	if ((flag & (T1_SIG | T1_VISIT)) == T1_SIG) {
		mqc_setcurctx(mqc, t1_getctxno_mag(flag));	
			v = mqc_decode(mqc);
		t = v ? poshalf : neghalf;
		*datap += *datap < 0 ? -t : t;
		*flagsp |= T1_REFINE;
		}
}				

static INLINE void t1_dec_refpass_step_mqc_vsc(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int poshalf,
		int neghalf,
		int vsc)
{
	int v, t, flag;
	
	opj_mqc_t *mqc = t1->mqc;	
	
	flag = vsc ? ((*flagsp) & (~(T1_SIG_S | T1_SIG_SE | T1_SIG_SW | T1_SGN_S))) : (*flagsp);
	if ((flag & (T1_SIG | T1_VISIT)) == T1_SIG) {
		mqc_setcurctx(mqc, t1_getctxno_mag(flag));	
		v = mqc_decode(mqc);
		t = v ? poshalf : neghalf;
		*datap += *datap < 0 ? -t : t;
		*flagsp |= T1_REFINE;
	}
}				

static void t1_enc_refpass(
		opj_t1_t *t1,
		int bpno,
		int *nmsedec,
		char type,
		int cblksty)
{
	int i, j, k, one, vsc;
	*nmsedec = 0;
	one = 1 << (bpno + T1_NMSEDEC_FRACBITS);
	for (k = 0; k < t1->h; k += 4) {
		for (i = 0; i < t1->w; ++i) {
			for (j = k; j < k + 4 && j < t1->h; ++j) {
				vsc = ((cblksty & J2K_CCP_CBLKSTY_VSC) && (j == k + 3 || j == t1->h - 1)) ? 1 : 0;
				t1_enc_refpass_step(
						t1,
						&t1->flags[((j+1) * t1->flags_stride) + i + 1],
						&t1->data[(j * t1->w) + i],
						bpno,
						one,
						nmsedec,
						type,
						vsc);
			}
		}
	}
}

static void t1_dec_refpass_raw(
		opj_t1_t *t1,
		int bpno,
		int cblksty)
{
	int i, j, k, one, poshalf, neghalf;
	int vsc;
	one = 1 << bpno;
	poshalf = one >> 1;
	neghalf = bpno > 0 ? -poshalf : -1;
	for (k = 0; k < t1->h; k += 4) {
		for (i = 0; i < t1->w; ++i) {
			for (j = k; j < k + 4 && j < t1->h; ++j) {
				vsc = ((cblksty & J2K_CCP_CBLKSTY_VSC) && (j == k + 3 || j == t1->h - 1)) ? 1 : 0;
				t1_dec_refpass_step_raw(
						t1,
						&t1->flags[((j+1) * t1->flags_stride) + i + 1],
						&t1->data[(j * t1->w) + i],
						poshalf,
						neghalf,
						vsc);
			}
		}
	}
}				

static void t1_dec_refpass_mqc(
		opj_t1_t *t1,
		int bpno)
{
	int i, j, k, one, poshalf, neghalf;
	int *data1 = t1->data;
	flag_t *flags1 = &t1->flags[1];
	one = 1 << bpno;
	poshalf = one >> 1;
	neghalf = bpno > 0 ? -poshalf : -1;
	for (k = 0; k < (t1->h & ~3); k += 4) {
		for (i = 0; i < t1->w; ++i) {
			int *data2 = data1 + i;
			flag_t *flags2 = flags1 + i;
			flags2 += t1->flags_stride;
			t1_dec_refpass_step_mqc(t1, flags2, data2, poshalf, neghalf);
			data2 += t1->w;
			flags2 += t1->flags_stride;
			t1_dec_refpass_step_mqc(t1, flags2, data2, poshalf, neghalf);
			data2 += t1->w;
			flags2 += t1->flags_stride;
			t1_dec_refpass_step_mqc(t1, flags2, data2, poshalf, neghalf);
			data2 += t1->w;
			flags2 += t1->flags_stride;
			t1_dec_refpass_step_mqc(t1, flags2, data2, poshalf, neghalf);
			data2 += t1->w;
		}
		data1 += t1->w << 2;
		flags1 += t1->flags_stride << 2;
	}
	for (i = 0; i < t1->w; ++i) {
		int *data2 = data1 + i;
		flag_t *flags2 = flags1 + i;
		for (j = k; j < t1->h; ++j) {
			flags2 += t1->flags_stride;
			t1_dec_refpass_step_mqc(t1, flags2, data2, poshalf, neghalf);
			data2 += t1->w;
		}
	}
}				

static void t1_dec_refpass_mqc_vsc(
		opj_t1_t *t1,
		int bpno)
{
	int i, j, k, one, poshalf, neghalf;
	int vsc;
	one = 1 << bpno;
	poshalf = one >> 1;
	neghalf = bpno > 0 ? -poshalf : -1;
	for (k = 0; k < t1->h; k += 4) {
		for (i = 0; i < t1->w; ++i) {
			for (j = k; j < k + 4 && j < t1->h; ++j) {
				vsc = ((j == k + 3 || j == t1->h - 1)) ? 1 : 0;
				t1_dec_refpass_step_mqc_vsc(
						t1,
						&t1->flags[((j+1) * t1->flags_stride) + i + 1],
						&t1->data[(j * t1->w) + i],
						poshalf,
						neghalf,
						vsc);
			}
		}
	}
}				

static void t1_enc_clnpass_step(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int bpno,
		int one,
		int *nmsedec,
		int partial,
		int vsc)
{
	int v, flag;
	
	opj_mqc_t *mqc = t1->mqc;	
	
	flag = vsc ? ((*flagsp) & (~(T1_SIG_S | T1_SIG_SE | T1_SIG_SW | T1_SGN_S))) : (*flagsp);
	if (partial) {
		goto LABEL_PARTIAL;
	}
	if (!(*flagsp & (T1_SIG | T1_VISIT))) {
		mqc_setcurctx(mqc, t1_getctxno_zc(flag, orient));
		v = int_abs(*datap) & one ? 1 : 0;
		mqc_encode(mqc, v);
		if (v) {
LABEL_PARTIAL:
			*nmsedec += t1_getnmsedec_sig(int_abs(*datap), bpno + T1_NMSEDEC_FRACBITS);
			mqc_setcurctx(mqc, t1_getctxno_sc(flag));
			v = *datap < 0 ? 1 : 0;
			mqc_encode(mqc, v ^ t1_getspb(flag));
			t1_updateflags(flagsp, v, t1->flags_stride);
		}
	}
	*flagsp &= ~T1_VISIT;
}

static void t1_dec_clnpass_step_partial(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int oneplushalf)
{
	int v, flag;
	opj_mqc_t *mqc = t1->mqc;	
	
	OPJ_ARG_NOT_USED(orient);
	
	flag = *flagsp;
	mqc_setcurctx(mqc, t1_getctxno_sc(flag));
	v = mqc_decode(mqc) ^ t1_getspb(flag);
	*datap = v ? -oneplushalf : oneplushalf;
	t1_updateflags(flagsp, v, t1->flags_stride);
	*flagsp &= ~T1_VISIT;
}				

static void t1_dec_clnpass_step(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int oneplushalf)
{
	int v, flag;
	
	opj_mqc_t *mqc = t1->mqc;	
	
	flag = *flagsp;
	if (!(flag & (T1_SIG | T1_VISIT))) {
		mqc_setcurctx(mqc, t1_getctxno_zc(flag, orient));
		if (mqc_decode(mqc)) {
			mqc_setcurctx(mqc, t1_getctxno_sc(flag));
			v = mqc_decode(mqc) ^ t1_getspb(flag);
			*datap = v ? -oneplushalf : oneplushalf;
			t1_updateflags(flagsp, v, t1->flags_stride);
		}
	}
	*flagsp &= ~T1_VISIT;
}				

static void t1_dec_clnpass_step_vsc(
		opj_t1_t *t1,
		flag_t *flagsp,
		int *datap,
		int orient,
		int oneplushalf,
		int partial,
		int vsc)
{
	int v, flag;
	
	opj_mqc_t *mqc = t1->mqc;	
	
	flag = vsc ? ((*flagsp) & (~(T1_SIG_S | T1_SIG_SE | T1_SIG_SW | T1_SGN_S))) : (*flagsp);
	if (partial) {
		goto LABEL_PARTIAL;
	}
	if (!(flag & (T1_SIG | T1_VISIT))) {
		mqc_setcurctx(mqc, t1_getctxno_zc(flag, orient));
		if (mqc_decode(mqc)) {
LABEL_PARTIAL:
			mqc_setcurctx(mqc, t1_getctxno_sc(flag));
			v = mqc_decode(mqc) ^ t1_getspb(flag);
			*datap = v ? -oneplushalf : oneplushalf;
			t1_updateflags(flagsp, v, t1->flags_stride);
		}
	}
	*flagsp &= ~T1_VISIT;
}

static void t1_enc_clnpass(
		opj_t1_t *t1,
		int bpno,
		int orient,
		int *nmsedec,
		int cblksty)
{
	int i, j, k, one, agg, runlen, vsc;
	
	opj_mqc_t *mqc = t1->mqc;	
	
	*nmsedec = 0;
	one = 1 << (bpno + T1_NMSEDEC_FRACBITS);
	for (k = 0; k < t1->h; k += 4) {
		for (i = 0; i < t1->w; ++i) {
			if (k + 3 < t1->h) {
				if (cblksty & J2K_CCP_CBLKSTY_VSC) {
					agg = !(MACRO_t1_flags(1 + k,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH)
						|| MACRO_t1_flags(1 + k + 1,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH)
						|| MACRO_t1_flags(1 + k + 2,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH)
						|| (MACRO_t1_flags(1 + k + 3,1 + i) 
						& (~(T1_SIG_S | T1_SIG_SE | T1_SIG_SW |	T1_SGN_S))) & (T1_SIG | T1_VISIT | T1_SIG_OTH));
				} else {
					agg = !(MACRO_t1_flags(1 + k,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH)
						|| MACRO_t1_flags(1 + k + 1,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH)
						|| MACRO_t1_flags(1 + k + 2,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH)
						|| MACRO_t1_flags(1 + k + 3,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH));
				}
			} else {
				agg = 0;
			}
			if (agg) {
				for (runlen = 0; runlen < 4; ++runlen) {
					if (int_abs(t1->data[((k + runlen)*t1->w) + i]) & one)
						break;
				}
				mqc_setcurctx(mqc, T1_CTXNO_AGG);
				mqc_encode(mqc, runlen != 4);
				if (runlen == 4) {
					continue;
				}
				mqc_setcurctx(mqc, T1_CTXNO_UNI);
				mqc_encode(mqc, runlen >> 1);
				mqc_encode(mqc, runlen & 1);
			} else {
				runlen = 0;
			}
			for (j = k + runlen; j < k + 4 && j < t1->h; ++j) {
				vsc = ((cblksty & J2K_CCP_CBLKSTY_VSC) && (j == k + 3 || j == t1->h - 1)) ? 1 : 0;
				t1_enc_clnpass_step(
						t1,
						&t1->flags[((j+1) * t1->flags_stride) + i + 1],
						&t1->data[(j * t1->w) + i],
						orient,
						bpno,
						one,
						nmsedec,
						agg && (j == k + runlen),
						vsc);
			}
		}
	}
}

static void t1_dec_clnpass(
		opj_t1_t *t1,
		int bpno,
		int orient,
		int cblksty)
{
	int i, j, k, one, half, oneplushalf, agg, runlen, vsc;
	int segsym = cblksty & J2K_CCP_CBLKSTY_SEGSYM;
	
	opj_mqc_t *mqc = t1->mqc;	
	
	one = 1 << bpno;
	half = one >> 1;
	oneplushalf = one | half;
	if (cblksty & J2K_CCP_CBLKSTY_VSC) {
	for (k = 0; k < t1->h; k += 4) {
		for (i = 0; i < t1->w; ++i) {
			if (k + 3 < t1->h) {
					agg = !(MACRO_t1_flags(1 + k,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH)
						|| MACRO_t1_flags(1 + k + 1,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH)
						|| MACRO_t1_flags(1 + k + 2,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH)
						|| (MACRO_t1_flags(1 + k + 3,1 + i) 
						& (~(T1_SIG_S | T1_SIG_SE | T1_SIG_SW |	T1_SGN_S))) & (T1_SIG | T1_VISIT | T1_SIG_OTH));
				} else {
				agg = 0;
			}
			if (agg) {
				mqc_setcurctx(mqc, T1_CTXNO_AGG);
				if (!mqc_decode(mqc)) {
					continue;
				}
				mqc_setcurctx(mqc, T1_CTXNO_UNI);
				runlen = mqc_decode(mqc);
				runlen = (runlen << 1) | mqc_decode(mqc);
			} else {
				runlen = 0;
			}
			for (j = k + runlen; j < k + 4 && j < t1->h; ++j) {
					vsc = (j == k + 3 || j == t1->h - 1) ? 1 : 0;
					t1_dec_clnpass_step_vsc(
						t1,
						&t1->flags[((j+1) * t1->flags_stride) + i + 1],
						&t1->data[(j * t1->w) + i],
						orient,
						oneplushalf,
						agg && (j == k + runlen),
						vsc);
			}
		}
	}
	} else {
		int *data1 = t1->data;
		flag_t *flags1 = &t1->flags[1];
		for (k = 0; k < (t1->h & ~3); k += 4) {
			for (i = 0; i < t1->w; ++i) {
				int *data2 = data1 + i;
				flag_t *flags2 = flags1 + i;
				agg = !(MACRO_t1_flags(1 + k,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH)
					|| MACRO_t1_flags(1 + k + 1,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH)
					|| MACRO_t1_flags(1 + k + 2,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH)
					|| MACRO_t1_flags(1 + k + 3,1 + i) & (T1_SIG | T1_VISIT | T1_SIG_OTH));
				if (agg) {
					mqc_setcurctx(mqc, T1_CTXNO_AGG);
					if (!mqc_decode(mqc)) {
						continue;
					}
					mqc_setcurctx(mqc, T1_CTXNO_UNI);
					runlen = mqc_decode(mqc);
					runlen = (runlen << 1) | mqc_decode(mqc);
					flags2 += runlen * t1->flags_stride;
					data2 += runlen * t1->w;
					for (j = k + runlen; j < k + 4 && j < t1->h; ++j) {
						flags2 += t1->flags_stride;
						if (agg && (j == k + runlen)) {
							t1_dec_clnpass_step_partial(t1, flags2, data2, orient, oneplushalf);
						} else {
							t1_dec_clnpass_step(t1, flags2, data2, orient, oneplushalf);
						}
						data2 += t1->w;
					}
				} else {
					flags2 += t1->flags_stride;
					t1_dec_clnpass_step(t1, flags2, data2, orient, oneplushalf);
					data2 += t1->w;
					flags2 += t1->flags_stride;
					t1_dec_clnpass_step(t1, flags2, data2, orient, oneplushalf);
					data2 += t1->w;
					flags2 += t1->flags_stride;
					t1_dec_clnpass_step(t1, flags2, data2, orient, oneplushalf);
					data2 += t1->w;
					flags2 += t1->flags_stride;
					t1_dec_clnpass_step(t1, flags2, data2, orient, oneplushalf);
					data2 += t1->w;
				}
			}
			data1 += t1->w << 2;
			flags1 += t1->flags_stride << 2;
		}
		for (i = 0; i < t1->w; ++i) {
			int *data2 = data1 + i;
			flag_t *flags2 = flags1 + i;
			for (j = k; j < t1->h; ++j) {
				flags2 += t1->flags_stride;
				t1_dec_clnpass_step(t1, flags2, data2, orient, oneplushalf);
				data2 += t1->w;
			}
		}
	}

	if (segsym) {
		int v = 0;
		mqc_setcurctx(mqc, T1_CTXNO_UNI);
		v = mqc_decode(mqc);
		v = (v << 1) | mqc_decode(mqc);
		v = (v << 1) | mqc_decode(mqc);
		v = (v << 1) | mqc_decode(mqc);
		
	}
}				

static double t1_getwmsedec(
		int nmsedec,
		int compno,
		int level,
		int orient,
		int bpno,
		int qmfbid,
		double stepsize,
		int numcomps,
		int mct)
{
	double w1, w2, wmsedec;
	if (qmfbid == 1) {
		w1 = (mct && numcomps==3) ? mct_getnorm(compno) : 1.0;
		w2 = dwt_getnorm(level, orient);
	} else {			
		w1 = (mct && numcomps==3) ? mct_getnorm_real(compno) : 1.0;
		w2 = dwt_getnorm_real(level, orient);
	}
	wmsedec = w1 * w2 * stepsize * (1 << bpno);
	wmsedec *= wmsedec * nmsedec / 8192.0;
	
	return wmsedec;
}

static opj_bool allocate_buffers(
		opj_t1_t *t1,
		int w,
		int h)
{
	int datasize=w * h;
	int flagssize;

	if(datasize > t1->datasize){
		opj_aligned_free(t1->data);
		t1->data = (int*) opj_aligned_malloc(datasize * sizeof(int));
		if(!t1->data){
			return OPJ_FALSE;
		}
		t1->datasize=datasize;
	}
	memset(t1->data,0,datasize * sizeof(int));

	t1->flags_stride=w+2;
	flagssize=t1->flags_stride * (h+2);

	if(flagssize > t1->flagssize){
		opj_aligned_free(t1->flags);
		t1->flags = (flag_t*) opj_aligned_malloc(flagssize * sizeof(flag_t));
		if(!t1->flags){
			return OPJ_FALSE;
		}
		t1->flagssize=flagssize;
	}
	memset(t1->flags,0,flagssize * sizeof(flag_t));

	t1->w=w;
	t1->h=h;

	return OPJ_TRUE;
}

static void t1_encode_cblk(
		opj_t1_t *t1,
		opj_tcd_cblk_enc_t* cblk,
		int orient,
		int compno,
		int level,
		int qmfbid,
		double stepsize,
		int cblksty,
		int numcomps,
		int mct,
		opj_tcd_tile_t * tile)
{
	double cumwmsedec = 0.0;

	opj_mqc_t *mqc = t1->mqc;	

	int passno, bpno, passtype;
	int nmsedec = 0;
	int i, max;
	char type = T1_TYPE_MQ;
	double tempwmsedec;

	max = 0;
	for (i = 0; i < t1->w * t1->h; ++i) {
		int tmp = abs(t1->data[i]);
		max = int_max(max, tmp);
	}

	cblk->numbps = max ? (int_floorlog2(max) + 1) - T1_NMSEDEC_FRACBITS : 0;
	
	bpno = cblk->numbps - 1;
	passtype = 2;
	
	mqc_resetstates(mqc);
	mqc_setstate(mqc, T1_CTXNO_UNI, 0, 46);
	mqc_setstate(mqc, T1_CTXNO_AGG, 0, 3);
	mqc_setstate(mqc, T1_CTXNO_ZC, 0, 4);
	mqc_init_enc(mqc, cblk->data);
	
	for (passno = 0; bpno >= 0; ++passno) {
		opj_tcd_pass_t *pass = &cblk->passes[passno];
		int correction = 3;
		type = ((bpno < (cblk->numbps - 4)) && (passtype < 2) && (cblksty & J2K_CCP_CBLKSTY_LAZY)) ? T1_TYPE_RAW : T1_TYPE_MQ;
		
		switch (passtype) {
			case 0:
				t1_enc_sigpass(t1, bpno, orient, &nmsedec, type, cblksty);
				break;
			case 1:
				t1_enc_refpass(t1, bpno, &nmsedec, type, cblksty);
				break;
			case 2:
				t1_enc_clnpass(t1, bpno, orient, &nmsedec, cblksty);
				
				if (cblksty & J2K_CCP_CBLKSTY_SEGSYM)
					mqc_segmark_enc(mqc);
				break;
		}
		
		
		tempwmsedec = t1_getwmsedec(nmsedec, compno, level, orient, bpno, qmfbid, stepsize, numcomps, mct);
		cumwmsedec += tempwmsedec;
		tile->distotile += tempwmsedec;
		
		
		if ((cblksty & J2K_CCP_CBLKSTY_TERMALL)	&& !((passtype == 2) && (bpno - 1 < 0))) {
			if (type == T1_TYPE_RAW) {
				mqc_flush(mqc);
				correction = 1;
				
			} else {			
				mqc_flush(mqc);
				correction = 1;
			}
			pass->term = 1;
		} else {
			if (((bpno < (cblk->numbps - 4) && (passtype > 0)) 
				|| ((bpno == (cblk->numbps - 4)) && (passtype == 2))) && (cblksty & J2K_CCP_CBLKSTY_LAZY)) {
				if (type == T1_TYPE_RAW) {
					mqc_flush(mqc);
					correction = 1;
					
				} else {		
					mqc_flush(mqc);
					correction = 1;
				}
				pass->term = 1;
			} else {
				pass->term = 0;
			}
		}
		
		if (++passtype == 3) {
			passtype = 0;
			bpno--;
		}
		
		if (pass->term && bpno > 0) {
			type = ((bpno < (cblk->numbps - 4)) && (passtype < 2) && (cblksty & J2K_CCP_CBLKSTY_LAZY)) ? T1_TYPE_RAW : T1_TYPE_MQ;
			if (type == T1_TYPE_RAW)
				mqc_bypass_init_enc(mqc);
			else
				mqc_restart_init_enc(mqc);
		}
		
		pass->distortiondec = cumwmsedec;
		pass->rate = mqc_numbytes(mqc) + correction;	
		
		
		if (cblksty & J2K_CCP_CBLKSTY_RESET)
			mqc_reset_enc(mqc);
	}
	
	
	if (cblksty & J2K_CCP_CBLKSTY_PTERM)
		mqc_erterm_enc(mqc);
	else  if (!(cblksty & J2K_CCP_CBLKSTY_LAZY))
		mqc_flush(mqc);
	
	cblk->totalpasses = passno;

	for (passno = 0; passno<cblk->totalpasses; passno++) {
		opj_tcd_pass_t *pass = &cblk->passes[passno];
		if (pass->rate > mqc_numbytes(mqc))
			pass->rate = mqc_numbytes(mqc);
		
		if((pass->rate>1) && (cblk->data[pass->rate - 1] == 0xFF)){
			pass->rate--;
		}
		pass->len = pass->rate - (passno == 0 ? 0 : cblk->passes[passno - 1].rate);		
	}
}

static void t1_decode_cblk(
		opj_t1_t *t1,
		opj_tcd_cblk_dec_t* cblk,
		int orient,
		int roishift,
		int cblksty)
{
	opj_raw_t *raw = t1->raw;	
	opj_mqc_t *mqc = t1->mqc;	

	int bpno, passtype;
	int segno, passno;
	char type = T1_TYPE_MQ; 

	if(!allocate_buffers(
				t1,
				cblk->x1 - cblk->x0,
				cblk->y1 - cblk->y0))
	{
		return;
	}

	bpno = roishift + cblk->numbps - 1;
	passtype = 2;
	
	mqc_resetstates(mqc);
	mqc_setstate(mqc, T1_CTXNO_UNI, 0, 46);
	mqc_setstate(mqc, T1_CTXNO_AGG, 0, 3);
	mqc_setstate(mqc, T1_CTXNO_ZC, 0, 4);
	
	for (segno = 0; segno < cblk->numsegs; ++segno) {
		opj_tcd_seg_t *seg = &cblk->segs[segno];
		
		
		type = ((bpno <= (cblk->numbps - 1) - 4) && (passtype < 2) && (cblksty & J2K_CCP_CBLKSTY_LAZY)) ? T1_TYPE_RAW : T1_TYPE_MQ;
		
		if(seg->data == NULL){
			continue;
		}
		if (type == T1_TYPE_RAW) {
			raw_init_dec(raw, (*seg->data) + seg->dataindex, seg->len);
		} else {
			mqc_init_dec(mqc, (*seg->data) + seg->dataindex, seg->len);
		}
		
		for (passno = 0; passno < seg->numpasses; ++passno) {
			switch (passtype) {
				case 0:
					if (type == T1_TYPE_RAW) {
						t1_dec_sigpass_raw(t1, bpno+1, orient, cblksty);
					} else {
						if (cblksty & J2K_CCP_CBLKSTY_VSC) {
							t1_dec_sigpass_mqc_vsc(t1, bpno+1, orient);
						} else {
							t1_dec_sigpass_mqc(t1, bpno+1, orient);
						}
					}
					break;
				case 1:
					if (type == T1_TYPE_RAW) {
						t1_dec_refpass_raw(t1, bpno+1, cblksty);
					} else {
						if (cblksty & J2K_CCP_CBLKSTY_VSC) {
							t1_dec_refpass_mqc_vsc(t1, bpno+1);
						} else {
							t1_dec_refpass_mqc(t1, bpno+1);
						}
					}
					break;
				case 2:
					t1_dec_clnpass(t1, bpno+1, orient, cblksty);
					break;
			}
			
			if ((cblksty & J2K_CCP_CBLKSTY_RESET) && type == T1_TYPE_MQ) {
				mqc_resetstates(mqc);
				mqc_setstate(mqc, T1_CTXNO_UNI, 0, 46);				
				mqc_setstate(mqc, T1_CTXNO_AGG, 0, 3);
				mqc_setstate(mqc, T1_CTXNO_ZC, 0, 4);
			}
			if (++passtype == 3) {
				passtype = 0;
				bpno--;
			}
		}
	}
}

opj_t1_t* t1_create(opj_common_ptr cinfo) {
	opj_t1_t *t1 = (opj_t1_t*) opj_malloc(sizeof(opj_t1_t));
	if(!t1)
		return NULL;

	t1->cinfo = cinfo;
	
	t1->mqc = mqc_create();
	t1->raw = raw_create();

	t1->data=NULL;
	t1->flags=NULL;
	t1->datasize=0;
	t1->flagssize=0;

	return t1;
}

void t1_destroy(opj_t1_t *t1) {
	if(t1) {
		
		mqc_destroy(t1->mqc);
		raw_destroy(t1->raw);
		opj_aligned_free(t1->data);
		opj_aligned_free(t1->flags);
		opj_free(t1);
	}
}

void t1_encode_cblks(
		opj_t1_t *t1,
		opj_tcd_tile_t *tile,
		opj_tcp_t *tcp)
{
	int compno, resno, bandno, precno, cblkno;

	tile->distotile = 0;		

	for (compno = 0; compno < tile->numcomps; ++compno) {
		opj_tcd_tilecomp_t* tilec = &tile->comps[compno];
		opj_tccp_t* tccp = &tcp->tccps[compno];
		int tile_w = tilec->x1 - tilec->x0;

		for (resno = 0; resno < tilec->numresolutions; ++resno) {
			opj_tcd_resolution_t *res = &tilec->resolutions[resno];

			for (bandno = 0; bandno < res->numbands; ++bandno) {
				opj_tcd_band_t* restrict band = &res->bands[bandno];
        int bandconst = 8192 * 8192 / ((int) floor(band->stepsize * 8192));

				for (precno = 0; precno < res->pw * res->ph; ++precno) {
					opj_tcd_precinct_t *prc = &band->precincts[precno];

					for (cblkno = 0; cblkno < prc->cw * prc->ch; ++cblkno) {
						opj_tcd_cblk_enc_t* cblk = &prc->cblks.enc[cblkno];
						int* restrict datap;
						int* restrict tiledp;
						int cblk_w;
						int cblk_h;
						int i, j;

						int x = cblk->x0 - band->x0;
						int y = cblk->y0 - band->y0;
						if (band->bandno & 1) {
							opj_tcd_resolution_t *pres = &tilec->resolutions[resno - 1];
							x += pres->x1 - pres->x0;
						}
						if (band->bandno & 2) {
							opj_tcd_resolution_t *pres = &tilec->resolutions[resno - 1];
							y += pres->y1 - pres->y0;
						}

						if(!allocate_buffers(
									t1,
									cblk->x1 - cblk->x0,
									cblk->y1 - cblk->y0))
						{
							return;
						}

						datap=t1->data;
						cblk_w = t1->w;
						cblk_h = t1->h;

						tiledp=&tilec->data[(y * tile_w) + x];
						if (tccp->qmfbid == 1) {
							for (j = 0; j < cblk_h; ++j) {
								for (i = 0; i < cblk_w; ++i) {
									int tmp = tiledp[(j * tile_w) + i];
									datap[(j * cblk_w) + i] = tmp << T1_NMSEDEC_FRACBITS;
								}
							}
						} else {		
							for (j = 0; j < cblk_h; ++j) {
								for (i = 0; i < cblk_w; ++i) {
									int tmp = tiledp[(j * tile_w) + i];
									datap[(j * cblk_w) + i] =
										fix_mul(
										tmp,
										bandconst) >> (11 - T1_NMSEDEC_FRACBITS);
								}
							}
						}

						t1_encode_cblk(
								t1,
								cblk,
								band->bandno,
								compno,
								tilec->numresolutions - 1 - resno,
								tccp->qmfbid,
								band->stepsize,
								tccp->cblksty,
								tile->numcomps,
								tcp->mct,
								tile);

					} 
				} 
			} 
		} 
	} 
}

void t1_decode_cblks(
		opj_t1_t* t1,
		opj_tcd_tilecomp_t* tilec,
		opj_tccp_t* tccp)
{
	int resno, bandno, precno, cblkno;

	int tile_w = tilec->x1 - tilec->x0;

	for (resno = 0; resno < tilec->numresolutions; ++resno) {
		opj_tcd_resolution_t* res = &tilec->resolutions[resno];

		for (bandno = 0; bandno < res->numbands; ++bandno) {
			opj_tcd_band_t* restrict band = &res->bands[bandno];

			for (precno = 0; precno < res->pw * res->ph; ++precno) {
				opj_tcd_precinct_t* precinct = &band->precincts[precno];

				for (cblkno = 0; cblkno < precinct->cw * precinct->ch; ++cblkno) {
					opj_tcd_cblk_dec_t* cblk = &precinct->cblks.dec[cblkno];
					int* restrict datap;
					int cblk_w, cblk_h;
					int x, y;
					int i, j;

					t1_decode_cblk(
							t1,
							cblk,
							band->bandno,
							tccp->roishift,
							tccp->cblksty);

					x = cblk->x0 - band->x0;
					y = cblk->y0 - band->y0;
					if (band->bandno & 1) {
						opj_tcd_resolution_t* pres = &tilec->resolutions[resno - 1];
						x += pres->x1 - pres->x0;
					}
					if (band->bandno & 2) {
						opj_tcd_resolution_t* pres = &tilec->resolutions[resno - 1];
						y += pres->y1 - pres->y0;
					}

					datap=t1->data;
					cblk_w = t1->w;
					cblk_h = t1->h;

					if (tccp->roishift) {
						int thresh = 1 << tccp->roishift;
						for (j = 0; j < cblk_h; ++j) {
							for (i = 0; i < cblk_w; ++i) {
								int val = datap[(j * cblk_w) + i];
								int mag = abs(val);
								if (mag >= thresh) {
									mag >>= tccp->roishift;
									datap[(j * cblk_w) + i] = val < 0 ? -mag : mag;
								}
							}
						}
					}

					if (tccp->qmfbid == 1) {
						int* restrict tiledp = &tilec->data[(y * tile_w) + x];
						for (j = 0; j < cblk_h; ++j) {
							for (i = 0; i < cblk_w; ++i) {
								int tmp = datap[(j * cblk_w) + i];
								((int*)tiledp)[(j * tile_w) + i] = tmp / 2;
							}
						}
					} else {		
						float* restrict tiledp = (float*) &tilec->data[(y * tile_w) + x];
						for (j = 0; j < cblk_h; ++j) {
							float* restrict tiledp2 = tiledp;
							for (i = 0; i < cblk_w; ++i) {
								float tmp = *datap * band->stepsize;
								*tiledp2 = tmp;
								datap++;
								tiledp2++;
							}
							tiledp += tile_w;
						}
					}
					opj_free(cblk->data);
					opj_free(cblk->segs);
				} 
				opj_free(precinct->cblks.dec);
			} 
		} 
	} 
}

