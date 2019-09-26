

#include "tiffiop.h"
#ifdef PIXARLOG_SUPPORT

#include "tif_predict.h"
#include "zlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  TSIZE	 2048		
#define  TSIZEP1 2049		
#define  ONE	 1250		
#define  RATIO	 1.004		

#define CODE_MASK 0x7ff         

static float  Fltsize;
static float  LogK1, LogK2;

#define REPEAT(n, op)   { int i; i=n; do { i--; op; } while (i>0); }

static void
horizontalAccumulateF(uint16 *wp, int n, int stride, float *op, 
	float *ToLinearF)
{
    register unsigned int  cr, cg, cb, ca, mask;
    register float  t0, t1, t2, t3;

    if (n >= stride) {
	mask = CODE_MASK;
	if (stride == 3) {
	    t0 = ToLinearF[cr = wp[0]];
	    t1 = ToLinearF[cg = wp[1]];
	    t2 = ToLinearF[cb = wp[2]];
	    op[0] = t0;
	    op[1] = t1;
	    op[2] = t2;
	    n -= 3;
	    while (n > 0) {
		wp += 3;
		op += 3;
		n -= 3;
		t0 = ToLinearF[(cr += wp[0]) & mask];
		t1 = ToLinearF[(cg += wp[1]) & mask];
		t2 = ToLinearF[(cb += wp[2]) & mask];
		op[0] = t0;
		op[1] = t1;
		op[2] = t2;
	    }
	} else if (stride == 4) {
	    t0 = ToLinearF[cr = wp[0]];
	    t1 = ToLinearF[cg = wp[1]];
	    t2 = ToLinearF[cb = wp[2]];
	    t3 = ToLinearF[ca = wp[3]];
	    op[0] = t0;
	    op[1] = t1;
	    op[2] = t2;
	    op[3] = t3;
	    n -= 4;
	    while (n > 0) {
		wp += 4;
		op += 4;
		n -= 4;
		t0 = ToLinearF[(cr += wp[0]) & mask];
		t1 = ToLinearF[(cg += wp[1]) & mask];
		t2 = ToLinearF[(cb += wp[2]) & mask];
		t3 = ToLinearF[(ca += wp[3]) & mask];
		op[0] = t0;
		op[1] = t1;
		op[2] = t2;
		op[3] = t3;
	    }
	} else {
	    REPEAT(stride, *op = ToLinearF[*wp&mask]; wp++; op++)
	    n -= stride;
	    while (n > 0) {
		REPEAT(stride,
		    wp[stride] += *wp; *op = ToLinearF[*wp&mask]; wp++; op++)
		n -= stride;
	    }
	}
    }
}

static void
horizontalAccumulate12(uint16 *wp, int n, int stride, int16 *op,
	float *ToLinearF)
{
    register unsigned int  cr, cg, cb, ca, mask;
    register float  t0, t1, t2, t3;

#define SCALE12 2048.0F
#define CLAMP12(t) (((t) < 3071) ? (uint16) (t) : 3071)

    if (n >= stride) {
	mask = CODE_MASK;
	if (stride == 3) {
	    t0 = ToLinearF[cr = wp[0]] * SCALE12;
	    t1 = ToLinearF[cg = wp[1]] * SCALE12;
	    t2 = ToLinearF[cb = wp[2]] * SCALE12;
	    op[0] = CLAMP12(t0);
	    op[1] = CLAMP12(t1);
	    op[2] = CLAMP12(t2);
	    n -= 3;
	    while (n > 0) {
		wp += 3;
		op += 3;
		n -= 3;
		t0 = ToLinearF[(cr += wp[0]) & mask] * SCALE12;
		t1 = ToLinearF[(cg += wp[1]) & mask] * SCALE12;
		t2 = ToLinearF[(cb += wp[2]) & mask] * SCALE12;
		op[0] = CLAMP12(t0);
		op[1] = CLAMP12(t1);
		op[2] = CLAMP12(t2);
	    }
	} else if (stride == 4) {
	    t0 = ToLinearF[cr = wp[0]] * SCALE12;
	    t1 = ToLinearF[cg = wp[1]] * SCALE12;
	    t2 = ToLinearF[cb = wp[2]] * SCALE12;
	    t3 = ToLinearF[ca = wp[3]] * SCALE12;
	    op[0] = CLAMP12(t0);
	    op[1] = CLAMP12(t1);
	    op[2] = CLAMP12(t2);
	    op[3] = CLAMP12(t3);
	    n -= 4;
	    while (n > 0) {
		wp += 4;
		op += 4;
		n -= 4;
		t0 = ToLinearF[(cr += wp[0]) & mask] * SCALE12;
		t1 = ToLinearF[(cg += wp[1]) & mask] * SCALE12;
		t2 = ToLinearF[(cb += wp[2]) & mask] * SCALE12;
		t3 = ToLinearF[(ca += wp[3]) & mask] * SCALE12;
		op[0] = CLAMP12(t0);
		op[1] = CLAMP12(t1);
		op[2] = CLAMP12(t2);
		op[3] = CLAMP12(t3);
	    }
	} else {
	    REPEAT(stride, t0 = ToLinearF[*wp&mask] * SCALE12;
                           *op = CLAMP12(t0); wp++; op++)
	    n -= stride;
	    while (n > 0) {
		REPEAT(stride,
		    wp[stride] += *wp; t0 = ToLinearF[wp[stride]&mask]*SCALE12;
		    *op = CLAMP12(t0);  wp++; op++)
		n -= stride;
	    }
	}
    }
}

static void
horizontalAccumulate16(uint16 *wp, int n, int stride, uint16 *op,
	uint16 *ToLinear16)
{
    register unsigned int  cr, cg, cb, ca, mask;

    if (n >= stride) {
	mask = CODE_MASK;
	if (stride == 3) {
	    op[0] = ToLinear16[cr = wp[0]];
	    op[1] = ToLinear16[cg = wp[1]];
	    op[2] = ToLinear16[cb = wp[2]];
	    n -= 3;
	    while (n > 0) {
		wp += 3;
		op += 3;
		n -= 3;
		op[0] = ToLinear16[(cr += wp[0]) & mask];
		op[1] = ToLinear16[(cg += wp[1]) & mask];
		op[2] = ToLinear16[(cb += wp[2]) & mask];
	    }
	} else if (stride == 4) {
	    op[0] = ToLinear16[cr = wp[0]];
	    op[1] = ToLinear16[cg = wp[1]];
	    op[2] = ToLinear16[cb = wp[2]];
	    op[3] = ToLinear16[ca = wp[3]];
	    n -= 4;
	    while (n > 0) {
		wp += 4;
		op += 4;
		n -= 4;
		op[0] = ToLinear16[(cr += wp[0]) & mask];
		op[1] = ToLinear16[(cg += wp[1]) & mask];
		op[2] = ToLinear16[(cb += wp[2]) & mask];
		op[3] = ToLinear16[(ca += wp[3]) & mask];
	    }
	} else {
	    REPEAT(stride, *op = ToLinear16[*wp&mask]; wp++; op++)
	    n -= stride;
	    while (n > 0) {
		REPEAT(stride,
		    wp[stride] += *wp; *op = ToLinear16[*wp&mask]; wp++; op++)
		n -= stride;
	    }
	}
    }
}

static void
horizontalAccumulate11(uint16 *wp, int n, int stride, uint16 *op)
{
    register unsigned int  cr, cg, cb, ca, mask;

    if (n >= stride) {
	mask = CODE_MASK;
	if (stride == 3) {
	    op[0] = cr = wp[0];  op[1] = cg = wp[1];  op[2] = cb = wp[2];
	    n -= 3;
	    while (n > 0) {
		wp += 3;
		op += 3;
		n -= 3;
		op[0] = (cr += wp[0]) & mask;
		op[1] = (cg += wp[1]) & mask;
		op[2] = (cb += wp[2]) & mask;
	    }
	} else if (stride == 4) {
	    op[0] = cr = wp[0];  op[1] = cg = wp[1];
	    op[2] = cb = wp[2];  op[3] = ca = wp[3];
	    n -= 4;
	    while (n > 0) {
		wp += 4;
		op += 4;
		n -= 4;
		op[0] = (cr += wp[0]) & mask;
		op[1] = (cg += wp[1]) & mask;
		op[2] = (cb += wp[2]) & mask;
		op[3] = (ca += wp[3]) & mask;
	    } 
	} else {
	    REPEAT(stride, *op = *wp&mask; wp++; op++)
	    n -= stride;
	    while (n > 0) {
		REPEAT(stride,
		    wp[stride] += *wp; *op = *wp&mask; wp++; op++)
		n -= stride;
	    }
	}
    }
}

static void
horizontalAccumulate8(uint16 *wp, int n, int stride, unsigned char *op,
	unsigned char *ToLinear8)
{
    register unsigned int  cr, cg, cb, ca, mask;

    if (n >= stride) {
	mask = CODE_MASK;
	if (stride == 3) {
	    op[0] = ToLinear8[cr = wp[0]];
	    op[1] = ToLinear8[cg = wp[1]];
	    op[2] = ToLinear8[cb = wp[2]];
	    n -= 3;
	    while (n > 0) {
		n -= 3;
		wp += 3;
		op += 3;
		op[0] = ToLinear8[(cr += wp[0]) & mask];
		op[1] = ToLinear8[(cg += wp[1]) & mask];
		op[2] = ToLinear8[(cb += wp[2]) & mask];
	    }
	} else if (stride == 4) {
	    op[0] = ToLinear8[cr = wp[0]];
	    op[1] = ToLinear8[cg = wp[1]];
	    op[2] = ToLinear8[cb = wp[2]];
	    op[3] = ToLinear8[ca = wp[3]];
	    n -= 4;
	    while (n > 0) {
		n -= 4;
		wp += 4;
		op += 4;
		op[0] = ToLinear8[(cr += wp[0]) & mask];
		op[1] = ToLinear8[(cg += wp[1]) & mask];
		op[2] = ToLinear8[(cb += wp[2]) & mask];
		op[3] = ToLinear8[(ca += wp[3]) & mask];
	    }
	} else {
	    REPEAT(stride, *op = ToLinear8[*wp&mask]; wp++; op++)
	    n -= stride;
	    while (n > 0) {
		REPEAT(stride,
		    wp[stride] += *wp; *op = ToLinear8[*wp&mask]; wp++; op++)
		n -= stride;
	    }
	}
    }
}

static void
horizontalAccumulate8abgr(uint16 *wp, int n, int stride, unsigned char *op,
	unsigned char *ToLinear8)
{
    register unsigned int  cr, cg, cb, ca, mask;
    register unsigned char  t0, t1, t2, t3;

    if (n >= stride) {
	mask = CODE_MASK;
	if (stride == 3) {
	    op[0] = 0;
	    t1 = ToLinear8[cb = wp[2]];
	    t2 = ToLinear8[cg = wp[1]];
	    t3 = ToLinear8[cr = wp[0]];
	    op[1] = t1;
	    op[2] = t2;
	    op[3] = t3;
	    n -= 3;
	    while (n > 0) {
		n -= 3;
		wp += 3;
		op += 4;
		op[0] = 0;
		t1 = ToLinear8[(cb += wp[2]) & mask];
		t2 = ToLinear8[(cg += wp[1]) & mask];
		t3 = ToLinear8[(cr += wp[0]) & mask];
		op[1] = t1;
		op[2] = t2;
		op[3] = t3;
	    }
	} else if (stride == 4) {
	    t0 = ToLinear8[ca = wp[3]];
	    t1 = ToLinear8[cb = wp[2]];
	    t2 = ToLinear8[cg = wp[1]];
	    t3 = ToLinear8[cr = wp[0]];
	    op[0] = t0;
	    op[1] = t1;
	    op[2] = t2;
	    op[3] = t3;
	    n -= 4;
	    while (n > 0) {
		n -= 4;
		wp += 4;
		op += 4;
		t0 = ToLinear8[(ca += wp[3]) & mask];
		t1 = ToLinear8[(cb += wp[2]) & mask];
		t2 = ToLinear8[(cg += wp[1]) & mask];
		t3 = ToLinear8[(cr += wp[0]) & mask];
		op[0] = t0;
		op[1] = t1;
		op[2] = t2;
		op[3] = t3;
	    }
	} else {
	    REPEAT(stride, *op = ToLinear8[*wp&mask]; wp++; op++)
	    n -= stride;
	    while (n > 0) {
		REPEAT(stride,
		    wp[stride] += *wp; *op = ToLinear8[*wp&mask]; wp++; op++)
		n -= stride;
	    }
	}
    }
}

typedef	struct {
	TIFFPredictorState	predict;
	z_stream		stream;
	uint16			*tbuf; 
	uint16			stride;
	int			state;
	int			user_datafmt;
	int			quality;
#define PLSTATE_INIT 1

	TIFFVSetMethod		vgetparent;	
	TIFFVSetMethod		vsetparent;	

	float *ToLinearF;
	uint16 *ToLinear16;
	unsigned char *ToLinear8;
	uint16  *FromLT2;
	uint16  *From14; 
	uint16  *From8;
	
} PixarLogState;

static int
PixarLogMakeTables(PixarLogState *sp)
{

    int  nlin, lt2size;
    int  i, j;
    double  b, c, linstep, v;
    float *ToLinearF;
    uint16 *ToLinear16;
    unsigned char *ToLinear8;
    uint16  *FromLT2;
    uint16  *From14; 
    uint16  *From8;

    c = log(RATIO);	
    nlin = (int)(1./c);	
    c = 1./nlin;
    b = exp(-c*ONE);	
    linstep = b*c*exp(1.);

    LogK1 = (float)(1./c);	
    LogK2 = (float)(1./b);
    lt2size = (int)(2./linstep) + 1;
    FromLT2 = (uint16 *)_TIFFmalloc(lt2size*sizeof(uint16));
    From14 = (uint16 *)_TIFFmalloc(16384*sizeof(uint16));
    From8 = (uint16 *)_TIFFmalloc(256*sizeof(uint16));
    ToLinearF = (float *)_TIFFmalloc(TSIZEP1 * sizeof(float));
    ToLinear16 = (uint16 *)_TIFFmalloc(TSIZEP1 * sizeof(uint16));
    ToLinear8 = (unsigned char *)_TIFFmalloc(TSIZEP1 * sizeof(unsigned char));
    if (FromLT2 == NULL || From14  == NULL || From8   == NULL ||
	 ToLinearF == NULL || ToLinear16 == NULL || ToLinear8 == NULL) {
	if (FromLT2) _TIFFfree(FromLT2);
	if (From14) _TIFFfree(From14);
	if (From8) _TIFFfree(From8);
	if (ToLinearF) _TIFFfree(ToLinearF);
	if (ToLinear16) _TIFFfree(ToLinear16);
	if (ToLinear8) _TIFFfree(ToLinear8);
	sp->FromLT2 = NULL;
	sp->From14 = NULL;
	sp->From8 = NULL;
	sp->ToLinearF = NULL;
	sp->ToLinear16 = NULL;
	sp->ToLinear8 = NULL;
	return 0;
    }

    j = 0;

    for (i = 0; i < nlin; i++)  {
	v = i * linstep;
	ToLinearF[j++] = (float)v;
    }

    for (i = nlin; i < TSIZE; i++)
	ToLinearF[j++] = (float)(b*exp(c*i));

    ToLinearF[2048] = ToLinearF[2047];

    for (i = 0; i < TSIZEP1; i++)  {
	v = ToLinearF[i]*65535.0 + 0.5;
	ToLinear16[i] = (v > 65535.0) ? 65535 : (uint16)v;
	v = ToLinearF[i]*255.0  + 0.5;
	ToLinear8[i]  = (v > 255.0) ? 255 : (unsigned char)v;
    }

    j = 0;
    for (i = 0; i < lt2size; i++)  {
	if ((i*linstep)*(i*linstep) > ToLinearF[j]*ToLinearF[j+1])
	    j++;
	FromLT2[i] = j;
    }

    
    j = 0;
    for (i = 0; i < 16384; i++)  {
	while ((i/16383.)*(i/16383.) > ToLinearF[j]*ToLinearF[j+1])
	    j++;
	From14[i] = j;
    }

    j = 0;
    for (i = 0; i < 256; i++)  {
	while ((i/255.)*(i/255.) > ToLinearF[j]*ToLinearF[j+1])
	    j++;
	From8[i] = j;
    }

    Fltsize = (float)(lt2size/2);

    sp->ToLinearF = ToLinearF;
    sp->ToLinear16 = ToLinear16;
    sp->ToLinear8 = ToLinear8;
    sp->FromLT2 = FromLT2;
    sp->From14 = From14;
    sp->From8 = From8;

    return 1;
}

#define	DecoderState(tif)	((PixarLogState*) (tif)->tif_data)
#define	EncoderState(tif)	((PixarLogState*) (tif)->tif_data)

static	int PixarLogEncode(TIFF*, tidata_t, tsize_t, tsample_t);
static	int PixarLogDecode(TIFF*, tidata_t, tsize_t, tsample_t);

#define PIXARLOGDATAFMT_UNKNOWN	-1

static int
PixarLogGuessDataFmt(TIFFDirectory *td)
{
	int guess = PIXARLOGDATAFMT_UNKNOWN;
	int format = td->td_sampleformat;

	
	switch (td->td_bitspersample) {
	 case 32:
		if (format == SAMPLEFORMAT_IEEEFP)
			guess = PIXARLOGDATAFMT_FLOAT;
		break;
	 case 16:
		if (format == SAMPLEFORMAT_VOID || format == SAMPLEFORMAT_UINT)
			guess = PIXARLOGDATAFMT_16BIT;
		break;
	 case 12:
		if (format == SAMPLEFORMAT_VOID || format == SAMPLEFORMAT_INT)
			guess = PIXARLOGDATAFMT_12BITPICIO;
		break;
	 case 11:
		if (format == SAMPLEFORMAT_VOID || format == SAMPLEFORMAT_UINT)
			guess = PIXARLOGDATAFMT_11BITLOG;
		break;
	 case 8:
		if (format == SAMPLEFORMAT_VOID || format == SAMPLEFORMAT_UINT)
			guess = PIXARLOGDATAFMT_8BIT;
		break;
	}

	return guess;
}

static uint32
multiply(size_t m1, size_t m2)
{
	uint32	bytes = m1 * m2;

	if (m1 && bytes / m1 != m2)
		bytes = 0;

	return bytes;
}

static int
PixarLogSetupDecode(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;
	PixarLogState* sp = DecoderState(tif);
	tsize_t tbuf_size;
	static const char module[] = "PixarLogSetupDecode";

	assert(sp != NULL);

	
	tif->tif_postdecode = _TIFFNoPostDecode;

	

	sp->stride = (td->td_planarconfig == PLANARCONFIG_CONTIG ?
	    td->td_samplesperpixel : 1);
	tbuf_size = multiply(multiply(multiply(sp->stride, td->td_imagewidth),
				      td->td_rowsperstrip), sizeof(uint16));
	if (tbuf_size == 0)
		return (0);
	sp->tbuf = (uint16 *) _TIFFmalloc(tbuf_size);
	if (sp->tbuf == NULL)
		return (0);
	if (sp->user_datafmt == PIXARLOGDATAFMT_UNKNOWN)
		sp->user_datafmt = PixarLogGuessDataFmt(td);
	if (sp->user_datafmt == PIXARLOGDATAFMT_UNKNOWN) {
		TIFFErrorExt(tif->tif_clientdata, module,
			"PixarLog compression can't handle bits depth/data format combination (depth: %d)", 
			td->td_bitspersample);
		return (0);
	}

	if (inflateInit(&sp->stream) != Z_OK) {
		TIFFErrorExt(tif->tif_clientdata, module, "%s: %s", tif->tif_name, sp->stream.msg);
		return (0);
	} else {
		sp->state |= PLSTATE_INIT;
		return (1);
	}
}

static int
PixarLogPreDecode(TIFF* tif, tsample_t s)
{
	PixarLogState* sp = DecoderState(tif);

	(void) s;
	assert(sp != NULL);
	sp->stream.next_in = tif->tif_rawdata;
	sp->stream.avail_in = tif->tif_rawcc;
	return (inflateReset(&sp->stream) == Z_OK);
}

static int
PixarLogDecode(TIFF* tif, tidata_t op, tsize_t occ, tsample_t s)
{
	TIFFDirectory *td = &tif->tif_dir;
	PixarLogState* sp = DecoderState(tif);
	static const char module[] = "PixarLogDecode";
	int i, nsamples, llen;
	uint16 *up;

	switch (sp->user_datafmt) {
	case PIXARLOGDATAFMT_FLOAT:
		nsamples = occ / sizeof(float);	
		break;
	case PIXARLOGDATAFMT_16BIT:
	case PIXARLOGDATAFMT_12BITPICIO:
	case PIXARLOGDATAFMT_11BITLOG:
		nsamples = occ / sizeof(uint16); 
		break;
	case PIXARLOGDATAFMT_8BIT:
	case PIXARLOGDATAFMT_8BITABGR:
		nsamples = occ;
		break;
	default:
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			"%d bit input not supported in PixarLog",
			td->td_bitspersample);
		return 0;
	}

	llen = sp->stride * td->td_imagewidth;

	(void) s;
	assert(sp != NULL);
	sp->stream.next_out = (unsigned char *) sp->tbuf;
	sp->stream.avail_out = nsamples * sizeof(uint16);
	do {
		int state = inflate(&sp->stream, Z_PARTIAL_FLUSH);
		if (state == Z_STREAM_END) {
			break;			
		}
		if (state == Z_DATA_ERROR) {
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Decoding error at scanline %d, %s",
			    tif->tif_name, tif->tif_row, sp->stream.msg);
			if (inflateSync(&sp->stream) != Z_OK)
				return (0);
			continue;
		}
		if (state != Z_OK) {
			TIFFErrorExt(tif->tif_clientdata, module, "%s: zlib error: %s",
			    tif->tif_name, sp->stream.msg);
			return (0);
		}
	} while (sp->stream.avail_out > 0);

	
	if (sp->stream.avail_out != 0) {
		TIFFErrorExt(tif->tif_clientdata, module,
		    "%s: Not enough data at scanline %d (short %d bytes)",
		    tif->tif_name, tif->tif_row, sp->stream.avail_out);
		return (0);
	}

	up = sp->tbuf;
	
	if (tif->tif_flags & TIFF_SWAB)
		TIFFSwabArrayOfShort(up, nsamples);

	
	if (nsamples % llen) { 
		TIFFWarningExt(tif->tif_clientdata, module,
			"%s: stride %d is not a multiple of sample count, "
			"%d, data truncated.", tif->tif_name, llen, nsamples);
		nsamples -= nsamples % llen;
	}

	for (i = 0; i < nsamples; i += llen, up += llen) {
		switch (sp->user_datafmt)  {
		case PIXARLOGDATAFMT_FLOAT:
			horizontalAccumulateF(up, llen, sp->stride,
					(float *)op, sp->ToLinearF);
			op += llen * sizeof(float);
			break;
		case PIXARLOGDATAFMT_16BIT:
			horizontalAccumulate16(up, llen, sp->stride,
					(uint16 *)op, sp->ToLinear16);
			op += llen * sizeof(uint16);
			break;
		case PIXARLOGDATAFMT_12BITPICIO:
			horizontalAccumulate12(up, llen, sp->stride,
					(int16 *)op, sp->ToLinearF);
			op += llen * sizeof(int16);
			break;
		case PIXARLOGDATAFMT_11BITLOG:
			horizontalAccumulate11(up, llen, sp->stride,
					(uint16 *)op);
			op += llen * sizeof(uint16);
			break;
		case PIXARLOGDATAFMT_8BIT:
			horizontalAccumulate8(up, llen, sp->stride,
					(unsigned char *)op, sp->ToLinear8);
			op += llen * sizeof(unsigned char);
			break;
		case PIXARLOGDATAFMT_8BITABGR:
			horizontalAccumulate8abgr(up, llen, sp->stride,
					(unsigned char *)op, sp->ToLinear8);
			op += llen * sizeof(unsigned char);
			break;
		default:
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
				  "PixarLogDecode: unsupported bits/sample: %d", 
				  td->td_bitspersample);
			return (0);
		}
	}

	return (1);
}

static int
PixarLogSetupEncode(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;
	PixarLogState* sp = EncoderState(tif);
	tsize_t tbuf_size;
	static const char module[] = "PixarLogSetupEncode";

	assert(sp != NULL);

	

	sp->stride = (td->td_planarconfig == PLANARCONFIG_CONTIG ?
	    td->td_samplesperpixel : 1);
	tbuf_size = multiply(multiply(multiply(sp->stride, td->td_imagewidth),
				      td->td_rowsperstrip), sizeof(uint16));
	if (tbuf_size == 0)
		return (0);
	sp->tbuf = (uint16 *) _TIFFmalloc(tbuf_size);
	if (sp->tbuf == NULL)
		return (0);
	if (sp->user_datafmt == PIXARLOGDATAFMT_UNKNOWN)
		sp->user_datafmt = PixarLogGuessDataFmt(td);
	if (sp->user_datafmt == PIXARLOGDATAFMT_UNKNOWN) {
		TIFFErrorExt(tif->tif_clientdata, module, "PixarLog compression can't handle %d bit linear encodings", td->td_bitspersample);
		return (0);
	}

	if (deflateInit(&sp->stream, sp->quality) != Z_OK) {
		TIFFErrorExt(tif->tif_clientdata, module, "%s: %s", tif->tif_name, sp->stream.msg);
		return (0);
	} else {
		sp->state |= PLSTATE_INIT;
		return (1);
	}
}

static int
PixarLogPreEncode(TIFF* tif, tsample_t s)
{
	PixarLogState *sp = EncoderState(tif);

	(void) s;
	assert(sp != NULL);
	sp->stream.next_out = tif->tif_rawdata;
	sp->stream.avail_out = tif->tif_rawdatasize;
	return (deflateReset(&sp->stream) == Z_OK);
}

static void
horizontalDifferenceF(float *ip, int n, int stride, uint16 *wp, uint16 *FromLT2)
{

    int32 r1, g1, b1, a1, r2, g2, b2, a2, mask;
    float fltsize = Fltsize;

#define  CLAMP(v) ( (v<(float)0.)   ? 0				\
		  : (v<(float)2.)   ? FromLT2[(int)(v*fltsize)]	\
		  : (v>(float)24.2) ? 2047			\
		  : LogK1*log(v*LogK2) + 0.5 )

    mask = CODE_MASK;
    if (n >= stride) {
	if (stride == 3) {
	    r2 = wp[0] = (uint16) CLAMP(ip[0]);
	    g2 = wp[1] = (uint16) CLAMP(ip[1]);
	    b2 = wp[2] = (uint16) CLAMP(ip[2]);
	    n -= 3;
	    while (n > 0) {
		n -= 3;
		wp += 3;
		ip += 3;
		r1 = (int32) CLAMP(ip[0]); wp[0] = (r1-r2) & mask; r2 = r1;
		g1 = (int32) CLAMP(ip[1]); wp[1] = (g1-g2) & mask; g2 = g1;
		b1 = (int32) CLAMP(ip[2]); wp[2] = (b1-b2) & mask; b2 = b1;
	    }
	} else if (stride == 4) {
	    r2 = wp[0] = (uint16) CLAMP(ip[0]);
	    g2 = wp[1] = (uint16) CLAMP(ip[1]);
	    b2 = wp[2] = (uint16) CLAMP(ip[2]);
	    a2 = wp[3] = (uint16) CLAMP(ip[3]);
	    n -= 4;
	    while (n > 0) {
		n -= 4;
		wp += 4;
		ip += 4;
		r1 = (int32) CLAMP(ip[0]); wp[0] = (r1-r2) & mask; r2 = r1;
		g1 = (int32) CLAMP(ip[1]); wp[1] = (g1-g2) & mask; g2 = g1;
		b1 = (int32) CLAMP(ip[2]); wp[2] = (b1-b2) & mask; b2 = b1;
		a1 = (int32) CLAMP(ip[3]); wp[3] = (a1-a2) & mask; a2 = a1;
	    }
	} else {
	    ip += n - 1;	
	    wp += n - 1;	
	    n -= stride;
	    while (n > 0) {
		REPEAT(stride, wp[0] = (uint16) CLAMP(ip[0]);
				wp[stride] -= wp[0];
				wp[stride] &= mask;
				wp--; ip--)
		n -= stride;
	    }
	    REPEAT(stride, wp[0] = (uint16) CLAMP(ip[0]); wp--; ip--)
	}
    }
}

static void
horizontalDifference16(unsigned short *ip, int n, int stride, 
	unsigned short *wp, uint16 *From14)
{
    register int  r1, g1, b1, a1, r2, g2, b2, a2, mask;

#undef   CLAMP
#define  CLAMP(v) From14[(v) >> 2]

    mask = CODE_MASK;
    if (n >= stride) {
	if (stride == 3) {
	    r2 = wp[0] = CLAMP(ip[0]);  g2 = wp[1] = CLAMP(ip[1]);
	    b2 = wp[2] = CLAMP(ip[2]);
	    n -= 3;
	    while (n > 0) {
		n -= 3;
		wp += 3;
		ip += 3;
		r1 = CLAMP(ip[0]); wp[0] = (r1-r2) & mask; r2 = r1;
		g1 = CLAMP(ip[1]); wp[1] = (g1-g2) & mask; g2 = g1;
		b1 = CLAMP(ip[2]); wp[2] = (b1-b2) & mask; b2 = b1;
	    }
	} else if (stride == 4) {
	    r2 = wp[0] = CLAMP(ip[0]);  g2 = wp[1] = CLAMP(ip[1]);
	    b2 = wp[2] = CLAMP(ip[2]);  a2 = wp[3] = CLAMP(ip[3]);
	    n -= 4;
	    while (n > 0) {
		n -= 4;
		wp += 4;
		ip += 4;
		r1 = CLAMP(ip[0]); wp[0] = (r1-r2) & mask; r2 = r1;
		g1 = CLAMP(ip[1]); wp[1] = (g1-g2) & mask; g2 = g1;
		b1 = CLAMP(ip[2]); wp[2] = (b1-b2) & mask; b2 = b1;
		a1 = CLAMP(ip[3]); wp[3] = (a1-a2) & mask; a2 = a1;
	    }
	} else {
	    ip += n - 1;	
	    wp += n - 1;	
	    n -= stride;
	    while (n > 0) {
		REPEAT(stride, wp[0] = CLAMP(ip[0]);
				wp[stride] -= wp[0];
				wp[stride] &= mask;
				wp--; ip--)
		n -= stride;
	    }
	    REPEAT(stride, wp[0] = CLAMP(ip[0]); wp--; ip--)
	}
    }
}

static void
horizontalDifference8(unsigned char *ip, int n, int stride, 
	unsigned short *wp, uint16 *From8)
{
    register int  r1, g1, b1, a1, r2, g2, b2, a2, mask;

#undef	 CLAMP
#define  CLAMP(v) (From8[(v)])

    mask = CODE_MASK;
    if (n >= stride) {
	if (stride == 3) {
	    r2 = wp[0] = CLAMP(ip[0]);  g2 = wp[1] = CLAMP(ip[1]);
	    b2 = wp[2] = CLAMP(ip[2]);
	    n -= 3;
	    while (n > 0) {
		n -= 3;
		r1 = CLAMP(ip[3]); wp[3] = (r1-r2) & mask; r2 = r1;
		g1 = CLAMP(ip[4]); wp[4] = (g1-g2) & mask; g2 = g1;
		b1 = CLAMP(ip[5]); wp[5] = (b1-b2) & mask; b2 = b1;
		wp += 3;
		ip += 3;
	    }
	} else if (stride == 4) {
	    r2 = wp[0] = CLAMP(ip[0]);  g2 = wp[1] = CLAMP(ip[1]);
	    b2 = wp[2] = CLAMP(ip[2]);  a2 = wp[3] = CLAMP(ip[3]);
	    n -= 4;
	    while (n > 0) {
		n -= 4;
		r1 = CLAMP(ip[4]); wp[4] = (r1-r2) & mask; r2 = r1;
		g1 = CLAMP(ip[5]); wp[5] = (g1-g2) & mask; g2 = g1;
		b1 = CLAMP(ip[6]); wp[6] = (b1-b2) & mask; b2 = b1;
		a1 = CLAMP(ip[7]); wp[7] = (a1-a2) & mask; a2 = a1;
		wp += 4;
		ip += 4;
	    }
	} else {
	    wp += n + stride - 1;	
	    ip += n + stride - 1;	
	    n -= stride;
	    while (n > 0) {
		REPEAT(stride, wp[0] = CLAMP(ip[0]);
				wp[stride] -= wp[0];
				wp[stride] &= mask;
				wp--; ip--)
		n -= stride;
	    }
	    REPEAT(stride, wp[0] = CLAMP(ip[0]); wp--; ip--)
	}
    }
}

static int
PixarLogEncode(TIFF* tif, tidata_t bp, tsize_t cc, tsample_t s)
{
	TIFFDirectory *td = &tif->tif_dir;
	PixarLogState *sp = EncoderState(tif);
	static const char module[] = "PixarLogEncode";
	int	i, n, llen;
	unsigned short * up;

	(void) s;

	switch (sp->user_datafmt) {
	case PIXARLOGDATAFMT_FLOAT:
		n = cc / sizeof(float);		
		break;
	case PIXARLOGDATAFMT_16BIT:
	case PIXARLOGDATAFMT_12BITPICIO:
	case PIXARLOGDATAFMT_11BITLOG:
		n = cc / sizeof(uint16);	
		break;
	case PIXARLOGDATAFMT_8BIT:
	case PIXARLOGDATAFMT_8BITABGR:
		n = cc;
		break;
	default:
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			"%d bit input not supported in PixarLog",
			td->td_bitspersample);
		return 0;
	}

	llen = sp->stride * td->td_imagewidth;

	for (i = 0, up = sp->tbuf; i < n; i += llen, up += llen) {
		switch (sp->user_datafmt)  {
		case PIXARLOGDATAFMT_FLOAT:
			horizontalDifferenceF((float *)bp, llen, 
				sp->stride, up, sp->FromLT2);
			bp += llen * sizeof(float);
			break;
		case PIXARLOGDATAFMT_16BIT:
			horizontalDifference16((uint16 *)bp, llen, 
				sp->stride, up, sp->From14);
			bp += llen * sizeof(uint16);
			break;
		case PIXARLOGDATAFMT_8BIT:
			horizontalDifference8((unsigned char *)bp, llen, 
				sp->stride, up, sp->From8);
			bp += llen * sizeof(unsigned char);
			break;
		default:
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
				"%d bit input not supported in PixarLog",
				td->td_bitspersample);
			return 0;
		}
	}
 
	sp->stream.next_in = (unsigned char *) sp->tbuf;
	sp->stream.avail_in = n * sizeof(uint16);

	do {
		if (deflate(&sp->stream, Z_NO_FLUSH) != Z_OK) {
			TIFFErrorExt(tif->tif_clientdata, module, "%s: Encoder error: %s",
			    tif->tif_name, sp->stream.msg);
			return (0);
		}
		if (sp->stream.avail_out == 0) {
			tif->tif_rawcc = tif->tif_rawdatasize;
			TIFFFlushData1(tif);
			sp->stream.next_out = tif->tif_rawdata;
			sp->stream.avail_out = tif->tif_rawdatasize;
		}
	} while (sp->stream.avail_in > 0);
	return (1);
}

static int
PixarLogPostEncode(TIFF* tif)
{
	PixarLogState *sp = EncoderState(tif);
	static const char module[] = "PixarLogPostEncode";
	int state;

	sp->stream.avail_in = 0;

	do {
		state = deflate(&sp->stream, Z_FINISH);
		switch (state) {
		case Z_STREAM_END:
		case Z_OK:
		    if (sp->stream.avail_out != (uint32)tif->tif_rawdatasize) {
			    tif->tif_rawcc =
				tif->tif_rawdatasize - sp->stream.avail_out;
			    TIFFFlushData1(tif);
			    sp->stream.next_out = tif->tif_rawdata;
			    sp->stream.avail_out = tif->tif_rawdatasize;
		    }
		    break;
		default:
			TIFFErrorExt(tif->tif_clientdata, module, "%s: zlib error: %s",
			tif->tif_name, sp->stream.msg);
		    return (0);
		}
	} while (state != Z_STREAM_END);
	return (1);
}

static void
PixarLogClose(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;

	
	td->td_bitspersample = 8;
	td->td_sampleformat = SAMPLEFORMAT_UINT;
}

static void
PixarLogCleanup(TIFF* tif)
{
	PixarLogState* sp = (PixarLogState*) tif->tif_data;

	assert(sp != 0);

	(void)TIFFPredictorCleanup(tif);

	tif->tif_tagmethods.vgetfield = sp->vgetparent;
	tif->tif_tagmethods.vsetfield = sp->vsetparent;

	if (sp->FromLT2) _TIFFfree(sp->FromLT2);
	if (sp->From14) _TIFFfree(sp->From14);
	if (sp->From8) _TIFFfree(sp->From8);
	if (sp->ToLinearF) _TIFFfree(sp->ToLinearF);
	if (sp->ToLinear16) _TIFFfree(sp->ToLinear16);
	if (sp->ToLinear8) _TIFFfree(sp->ToLinear8);
	if (sp->state&PLSTATE_INIT) {
		if (tif->tif_mode == O_RDONLY)
			inflateEnd(&sp->stream);
		else
			deflateEnd(&sp->stream);
	}
	if (sp->tbuf)
		_TIFFfree(sp->tbuf);
	_TIFFfree(sp);
	tif->tif_data = NULL;

	_TIFFSetDefaultCompressionState(tif);
}

static int
PixarLogVSetField(TIFF* tif, ttag_t tag, va_list ap)
{
    PixarLogState *sp = (PixarLogState *)tif->tif_data;
    int result;
    static const char module[] = "PixarLogVSetField";

    switch (tag) {
     case TIFFTAG_PIXARLOGQUALITY:
		sp->quality = va_arg(ap, int);
		if (tif->tif_mode != O_RDONLY && (sp->state&PLSTATE_INIT)) {
			if (deflateParams(&sp->stream,
			    sp->quality, Z_DEFAULT_STRATEGY) != Z_OK) {
				TIFFErrorExt(tif->tif_clientdata, module, "%s: zlib error: %s",
					tif->tif_name, sp->stream.msg);
				return (0);
			}
		}
		return (1);
     case TIFFTAG_PIXARLOGDATAFMT:
	sp->user_datafmt = va_arg(ap, int);
	
	switch (sp->user_datafmt) {
	 case PIXARLOGDATAFMT_8BIT:
	 case PIXARLOGDATAFMT_8BITABGR:
	    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
	    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	    break;
	 case PIXARLOGDATAFMT_11BITLOG:
	    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 16);
	    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	    break;
	 case PIXARLOGDATAFMT_12BITPICIO:
	    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 16);
	    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_INT);
	    break;
	 case PIXARLOGDATAFMT_16BIT:
	    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 16);
	    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	    break;
	 case PIXARLOGDATAFMT_FLOAT:
	    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 32);
	    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	    break;
	}
	
	tif->tif_tilesize = isTiled(tif) ? TIFFTileSize(tif) : (tsize_t) -1;
	tif->tif_scanlinesize = TIFFScanlineSize(tif);
	result = 1;		
	break;
     default:
	result = (*sp->vsetparent)(tif, tag, ap);
    }
    return (result);
}

static int
PixarLogVGetField(TIFF* tif, ttag_t tag, va_list ap)
{
    PixarLogState *sp = (PixarLogState *)tif->tif_data;

    switch (tag) {
     case TIFFTAG_PIXARLOGQUALITY:
	*va_arg(ap, int*) = sp->quality;
	break;
     case TIFFTAG_PIXARLOGDATAFMT:
	*va_arg(ap, int*) = sp->user_datafmt;
	break;
     default:
	return (*sp->vgetparent)(tif, tag, ap);
    }
    return (1);
}

static const TIFFFieldInfo pixarlogFieldInfo[] = {
    {TIFFTAG_PIXARLOGDATAFMT,0,0,TIFF_ANY,  FIELD_PSEUDO,FALSE,FALSE,""},
    {TIFFTAG_PIXARLOGQUALITY,0,0,TIFF_ANY,  FIELD_PSEUDO,FALSE,FALSE,""}
};

int
TIFFInitPixarLog(TIFF* tif, int scheme)
{
	static const char module[] = "TIFFInitPixarLog";

	PixarLogState* sp;

	assert(scheme == COMPRESSION_PIXARLOG);

	
	if (!_TIFFMergeFieldInfo(tif, pixarlogFieldInfo,
				 TIFFArrayCount(pixarlogFieldInfo))) {
		TIFFErrorExt(tif->tif_clientdata, module,
			     "Merging PixarLog codec-specific tags failed");
		return 0;
	}

	
	tif->tif_data = (tidata_t) _TIFFmalloc(sizeof (PixarLogState));
	if (tif->tif_data == NULL)
		goto bad;
	sp = (PixarLogState*) tif->tif_data;
	_TIFFmemset(sp, 0, sizeof (*sp));
	sp->stream.data_type = Z_BINARY;
	sp->user_datafmt = PIXARLOGDATAFMT_UNKNOWN;

	
	tif->tif_setupdecode = PixarLogSetupDecode;
	tif->tif_predecode = PixarLogPreDecode;
	tif->tif_decoderow = PixarLogDecode;
	tif->tif_decodestrip = PixarLogDecode;
	tif->tif_decodetile = PixarLogDecode;
	tif->tif_setupencode = PixarLogSetupEncode;
	tif->tif_preencode = PixarLogPreEncode;
	tif->tif_postencode = PixarLogPostEncode;
	tif->tif_encoderow = PixarLogEncode;
	tif->tif_encodestrip = PixarLogEncode;
	tif->tif_encodetile = PixarLogEncode;
	tif->tif_close = PixarLogClose;
	tif->tif_cleanup = PixarLogCleanup;

	
	sp->vgetparent = tif->tif_tagmethods.vgetfield;
	tif->tif_tagmethods.vgetfield = PixarLogVGetField;   
	sp->vsetparent = tif->tif_tagmethods.vsetfield;
	tif->tif_tagmethods.vsetfield = PixarLogVSetField;   

	
	sp->quality = Z_DEFAULT_COMPRESSION; 
	sp->state = 0;

	
	(void) TIFFPredictorInit(tif);

	
	PixarLogMakeTables(sp);

	return (1);
bad:
	TIFFErrorExt(tif->tif_clientdata, module,
		     "No space for PixarLog state block");
	return (0);
}
#endif 

