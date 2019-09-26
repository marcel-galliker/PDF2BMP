

#include "tiffiop.h"
#ifdef CCITT_SUPPORT

#include "tif_fax3.h"
#define	G3CODES
#include "t4.h"
#include <stdio.h>

typedef struct {
        int     rw_mode;                
	int	mode;			
	uint32	rowbytes;		
	uint32	rowpixels;		

	uint16	cleanfaxdata;		
	uint32	badfaxrun;		
	uint32	badfaxlines;		
	uint32	groupoptions;		
	uint32	recvparams;		
	char*	subaddress;		
	uint32	recvtime;		
	char*	faxdcs;			
	TIFFVGetMethod vgetparent;	
	TIFFVSetMethod vsetparent;	
	TIFFPrintMethod printdir;	
} Fax3BaseState;
#define	Fax3State(tif)		((Fax3BaseState*) (tif)->tif_data)

typedef enum { G3_1D, G3_2D } Ttag;
typedef struct {
	Fax3BaseState b;

	
	const unsigned char* bitmap;	
	uint32	data;			
	int	bit;			
	int	EOLcnt;			
	TIFFFaxFillFunc fill;		
	uint32*	runs;			
	uint32*	refruns;		
	uint32*	curruns;		

	
	Ttag    tag;			
	unsigned char*	refline;	
	int	k;			
	int	maxk;			

	int line;
} Fax3CodecState;
#define	DecoderState(tif)	((Fax3CodecState*) Fax3State(tif))
#define	EncoderState(tif)	((Fax3CodecState*) Fax3State(tif))

#define	is2DEncoding(sp) \
	(sp->b.groupoptions & GROUP3OPT_2DENCODING)
#define	isAligned(p,t)	((((unsigned long)(p)) & (sizeof (t)-1)) == 0)

#define	DECLARE_STATE(tif, sp, mod)					\
    static const char module[] = mod;					\
    Fax3CodecState* sp = DecoderState(tif);				\
    int a0;						\
    int lastx = sp->b.rowpixels;		\
    uint32 BitAcc;					\
    int BitsAvail;				\
    int RunLength;				\
    unsigned char* cp;				\
    unsigned char* ep;					\
    uint32* pa;					\
    uint32* thisrun;				\
    int EOLcnt;					\
    const unsigned char* bitmap = sp->bitmap;		\
    const TIFFFaxTabEnt* TabEnt
#define	DECLARE_STATE_2D(tif, sp, mod)					\
    DECLARE_STATE(tif, sp, mod);					\
    int b1;					\
    uint32* pb				\

#define	CACHE_STATE(tif, sp) do {					\
    BitAcc = sp->data;							\
    BitsAvail = sp->bit;						\
    EOLcnt = sp->EOLcnt;						\
    cp = (unsigned char*) tif->tif_rawcp;				\
    ep = cp + tif->tif_rawcc;						\
} while (0)

#define	UNCACHE_STATE(tif, sp) do {					\
    sp->bit = BitsAvail;						\
    sp->data = BitAcc;							\
    sp->EOLcnt = EOLcnt;						\
    tif->tif_rawcc -= (tidata_t) cp - tif->tif_rawcp;			\
    tif->tif_rawcp = (tidata_t) cp;					\
} while (0)

static int
Fax3PreDecode(TIFF* tif, tsample_t s)
{
	Fax3CodecState* sp = DecoderState(tif);

	(void) s;
	assert(sp != NULL);
	sp->bit = 0;			
	sp->data = 0;
	sp->EOLcnt = 0;			
	
	sp->bitmap =
	    TIFFGetBitRevTable(tif->tif_dir.td_fillorder != FILLORDER_LSB2MSB);
	if (sp->refruns) {		
		sp->refruns[0] = (uint32) sp->b.rowpixels;
		sp->refruns[1] = 0;
	}
	sp->line = 0;
	return (1);
}

static void
Fax3Unexpected(const char* module, TIFF* tif, uint32 line, uint32 a0)
{
	TIFFErrorExt(tif->tif_clientdata, module, "%s: Bad code word at line %u of %s %u (x %u)",
		     tif->tif_name, line, isTiled(tif) ? "tile" : "strip",
		     (isTiled(tif) ? tif->tif_curtile : tif->tif_curstrip),
		     a0);
}
#define	unexpected(table, a0)	Fax3Unexpected(module, tif, sp->line, a0)

static void
Fax3Extension(const char* module, TIFF* tif, uint32 line, uint32 a0)
{
	TIFFErrorExt(tif->tif_clientdata, module,
		     "%s: Uncompressed data (not supported) at line %u of %s %u (x %u)",
		     tif->tif_name, line, isTiled(tif) ? "tile" : "strip",
		     (isTiled(tif) ? tif->tif_curtile : tif->tif_curstrip),
		     a0);
}
#define	extension(a0)	Fax3Extension(module, tif, sp->line, a0)

static void
Fax3BadLength(const char* module, TIFF* tif, uint32 line, uint32 a0, uint32 lastx)
{
	TIFFWarningExt(tif->tif_clientdata, module, "%s: %s at line %u of %s %u (got %u, expected %u)",
		       tif->tif_name,
		       a0 < lastx ? "Premature EOL" : "Line length mismatch",
		       line, isTiled(tif) ? "tile" : "strip",
		       (isTiled(tif) ? tif->tif_curtile : tif->tif_curstrip),
		       a0, lastx);
}
#define	badlength(a0,lastx)	Fax3BadLength(module, tif, sp->line, a0, lastx)

static void
Fax3PrematureEOF(const char* module, TIFF* tif, uint32 line, uint32 a0)
{
	TIFFWarningExt(tif->tif_clientdata, module, "%s: Premature EOF at line %u of %s %u (x %u)",
	    tif->tif_name,
		       line, isTiled(tif) ? "tile" : "strip",
		       (isTiled(tif) ? tif->tif_curtile : tif->tif_curstrip),
		       a0);
}
#define	prematureEOF(a0)	Fax3PrematureEOF(module, tif, sp->line, a0)

#define	Nop

static int
Fax3Decode1D(TIFF* tif, tidata_t buf, tsize_t occ, tsample_t s)
{
	DECLARE_STATE(tif, sp, "Fax3Decode1D");

	(void) s;
	CACHE_STATE(tif, sp);
	thisrun = sp->curruns;
	while ((long)occ > 0) {
		a0 = 0;
		RunLength = 0;
		pa = thisrun;
#ifdef FAX3_DEBUG
		printf("\nBitAcc=%08X, BitsAvail = %d\n", BitAcc, BitsAvail);
		printf("-------------------- %d\n", tif->tif_row);
		fflush(stdout);
#endif
		SYNC_EOL(EOF1D);
		EXPAND1D(EOF1Da);
		(*sp->fill)(buf, thisrun, pa, lastx);
		buf += sp->b.rowbytes;
		occ -= sp->b.rowbytes;
		sp->line++;
		continue;
	EOF1D:				
		CLEANUP_RUNS();
	EOF1Da:				
		(*sp->fill)(buf, thisrun, pa, lastx);
		UNCACHE_STATE(tif, sp);
		return (-1);
	}
	UNCACHE_STATE(tif, sp);
	return (1);
}

#define	SWAP(t,a,b)	{ t x; x = (a); (a) = (b); (b) = x; }

static int
Fax3Decode2D(TIFF* tif, tidata_t buf, tsize_t occ, tsample_t s)
{
	DECLARE_STATE_2D(tif, sp, "Fax3Decode2D");
	int is1D;			

	(void) s;
	CACHE_STATE(tif, sp);
	while ((long)occ > 0) {
		a0 = 0;
		RunLength = 0;
		pa = thisrun = sp->curruns;
#ifdef FAX3_DEBUG
		printf("\nBitAcc=%08X, BitsAvail = %d EOLcnt = %d",
		    BitAcc, BitsAvail, EOLcnt);
#endif
		SYNC_EOL(EOF2D);
		NeedBits8(1, EOF2D);
		is1D = GetBits(1);	
		ClrBits(1);
#ifdef FAX3_DEBUG
		printf(" %s\n-------------------- %d\n",
		    is1D ? "1D" : "2D", tif->tif_row);
		fflush(stdout);
#endif
		pb = sp->refruns;
		b1 = *pb++;
		if (is1D)
			EXPAND1D(EOF2Da);
		else
			EXPAND2D(EOF2Da);
		(*sp->fill)(buf, thisrun, pa, lastx);
		SETVALUE(0);		
		SWAP(uint32*, sp->curruns, sp->refruns);
		buf += sp->b.rowbytes;
		occ -= sp->b.rowbytes;
		sp->line++;
		continue;
	EOF2D:				
		CLEANUP_RUNS();
	EOF2Da:				
		(*sp->fill)(buf, thisrun, pa, lastx);
		UNCACHE_STATE(tif, sp);
		return (-1);
	}
	UNCACHE_STATE(tif, sp);
	return (1);
}
#undef SWAP

#if SIZEOF_LONG == 8
# define FILL(n, cp)							    \
    switch (n) {							    \
    case 15:(cp)[14] = 0xff; case 14:(cp)[13] = 0xff; case 13: (cp)[12] = 0xff;\
    case 12:(cp)[11] = 0xff; case 11:(cp)[10] = 0xff; case 10: (cp)[9] = 0xff;\
    case  9: (cp)[8] = 0xff; case  8: (cp)[7] = 0xff; case  7: (cp)[6] = 0xff;\
    case  6: (cp)[5] = 0xff; case  5: (cp)[4] = 0xff; case  4: (cp)[3] = 0xff;\
    case  3: (cp)[2] = 0xff; case  2: (cp)[1] = 0xff;			      \
    case  1: (cp)[0] = 0xff; (cp) += (n); case 0:  ;			      \
    }
# define ZERO(n, cp)							\
    switch (n) {							\
    case 15:(cp)[14] = 0; case 14:(cp)[13] = 0; case 13: (cp)[12] = 0;	\
    case 12:(cp)[11] = 0; case 11:(cp)[10] = 0; case 10: (cp)[9] = 0;	\
    case  9: (cp)[8] = 0; case  8: (cp)[7] = 0; case  7: (cp)[6] = 0;	\
    case  6: (cp)[5] = 0; case  5: (cp)[4] = 0; case  4: (cp)[3] = 0;	\
    case  3: (cp)[2] = 0; case  2: (cp)[1] = 0;				\
    case  1: (cp)[0] = 0; (cp) += (n); case 0:  ;			\
    }
#else
# define FILL(n, cp)							    \
    switch (n) {							    \
    case 7: (cp)[6] = 0xff; case 6: (cp)[5] = 0xff; case 5: (cp)[4] = 0xff; \
    case 4: (cp)[3] = 0xff; case 3: (cp)[2] = 0xff; case 2: (cp)[1] = 0xff; \
    case 1: (cp)[0] = 0xff; (cp) += (n); case 0:  ;			    \
    }
# define ZERO(n, cp)							\
    switch (n) {							\
    case 7: (cp)[6] = 0; case 6: (cp)[5] = 0; case 5: (cp)[4] = 0;	\
    case 4: (cp)[3] = 0; case 3: (cp)[2] = 0; case 2: (cp)[1] = 0;	\
    case 1: (cp)[0] = 0; (cp) += (n); case 0:  ;			\
    }
#endif

void
_TIFFFax3fillruns(unsigned char* buf, uint32* runs, uint32* erun, uint32 lastx)
{
	static const unsigned char _fillmasks[] =
	    { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
	unsigned char* cp;
	uint32 x, bx, run;
	int32 n, nw;
	long* lp;

	if ((erun-runs)&1)
	    *erun++ = 0;
	x = 0;
	for (; runs < erun; runs += 2) {
	    run = runs[0];
	    if (x+run > lastx || run > lastx )
		run = runs[0] = (uint32) (lastx - x);
	    if (run) {
		cp = buf + (x>>3);
		bx = x&7;
		if (run > 8-bx) {
		    if (bx) {			
			*cp++ &= 0xff << (8-bx);
			run -= 8-bx;
		    }
		    if( (n = run >> 3) != 0 ) {	
			if ((n/sizeof (long)) > 1) {
			    
			    for (; n && !isAligned(cp, long); n--)
				    *cp++ = 0x00;
			    lp = (long*) cp;
			    nw = (int32)(n / sizeof (long));
			    n -= nw * sizeof (long);
			    do {
				    *lp++ = 0L;
			    } while (--nw);
			    cp = (unsigned char*) lp;
			}
			ZERO(n, cp);
			run &= 7;
		    }
		    if (run)
			cp[0] &= 0xff >> run;
		} else
		    cp[0] &= ~(_fillmasks[run]>>bx);
		x += runs[0];
	    }
	    run = runs[1];
	    if (x+run > lastx || run > lastx )
		run = runs[1] = lastx - x;
	    if (run) {
		cp = buf + (x>>3);
		bx = x&7;
		if (run > 8-bx) {
		    if (bx) {			
			*cp++ |= 0xff >> bx;
			run -= 8-bx;
		    }
		    if( (n = run>>3) != 0 ) {	
			if ((n/sizeof (long)) > 1) {
			    
			    for (; n && !isAligned(cp, long); n--)
				*cp++ = 0xff;
			    lp = (long*) cp;
			    nw = (int32)(n / sizeof (long));
			    n -= nw * sizeof (long);
			    do {
				*lp++ = -1L;
			    } while (--nw);
			    cp = (unsigned char*) lp;
			}
			FILL(n, cp);
			run &= 7;
		    }
		    if (run)
			cp[0] |= 0xff00 >> run;
		} else
		    cp[0] |= _fillmasks[run]>>bx;
		x += runs[1];
	    }
	}
	assert(x == lastx);
}
#undef	ZERO
#undef	FILL

static int
Fax3SetupState(TIFF* tif)
{
	TIFFDirectory* td = &tif->tif_dir;
	Fax3BaseState* sp = Fax3State(tif);
	int needsRefLine;
	Fax3CodecState* dsp = (Fax3CodecState*) Fax3State(tif);
	uint32 rowbytes, rowpixels, nruns;

	if (td->td_bitspersample != 1) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		    "Bits/sample must be 1 for Group 3/4 encoding/decoding");
		return (0);
	}
	
	if (isTiled(tif)) {
		rowbytes = TIFFTileRowSize(tif);
		rowpixels = td->td_tilewidth;
	} else {
		rowbytes = TIFFScanlineSize(tif);
		rowpixels = td->td_imagewidth;
	}
	sp->rowbytes = (uint32) rowbytes;
	sp->rowpixels = (uint32) rowpixels;
	
	needsRefLine = (
	    (sp->groupoptions & GROUP3OPT_2DENCODING) ||
	    td->td_compression == COMPRESSION_CCITTFAX4
	);

	
	dsp->runs=(uint32*) NULL;
	nruns = TIFFroundup(rowpixels,32);
	if (needsRefLine) {
		nruns = TIFFSafeMultiply(uint32,nruns,2);
	}
	if ((nruns == 0) || (TIFFSafeMultiply(uint32,nruns,2) == 0)) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			     "Row pixels integer overflow (rowpixels %u)",
			     rowpixels);
		return (0);
	}
	dsp->runs = (uint32*) _TIFFCheckMalloc(tif,
					       TIFFSafeMultiply(uint32,nruns,2),
					       sizeof (uint32),
					       "for Group 3/4 run arrays");
	if (dsp->runs == NULL)
		return (0);
	dsp->curruns = dsp->runs;
	if (needsRefLine)
		dsp->refruns = dsp->runs + nruns;
	else
		dsp->refruns = NULL;
	if (td->td_compression == COMPRESSION_CCITTFAX3
	    && is2DEncoding(dsp)) {	
		tif->tif_decoderow = Fax3Decode2D;
		tif->tif_decodestrip = Fax3Decode2D;
		tif->tif_decodetile = Fax3Decode2D;
	}

	if (needsRefLine) {		
		Fax3CodecState* esp = EncoderState(tif);
		
		esp->refline = (unsigned char*) _TIFFmalloc(rowbytes);
		if (esp->refline == NULL) {
			TIFFErrorExt(tif->tif_clientdata, "Fax3SetupState",
			    "%s: No space for Group 3/4 reference line",
			    tif->tif_name);
			return (0);
		}
	} else					
		EncoderState(tif)->refline = NULL;

	return (1);
}

#define	Fax3FlushBits(tif, sp) {				\
	if ((tif)->tif_rawcc >= (tif)->tif_rawdatasize)		\
		(void) TIFFFlushData1(tif);			\
	*(tif)->tif_rawcp++ = (tidataval_t) (sp)->data;		\
	(tif)->tif_rawcc++;					\
	(sp)->data = 0, (sp)->bit = 8;				\
}
#define	_FlushBits(tif) {					\
	if ((tif)->tif_rawcc >= (tif)->tif_rawdatasize)		\
		(void) TIFFFlushData1(tif);			\
	*(tif)->tif_rawcp++ = (tidataval_t) data;		\
	(tif)->tif_rawcc++;					\
	data = 0, bit = 8;					\
}
static const int _msbmask[9] =
    { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
#define	_PutBits(tif, bits, length) {				\
	while (length > bit) {					\
		data |= bits >> (length - bit);			\
		length -= bit;					\
		_FlushBits(tif);				\
	}							\
	data |= (bits & _msbmask[length]) << (bit - length);	\
	bit -= length;						\
	if (bit == 0)						\
		_FlushBits(tif);				\
}
	

static void
Fax3PutBits(TIFF* tif, unsigned int bits, unsigned int length)
{
	Fax3CodecState* sp = EncoderState(tif);
	unsigned int bit = sp->bit;
	int data = sp->data;

	_PutBits(tif, bits, length);

	sp->data = data;
	sp->bit = bit;
}

#define putcode(tif, te)	Fax3PutBits(tif, (te)->code, (te)->length)

#ifdef FAX3_DEBUG
#define	DEBUG_COLOR(w) (tab == TIFFFaxWhiteCodes ? w "W" : w "B")
#define	DEBUG_PRINT(what,len) {						\
    int t;								\
    printf("%08X/%-2d: %s%5d\t", data, bit, DEBUG_COLOR(what), len);	\
    for (t = length-1; t >= 0; t--)					\
	putchar(code & (1<<t) ? '1' : '0');				\
    putchar('\n');							\
}
#endif

static void
putspan(TIFF* tif, int32 span, const tableentry* tab)
{
	Fax3CodecState* sp = EncoderState(tif);
	unsigned int bit = sp->bit;
	int data = sp->data;
	unsigned int code, length;

	while (span >= 2624) {
		const tableentry* te = &tab[63 + (2560>>6)];
		code = te->code, length = te->length;
#ifdef FAX3_DEBUG
		DEBUG_PRINT("MakeUp", te->runlen);
#endif
		_PutBits(tif, code, length);
		span -= te->runlen;
	}
	if (span >= 64) {
		const tableentry* te = &tab[63 + (span>>6)];
		assert(te->runlen == 64*(span>>6));
		code = te->code, length = te->length;
#ifdef FAX3_DEBUG
		DEBUG_PRINT("MakeUp", te->runlen);
#endif
		_PutBits(tif, code, length);
		span -= te->runlen;
	}
	code = tab[span].code, length = tab[span].length;
#ifdef FAX3_DEBUG
	DEBUG_PRINT("  Term", tab[span].runlen);
#endif
	_PutBits(tif, code, length);

	sp->data = data;
	sp->bit = bit;
}

static void
Fax3PutEOL(TIFF* tif)
{
	Fax3CodecState* sp = EncoderState(tif);
	unsigned int bit = sp->bit;
	int data = sp->data;
	unsigned int code, length, tparm;

	if (sp->b.groupoptions & GROUP3OPT_FILLBITS) {
		
		int align = 8 - 4;
		if (align != sp->bit) {
			if (align > sp->bit)
				align = sp->bit + (8 - align);
			else
				align = sp->bit - align;
			code = 0;
			tparm=align; 
			_PutBits(tif, 0, tparm);
		}
	}
	code = EOL, length = 12;
	if (is2DEncoding(sp))
		code = (code<<1) | (sp->tag == G3_1D), length++;
	_PutBits(tif, code, length);

	sp->data = data;
	sp->bit = bit;
}

static int
Fax3PreEncode(TIFF* tif, tsample_t s)
{
	Fax3CodecState* sp = EncoderState(tif);

	(void) s;
	assert(sp != NULL);
	sp->bit = 8;
	sp->data = 0;
	sp->tag = G3_1D;
	
	if (sp->refline)
		_TIFFmemset(sp->refline, 0x00, sp->b.rowbytes);
	if (is2DEncoding(sp)) {
		float res = tif->tif_dir.td_yresolution;
		
		if (tif->tif_dir.td_resolutionunit == RESUNIT_CENTIMETER)
			res *= 2.54f;		
		sp->maxk = (res > 150 ? 4 : 2);
		sp->k = sp->maxk-1;
	} else
		sp->k = sp->maxk = 0;
	sp->line = 0;
	return (1);
}

static const unsigned char zeroruns[256] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,	
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,	
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
};
static const unsigned char oneruns[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,	
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 8,	
};

#ifdef VAXC
static	int32 find0span(unsigned char*, int32, int32);
static	int32 find1span(unsigned char*, int32, int32);
#pragma inline(find0span,find1span)
#endif

static int32
find0span(unsigned char* bp, int32 bs, int32 be)
{
	int32 bits = be - bs;
	int32 n, span;

	bp += bs>>3;
	
	if (bits > 0 && (n = (bs & 7))) {
		span = zeroruns[(*bp << n) & 0xff];
		if (span > 8-n)		
			span = 8-n;
		if (span > bits)	
			span = bits;
		if (n+span < 8)		
			return (span);
		bits -= span;
		bp++;
	} else
		span = 0;
	if (bits >= (int32)(2 * 8 * sizeof(long))) {
		long* lp;
		
		while (!isAligned(bp, long)) {
			if (*bp != 0x00)
				return (span + zeroruns[*bp]);
			span += 8, bits -= 8;
			bp++;
		}
		lp = (long*) bp;
		while ((bits >= (int32)(8 * sizeof(long))) && (0 == *lp)) {
			span += 8*sizeof (long), bits -= 8*sizeof (long);
			lp++;
		}
		bp = (unsigned char*) lp;
	}
	
	while (bits >= 8) {
		if (*bp != 0x00)	
			return (span + zeroruns[*bp]);
		span += 8, bits -= 8;
		bp++;
	}
	
	if (bits > 0) {
		n = zeroruns[*bp];
		span += (n > bits ? bits : n);
	}
	return (span);
}

static int32
find1span(unsigned char* bp, int32 bs, int32 be)
{
	int32 bits = be - bs;
	int32 n, span;

	bp += bs>>3;
	
	if (bits > 0 && (n = (bs & 7))) {
		span = oneruns[(*bp << n) & 0xff];
		if (span > 8-n)		
			span = 8-n;
		if (span > bits)	
			span = bits;
		if (n+span < 8)		
			return (span);
		bits -= span;
		bp++;
	} else
		span = 0;
	if (bits >= (int32)(2 * 8 * sizeof(long))) {
		long* lp;
		
		while (!isAligned(bp, long)) {
			if (*bp != 0xff)
				return (span + oneruns[*bp]);
			span += 8, bits -= 8;
			bp++;
		}
		lp = (long*) bp;
		while ((bits >= (int32)(8 * sizeof(long))) && (~0 == *lp)) {
			span += 8*sizeof (long), bits -= 8*sizeof (long);
			lp++;
		}
		bp = (unsigned char*) lp;
	}
	
	while (bits >= 8) {
		if (*bp != 0xff)	
			return (span + oneruns[*bp]);
		span += 8, bits -= 8;
		bp++;
	}
	
	if (bits > 0) {
		n = oneruns[*bp];
		span += (n > bits ? bits : n);
	}
	return (span);
}

#define	finddiff(_cp, _bs, _be, _color)	\
	(_bs + (_color ? find1span(_cp,_bs,_be) : find0span(_cp,_bs,_be)))

#define	finddiff2(_cp, _bs, _be, _color) \
	(_bs < _be ? finddiff(_cp,_bs,_be,_color) : _be)

static int
Fax3Encode1DRow(TIFF* tif, unsigned char* bp, uint32 bits)
{
	Fax3CodecState* sp = EncoderState(tif);
	int32 span;
        uint32 bs = 0;

	for (;;) {
		span = find0span(bp, bs, bits);		
		putspan(tif, span, TIFFFaxWhiteCodes);
		bs += span;
		if (bs >= bits)
			break;
		span = find1span(bp, bs, bits);		
		putspan(tif, span, TIFFFaxBlackCodes);
		bs += span;
		if (bs >= bits)
			break;
	}
	if (sp->b.mode & (FAXMODE_BYTEALIGN|FAXMODE_WORDALIGN)) {
		if (sp->bit != 8)			
			Fax3FlushBits(tif, sp);
		if ((sp->b.mode&FAXMODE_WORDALIGN) &&
		    !isAligned(tif->tif_rawcp, uint16))
			Fax3FlushBits(tif, sp);
	}
	return (1);
}

static const tableentry horizcode =
    { 3, 0x1, 0 };	
static const tableentry passcode =
    { 4, 0x1, 0 };	
static const tableentry vcodes[7] = {
    { 7, 0x03, 0 },	
    { 6, 0x03, 0 },	
    { 3, 0x03, 0 },	
    { 1, 0x1, 0 },	
    { 3, 0x2, 0 },	
    { 6, 0x02, 0 },	
    { 7, 0x02, 0 }	
};

static int
Fax3Encode2DRow(TIFF* tif, unsigned char* bp, unsigned char* rp, uint32 bits)
{
#define	PIXEL(buf,ix)	((((buf)[(ix)>>3]) >> (7-((ix)&7))) & 1)
        uint32 a0 = 0;
	uint32 a1 = (PIXEL(bp, 0) != 0 ? 0 : finddiff(bp, 0, bits, 0));
	uint32 b1 = (PIXEL(rp, 0) != 0 ? 0 : finddiff(rp, 0, bits, 0));
	uint32 a2, b2;

	for (;;) {
		b2 = finddiff2(rp, b1, bits, PIXEL(rp,b1));
		if (b2 >= a1) {
			int32 d = b1 - a1;
			if (!(-3 <= d && d <= 3)) {	
				a2 = finddiff2(bp, a1, bits, PIXEL(bp,a1));
				putcode(tif, &horizcode);
				if (a0+a1 == 0 || PIXEL(bp, a0) == 0) {
					putspan(tif, a1-a0, TIFFFaxWhiteCodes);
					putspan(tif, a2-a1, TIFFFaxBlackCodes);
				} else {
					putspan(tif, a1-a0, TIFFFaxBlackCodes);
					putspan(tif, a2-a1, TIFFFaxWhiteCodes);
				}
				a0 = a2;
			} else {			
				putcode(tif, &vcodes[d+3]);
				a0 = a1;
			}
		} else {				
			putcode(tif, &passcode);
			a0 = b2;
		}
		if (a0 >= bits)
			break;
		a1 = finddiff(bp, a0, bits, PIXEL(bp,a0));
		b1 = finddiff(rp, a0, bits, !PIXEL(bp,a0));
		b1 = finddiff(rp, b1, bits, PIXEL(bp,a0));
	}
	return (1);
#undef PIXEL
}

static int
Fax3Encode(TIFF* tif, tidata_t bp, tsize_t cc, tsample_t s)
{
	Fax3CodecState* sp = EncoderState(tif);

	(void) s;
	while ((long)cc > 0) {
		if ((sp->b.mode & FAXMODE_NOEOL) == 0)
			Fax3PutEOL(tif);
		if (is2DEncoding(sp)) {
			if (sp->tag == G3_1D) {
				if (!Fax3Encode1DRow(tif, bp, sp->b.rowpixels))
					return (0);
				sp->tag = G3_2D;
			} else {
				if (!Fax3Encode2DRow(tif, bp, sp->refline,
                                                     sp->b.rowpixels))
					return (0);
				sp->k--;
			}
			if (sp->k == 0) {
				sp->tag = G3_1D;
				sp->k = sp->maxk-1;
			} else
				_TIFFmemcpy(sp->refline, bp, sp->b.rowbytes);
		} else {
			if (!Fax3Encode1DRow(tif, bp, sp->b.rowpixels))
				return (0);
		}
		bp += sp->b.rowbytes;
		cc -= sp->b.rowbytes;
	}
	return (1);
}

static int
Fax3PostEncode(TIFF* tif)
{
	Fax3CodecState* sp = EncoderState(tif);

	if (sp->bit != 8)
		Fax3FlushBits(tif, sp);
	return (1);
}

static void
Fax3Close(TIFF* tif)
{
	if ((Fax3State(tif)->mode & FAXMODE_NORTC) == 0) {
		Fax3CodecState* sp = EncoderState(tif);
		unsigned int code = EOL;
		unsigned int length = 12;
		int i;

		if (is2DEncoding(sp))
			code = (code<<1) | (sp->tag == G3_1D), length++;
		for (i = 0; i < 6; i++)
			Fax3PutBits(tif, code, length);
		Fax3FlushBits(tif, sp);
	}
}

static void
Fax3Cleanup(TIFF* tif)
{
	Fax3CodecState* sp = DecoderState(tif);
	
	assert(sp != 0);

	tif->tif_tagmethods.vgetfield = sp->b.vgetparent;
	tif->tif_tagmethods.vsetfield = sp->b.vsetparent;
	tif->tif_tagmethods.printdir = sp->b.printdir;

	if (sp->runs)
		_TIFFfree(sp->runs);
	if (sp->refline)
		_TIFFfree(sp->refline);

	if (Fax3State(tif)->subaddress)
		_TIFFfree(Fax3State(tif)->subaddress);
	if (Fax3State(tif)->faxdcs)
		_TIFFfree(Fax3State(tif)->faxdcs);

	_TIFFfree(tif->tif_data);
	tif->tif_data = NULL;

	_TIFFSetDefaultCompressionState(tif);
}

#define	FIELD_BADFAXLINES	(FIELD_CODEC+0)
#define	FIELD_CLEANFAXDATA	(FIELD_CODEC+1)
#define	FIELD_BADFAXRUN		(FIELD_CODEC+2)
#define	FIELD_RECVPARAMS	(FIELD_CODEC+3)
#define	FIELD_SUBADDRESS	(FIELD_CODEC+4)
#define	FIELD_RECVTIME		(FIELD_CODEC+5)
#define	FIELD_FAXDCS		(FIELD_CODEC+6)

#define	FIELD_OPTIONS		(FIELD_CODEC+7)

static const TIFFFieldInfo faxFieldInfo[] = {
    { TIFFTAG_FAXMODE,		 0, 0,	TIFF_ANY,	FIELD_PSEUDO,
      FALSE,	FALSE,	"FaxMode" },
    { TIFFTAG_FAXFILLFUNC,	 0, 0,	TIFF_ANY,	FIELD_PSEUDO,
      FALSE,	FALSE,	"FaxFillFunc" },
    { TIFFTAG_BADFAXLINES,	 1, 1,	TIFF_LONG,	FIELD_BADFAXLINES,
      TRUE,	FALSE,	"BadFaxLines" },
    { TIFFTAG_BADFAXLINES,	 1, 1,	TIFF_SHORT,	FIELD_BADFAXLINES,
      TRUE,	FALSE,	"BadFaxLines" },
    { TIFFTAG_CLEANFAXDATA,	 1, 1,	TIFF_SHORT,	FIELD_CLEANFAXDATA,
      TRUE,	FALSE,	"CleanFaxData" },
    { TIFFTAG_CONSECUTIVEBADFAXLINES,1,1, TIFF_LONG,	FIELD_BADFAXRUN,
      TRUE,	FALSE,	"ConsecutiveBadFaxLines" },
    { TIFFTAG_CONSECUTIVEBADFAXLINES,1,1, TIFF_SHORT,	FIELD_BADFAXRUN,
      TRUE,	FALSE,	"ConsecutiveBadFaxLines" },
    { TIFFTAG_FAXRECVPARAMS,	 1, 1, TIFF_LONG,	FIELD_RECVPARAMS,
      TRUE,	FALSE,	"FaxRecvParams" },
    { TIFFTAG_FAXSUBADDRESS,	-1,-1, TIFF_ASCII,	FIELD_SUBADDRESS,
      TRUE,	FALSE,	"FaxSubAddress" },
    { TIFFTAG_FAXRECVTIME,	 1, 1, TIFF_LONG,	FIELD_RECVTIME,
      TRUE,	FALSE,	"FaxRecvTime" },
    { TIFFTAG_FAXDCS,		-1,-1, TIFF_ASCII,	FIELD_FAXDCS,
      TRUE,	FALSE,	"FaxDcs" },
};
static const TIFFFieldInfo fax3FieldInfo[] = {
    { TIFFTAG_GROUP3OPTIONS,	 1, 1,	TIFF_LONG,	FIELD_OPTIONS,
      FALSE,	FALSE,	"Group3Options" },
};
static const TIFFFieldInfo fax4FieldInfo[] = {
    { TIFFTAG_GROUP4OPTIONS,	 1, 1,	TIFF_LONG,	FIELD_OPTIONS,
      FALSE,	FALSE,	"Group4Options" },
};
#define	N(a)	(sizeof (a) / sizeof (a[0]))

static int
Fax3VSetField(TIFF* tif, ttag_t tag, va_list ap)
{
	Fax3BaseState* sp = Fax3State(tif);
	const TIFFFieldInfo* fip;

	assert(sp != 0);
	assert(sp->vsetparent != 0);

	switch (tag) {
	case TIFFTAG_FAXMODE:
		sp->mode = va_arg(ap, int);
		return 1;			
	case TIFFTAG_FAXFILLFUNC:
		DecoderState(tif)->fill = va_arg(ap, TIFFFaxFillFunc);
		return 1;			
	case TIFFTAG_GROUP3OPTIONS:
		
		if (tif->tif_dir.td_compression == COMPRESSION_CCITTFAX3)
			sp->groupoptions = va_arg(ap, uint32);
		break;
	case TIFFTAG_GROUP4OPTIONS:
		
		if (tif->tif_dir.td_compression == COMPRESSION_CCITTFAX4)
			sp->groupoptions = va_arg(ap, uint32);
		break;
	case TIFFTAG_BADFAXLINES:
		sp->badfaxlines = va_arg(ap, uint32);
		break;
	case TIFFTAG_CLEANFAXDATA:
		sp->cleanfaxdata = (uint16) va_arg(ap, int);
		break;
	case TIFFTAG_CONSECUTIVEBADFAXLINES:
		sp->badfaxrun = va_arg(ap, uint32);
		break;
	case TIFFTAG_FAXRECVPARAMS:
		sp->recvparams = va_arg(ap, uint32);
		break;
	case TIFFTAG_FAXSUBADDRESS:
		_TIFFsetString(&sp->subaddress, va_arg(ap, char*));
		break;
	case TIFFTAG_FAXRECVTIME:
		sp->recvtime = va_arg(ap, uint32);
		break;
	case TIFFTAG_FAXDCS:
		_TIFFsetString(&sp->faxdcs, va_arg(ap, char*));
		break;
	default:
		return (*sp->vsetparent)(tif, tag, ap);
	}
	
	if ((fip = _TIFFFieldWithTag(tif, tag)))
		TIFFSetFieldBit(tif, fip->field_bit);
	else
		return 0;

	tif->tif_flags |= TIFF_DIRTYDIRECT;
	return 1;
}

static int
Fax3VGetField(TIFF* tif, ttag_t tag, va_list ap)
{
	Fax3BaseState* sp = Fax3State(tif);

	assert(sp != 0);

	switch (tag) {
	case TIFFTAG_FAXMODE:
		*va_arg(ap, int*) = sp->mode;
		break;
	case TIFFTAG_FAXFILLFUNC:
		*va_arg(ap, TIFFFaxFillFunc*) = DecoderState(tif)->fill;
		break;
	case TIFFTAG_GROUP3OPTIONS:
	case TIFFTAG_GROUP4OPTIONS:
		*va_arg(ap, uint32*) = sp->groupoptions;
		break;
	case TIFFTAG_BADFAXLINES:
		*va_arg(ap, uint32*) = sp->badfaxlines;
		break;
	case TIFFTAG_CLEANFAXDATA:
		*va_arg(ap, uint16*) = sp->cleanfaxdata;
		break;
	case TIFFTAG_CONSECUTIVEBADFAXLINES:
		*va_arg(ap, uint32*) = sp->badfaxrun;
		break;
	case TIFFTAG_FAXRECVPARAMS:
		*va_arg(ap, uint32*) = sp->recvparams;
		break;
	case TIFFTAG_FAXSUBADDRESS:
		*va_arg(ap, char**) = sp->subaddress;
		break;
	case TIFFTAG_FAXRECVTIME:
		*va_arg(ap, uint32*) = sp->recvtime;
		break;
	case TIFFTAG_FAXDCS:
		*va_arg(ap, char**) = sp->faxdcs;
		break;
	default:
		return (*sp->vgetparent)(tif, tag, ap);
	}
	return (1);
}

static void
Fax3PrintDir(TIFF* tif, FILE* fd, long flags)
{
	Fax3BaseState* sp = Fax3State(tif);

	assert(sp != 0);

	(void) flags;
	if (TIFFFieldSet(tif,FIELD_OPTIONS)) {
		const char* sep = " ";
		if (tif->tif_dir.td_compression == COMPRESSION_CCITTFAX4) {
			fprintf(fd, "  Group 4 Options:");
			if (sp->groupoptions & GROUP4OPT_UNCOMPRESSED)
				fprintf(fd, "%suncompressed data", sep);
		} else {

			fprintf(fd, "  Group 3 Options:");
			if (sp->groupoptions & GROUP3OPT_2DENCODING)
				fprintf(fd, "%s2-d encoding", sep), sep = "+";
			if (sp->groupoptions & GROUP3OPT_FILLBITS)
				fprintf(fd, "%sEOL padding", sep), sep = "+";
			if (sp->groupoptions & GROUP3OPT_UNCOMPRESSED)
				fprintf(fd, "%suncompressed data", sep);
		}
		fprintf(fd, " (%lu = 0x%lx)\n",
                        (unsigned long) sp->groupoptions,
                        (unsigned long) sp->groupoptions);
	}
	if (TIFFFieldSet(tif,FIELD_CLEANFAXDATA)) {
		fprintf(fd, "  Fax Data:");
		switch (sp->cleanfaxdata) {
		case CLEANFAXDATA_CLEAN:
			fprintf(fd, " clean");
			break;
		case CLEANFAXDATA_REGENERATED:
			fprintf(fd, " receiver regenerated");
			break;
		case CLEANFAXDATA_UNCLEAN:
			fprintf(fd, " uncorrected errors");
			break;
		}
		fprintf(fd, " (%u = 0x%x)\n",
		    sp->cleanfaxdata, sp->cleanfaxdata);
	}
	if (TIFFFieldSet(tif,FIELD_BADFAXLINES))
		fprintf(fd, "  Bad Fax Lines: %lu\n",
                        (unsigned long) sp->badfaxlines);
	if (TIFFFieldSet(tif,FIELD_BADFAXRUN))
		fprintf(fd, "  Consecutive Bad Fax Lines: %lu\n",
		    (unsigned long) sp->badfaxrun);
	if (TIFFFieldSet(tif,FIELD_RECVPARAMS))
		fprintf(fd, "  Fax Receive Parameters: %08lx\n",
		   (unsigned long) sp->recvparams);
	if (TIFFFieldSet(tif,FIELD_SUBADDRESS))
		fprintf(fd, "  Fax SubAddress: %s\n", sp->subaddress);
	if (TIFFFieldSet(tif,FIELD_RECVTIME))
		fprintf(fd, "  Fax Receive Time: %lu secs\n",
		    (unsigned long) sp->recvtime);
	if (TIFFFieldSet(tif,FIELD_FAXDCS))
		fprintf(fd, "  Fax DCS: %s\n", sp->faxdcs);
}

static int
InitCCITTFax3(TIFF* tif)
{
	Fax3BaseState* sp;

	
	if (!_TIFFMergeFieldInfo(tif, faxFieldInfo, N(faxFieldInfo))) {
		TIFFErrorExt(tif->tif_clientdata, "InitCCITTFax3",
			"Merging common CCITT Fax codec-specific tags failed");
		return 0;
	}

	
	tif->tif_data = (tidata_t)
		_TIFFmalloc(sizeof (Fax3CodecState));

	if (tif->tif_data == NULL) {
		TIFFErrorExt(tif->tif_clientdata, "TIFFInitCCITTFax3",
		    "%s: No space for state block", tif->tif_name);
		return (0);
	}

	sp = Fax3State(tif);
        sp->rw_mode = tif->tif_mode;

	
	sp->vgetparent = tif->tif_tagmethods.vgetfield;
	tif->tif_tagmethods.vgetfield = Fax3VGetField; 
	sp->vsetparent = tif->tif_tagmethods.vsetfield;
	tif->tif_tagmethods.vsetfield = Fax3VSetField; 
	sp->printdir = tif->tif_tagmethods.printdir;
	tif->tif_tagmethods.printdir = Fax3PrintDir;   
	sp->groupoptions = 0;	
	sp->recvparams = 0;
	sp->subaddress = NULL;
	sp->faxdcs = NULL;

	if (sp->rw_mode == O_RDONLY) 
		tif->tif_flags |= TIFF_NOBITREV; 
	DecoderState(tif)->runs = NULL;
	TIFFSetField(tif, TIFFTAG_FAXFILLFUNC, _TIFFFax3fillruns);
	EncoderState(tif)->refline = NULL;

	
	tif->tif_setupdecode = Fax3SetupState;
	tif->tif_predecode = Fax3PreDecode;
	tif->tif_decoderow = Fax3Decode1D;
	tif->tif_decodestrip = Fax3Decode1D;
	tif->tif_decodetile = Fax3Decode1D;
	tif->tif_setupencode = Fax3SetupState;
	tif->tif_preencode = Fax3PreEncode;
	tif->tif_postencode = Fax3PostEncode;
	tif->tif_encoderow = Fax3Encode;
	tif->tif_encodestrip = Fax3Encode;
	tif->tif_encodetile = Fax3Encode;
	tif->tif_close = Fax3Close;
	tif->tif_cleanup = Fax3Cleanup;

	return (1);
}

int
TIFFInitCCITTFax3(TIFF* tif, int scheme)
{
	(void) scheme;
	if (InitCCITTFax3(tif)) {
		
		if (!_TIFFMergeFieldInfo(tif, fax3FieldInfo, N(fax3FieldInfo))) {
			TIFFErrorExt(tif->tif_clientdata, "TIFFInitCCITTFax3",
			"Merging CCITT Fax 3 codec-specific tags failed");
			return 0;
		}

		
		return TIFFSetField(tif, TIFFTAG_FAXMODE, FAXMODE_CLASSF);
	} else
		return 01;
}

#define	SWAP(t,a,b)	{ t x; x = (a); (a) = (b); (b) = x; }

static int
Fax4Decode(TIFF* tif, tidata_t buf, tsize_t occ, tsample_t s)
{
	DECLARE_STATE_2D(tif, sp, "Fax4Decode");

	(void) s;
	CACHE_STATE(tif, sp);
	while ((long)occ > 0) {
		a0 = 0;
		RunLength = 0;
		pa = thisrun = sp->curruns;
		pb = sp->refruns;
		b1 = *pb++;
#ifdef FAX3_DEBUG
		printf("\nBitAcc=%08X, BitsAvail = %d\n", BitAcc, BitsAvail);
		printf("-------------------- %d\n", tif->tif_row);
		fflush(stdout);
#endif
		EXPAND2D(EOFG4);
                if (EOLcnt)
                    goto EOFG4;
		(*sp->fill)(buf, thisrun, pa, lastx);
		SETVALUE(0);		
		SWAP(uint32*, sp->curruns, sp->refruns);
		buf += sp->b.rowbytes;
		occ -= sp->b.rowbytes;
		sp->line++;
		continue;
	EOFG4:
                NeedBits16( 13, BADG4 );
        BADG4:
#ifdef FAX3_DEBUG
                if( GetBits(13) != 0x1001 )
                    fputs( "Bad EOFB\n", stderr );
#endif                
                ClrBits( 13 );
		(*sp->fill)(buf, thisrun, pa, lastx);
		UNCACHE_STATE(tif, sp);
		return ( sp->line ? 1 : -1);	
	}
	UNCACHE_STATE(tif, sp);
	return (1);
}
#undef	SWAP

static int
Fax4Encode(TIFF* tif, tidata_t bp, tsize_t cc, tsample_t s)
{
	Fax3CodecState *sp = EncoderState(tif);

	(void) s;
	while ((long)cc > 0) {
		if (!Fax3Encode2DRow(tif, bp, sp->refline, sp->b.rowpixels))
			return (0);
		_TIFFmemcpy(sp->refline, bp, sp->b.rowbytes);
		bp += sp->b.rowbytes;
		cc -= sp->b.rowbytes;
	}
	return (1);
}

static int
Fax4PostEncode(TIFF* tif)
{
	Fax3CodecState *sp = EncoderState(tif);

	
	Fax3PutBits(tif, EOL, 12);
	Fax3PutBits(tif, EOL, 12);
	if (sp->bit != 8)
		Fax3FlushBits(tif, sp);
	return (1);
}

int
TIFFInitCCITTFax4(TIFF* tif, int scheme)
{
	(void) scheme;
	if (InitCCITTFax3(tif)) {		
		
		if (!_TIFFMergeFieldInfo(tif, fax4FieldInfo, N(fax4FieldInfo))) {
			TIFFErrorExt(tif->tif_clientdata, "TIFFInitCCITTFax4",
			"Merging CCITT Fax 4 codec-specific tags failed");
			return 0;
		}

		tif->tif_decoderow = Fax4Decode;
		tif->tif_decodestrip = Fax4Decode;
		tif->tif_decodetile = Fax4Decode;
		tif->tif_encoderow = Fax4Encode;
		tif->tif_encodestrip = Fax4Encode;
		tif->tif_encodetile = Fax4Encode;
		tif->tif_postencode = Fax4PostEncode;
		
		return TIFFSetField(tif, TIFFTAG_FAXMODE, FAXMODE_NORTC);
	} else
		return (0);
}

static int
Fax3DecodeRLE(TIFF* tif, tidata_t buf, tsize_t occ, tsample_t s)
{
	DECLARE_STATE(tif, sp, "Fax3DecodeRLE");
	int mode = sp->b.mode;

	(void) s;
	CACHE_STATE(tif, sp);
	thisrun = sp->curruns;
	while ((long)occ > 0) {
		a0 = 0;
		RunLength = 0;
		pa = thisrun;
#ifdef FAX3_DEBUG
		printf("\nBitAcc=%08X, BitsAvail = %d\n", BitAcc, BitsAvail);
		printf("-------------------- %d\n", tif->tif_row);
		fflush(stdout);
#endif
		EXPAND1D(EOFRLE);
		(*sp->fill)(buf, thisrun, pa, lastx);
		
		if (mode & FAXMODE_BYTEALIGN) {
			int n = BitsAvail - (BitsAvail &~ 7);
			ClrBits(n);
		} else if (mode & FAXMODE_WORDALIGN) {
			int n = BitsAvail - (BitsAvail &~ 15);
			ClrBits(n);
			if (BitsAvail == 0 && !isAligned(cp, uint16))
			    cp++;
		}
		buf += sp->b.rowbytes;
		occ -= sp->b.rowbytes;
		sp->line++;
		continue;
	EOFRLE:				
		(*sp->fill)(buf, thisrun, pa, lastx);
		UNCACHE_STATE(tif, sp);
		return (-1);
	}
	UNCACHE_STATE(tif, sp);
	return (1);
}

int
TIFFInitCCITTRLE(TIFF* tif, int scheme)
{
	(void) scheme;
	if (InitCCITTFax3(tif)) {		
		tif->tif_decoderow = Fax3DecodeRLE;
		tif->tif_decodestrip = Fax3DecodeRLE;
		tif->tif_decodetile = Fax3DecodeRLE;
		
		return TIFFSetField(tif, TIFFTAG_FAXMODE,
		    FAXMODE_NORTC|FAXMODE_NOEOL|FAXMODE_BYTEALIGN);
	} else
		return (0);
}

int
TIFFInitCCITTRLEW(TIFF* tif, int scheme)
{
	(void) scheme;
	if (InitCCITTFax3(tif)) {		
		tif->tif_decoderow = Fax3DecodeRLE;
		tif->tif_decodestrip = Fax3DecodeRLE;
		tif->tif_decodetile = Fax3DecodeRLE;
		
		return TIFFSetField(tif, TIFFTAG_FAXMODE,
		    FAXMODE_NORTC|FAXMODE_NOEOL|FAXMODE_WORDALIGN);
	} else
		return (0);
}
#endif 

