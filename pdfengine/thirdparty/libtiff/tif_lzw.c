

#include "tiffiop.h"
#ifdef LZW_SUPPORT

#include "tif_predict.h"

#include <stdio.h>

#define	LZW_COMPAT		

#define	LZW_CHECKEOS		

#define MAXCODE(n)	((1L<<(n))-1)

#define	BITS_MIN	9		
#define	BITS_MAX	12		

#define	CODE_CLEAR	256		
#define	CODE_EOI	257		
#define CODE_FIRST	258		
#define	CODE_MAX	MAXCODE(BITS_MAX)
#define	HSIZE		9001L		
#define	HSHIFT		(13-8)
#ifdef LZW_COMPAT

#define	CSIZE		(MAXCODE(BITS_MAX)+1024L)
#else
#define	CSIZE		(MAXCODE(BITS_MAX)+1L)
#endif

typedef	struct {
	TIFFPredictorState predict;	

	unsigned short	nbits;		
	unsigned short	maxcode;	
	unsigned short	free_ent;	
	long		nextdata;	
	long		nextbits;	

        int             rw_mode;        
} LZWBaseState;

#define	lzw_nbits	base.nbits
#define	lzw_maxcode	base.maxcode
#define	lzw_free_ent	base.free_ent
#define	lzw_nextdata	base.nextdata
#define	lzw_nextbits	base.nextbits

typedef uint16 hcode_t;			
typedef struct {
	long	hash;
	hcode_t	code;
} hash_t;

typedef struct code_ent {
	struct code_ent *next;
	unsigned short	length;		
	unsigned char	value;		
	unsigned char	firstchar;	
} code_t;

typedef	int (*decodeFunc)(TIFF*, tidata_t, tsize_t, tsample_t);

typedef struct {
	LZWBaseState base;

	
	long	dec_nbitsmask;		
	long	dec_restart;		
#ifdef LZW_CHECKEOS
	long	dec_bitsleft;		
#endif
	decodeFunc dec_decode;		
	code_t*	dec_codep;		
	code_t*	dec_oldcodep;		
	code_t*	dec_free_entp;		
	code_t*	dec_maxcodep;		
	code_t*	dec_codetab;		

	
	int	enc_oldcode;		
	long	enc_checkpoint;		
#define CHECK_GAP	10000		
	long	enc_ratio;		
	long	enc_incount;		
	long	enc_outcount;		
	tidata_t enc_rawlimit;		
	hash_t*	enc_hashtab;		
} LZWCodecState;

#define	LZWState(tif)		((LZWBaseState*) (tif)->tif_data)
#define	DecoderState(tif)	((LZWCodecState*) LZWState(tif))
#define	EncoderState(tif)	((LZWCodecState*) LZWState(tif))

static	int LZWDecode(TIFF*, tidata_t, tsize_t, tsample_t);
#ifdef LZW_COMPAT
static	int LZWDecodeCompat(TIFF*, tidata_t, tsize_t, tsample_t);
#endif
static  void cl_hash(LZWCodecState*);

#ifdef LZW_CHECKEOS

#define	NextCode(_tif, _sp, _bp, _code, _get) {				\
	if ((_sp)->dec_bitsleft < nbits) {				\
		TIFFWarningExt(_tif->tif_clientdata, _tif->tif_name,				\
		    "LZWDecode: Strip %d not terminated with EOI code", \
		    _tif->tif_curstrip);				\
		_code = CODE_EOI;					\
	} else {							\
		_get(_sp,_bp,_code);					\
		(_sp)->dec_bitsleft -= nbits;				\
	}								\
}
#else
#define	NextCode(tif, sp, bp, code, get) get(sp, bp, code)
#endif

static int
LZWSetupDecode(TIFF* tif)
{
	LZWCodecState* sp = DecoderState(tif);
	static const char module[] = " LZWSetupDecode";
	int code;

        if( sp == NULL )
        {
            
            tif->tif_data = (tidata_t) _TIFFmalloc(sizeof(LZWCodecState));
            if (tif->tif_data == NULL)
            {
				TIFFErrorExt(tif->tif_clientdata, "LZWPreDecode", "No space for LZW state block");
                return (0);
            }

            DecoderState(tif)->dec_codetab = NULL;
            DecoderState(tif)->dec_decode = NULL;
            
            
            (void) TIFFPredictorInit(tif);

            sp = DecoderState(tif);
        }
            
	assert(sp != NULL);

	if (sp->dec_codetab == NULL) {
		sp->dec_codetab = (code_t*)_TIFFmalloc(CSIZE*sizeof (code_t));
		if (sp->dec_codetab == NULL) {
			TIFFErrorExt(tif->tif_clientdata, module,
				     "No space for LZW code table");
			return (0);
		}
		
                code = 255;
                do {
                    sp->dec_codetab[code].value = code;
                    sp->dec_codetab[code].firstchar = code;
                    sp->dec_codetab[code].length = 1;
                    sp->dec_codetab[code].next = NULL;
                } while (code--);
		
                 _TIFFmemset(&sp->dec_codetab[CODE_CLEAR], 0,
			     (CODE_FIRST - CODE_CLEAR) * sizeof (code_t));
	}
	return (1);
}

static int
LZWPreDecode(TIFF* tif, tsample_t s)
{
	LZWCodecState *sp = DecoderState(tif);

	(void) s;
	assert(sp != NULL);
        if( sp->dec_codetab == NULL )
        {
            tif->tif_setupdecode( tif );
        }

	
	if (tif->tif_rawdata[0] == 0 && (tif->tif_rawdata[1] & 0x1)) {
#ifdef LZW_COMPAT
		if (!sp->dec_decode) {
			TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
			    "Old-style LZW codes, convert file");
			
			tif->tif_decoderow = LZWDecodeCompat;
			tif->tif_decodestrip = LZWDecodeCompat;
			tif->tif_decodetile = LZWDecodeCompat;
			
			(*tif->tif_setupdecode)(tif);
			sp->dec_decode = LZWDecodeCompat;
		}
		sp->lzw_maxcode = MAXCODE(BITS_MIN);
#else 
		if (!sp->dec_decode) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			    "Old-style LZW codes not supported");
			sp->dec_decode = LZWDecode;
		}
		return (0);
#endif
	} else {
		sp->lzw_maxcode = MAXCODE(BITS_MIN)-1;
		sp->dec_decode = LZWDecode;
	}
	sp->lzw_nbits = BITS_MIN;
	sp->lzw_nextbits = 0;
	sp->lzw_nextdata = 0;

	sp->dec_restart = 0;
	sp->dec_nbitsmask = MAXCODE(BITS_MIN);
#ifdef LZW_CHECKEOS
	sp->dec_bitsleft = tif->tif_rawcc << 3;
#endif
	sp->dec_free_entp = sp->dec_codetab + CODE_FIRST;
	
	_TIFFmemset(sp->dec_free_entp, 0, (CSIZE-CODE_FIRST)*sizeof (code_t));
	sp->dec_oldcodep = &sp->dec_codetab[-1];
	sp->dec_maxcodep = &sp->dec_codetab[sp->dec_nbitsmask-1];
	return (1);
}

#define	GetNextCode(sp, bp, code) {				\
	nextdata = (nextdata<<8) | *(bp)++;			\
	nextbits += 8;						\
	if (nextbits < nbits) {					\
		nextdata = (nextdata<<8) | *(bp)++;		\
		nextbits += 8;					\
	}							\
	code = (hcode_t)((nextdata >> (nextbits-nbits)) & nbitsmask);	\
	nextbits -= nbits;					\
}

static void
codeLoop(TIFF* tif)
{
	TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
	    "LZWDecode: Bogus encoding, loop in the code table; scanline %d",
	    tif->tif_row);
}

static int
LZWDecode(TIFF* tif, tidata_t op0, tsize_t occ0, tsample_t s)
{
	LZWCodecState *sp = DecoderState(tif);
	char *op = (char*) op0;
	long occ = (long) occ0;
	char *tp;
	unsigned char *bp;
	hcode_t code;
	int len;
	long nbits, nextbits, nextdata, nbitsmask;
	code_t *codep, *free_entp, *maxcodep, *oldcodep;

	(void) s;
	assert(sp != NULL);
        assert(sp->dec_codetab != NULL);
	
	if (sp->dec_restart) {
		long residue;

		codep = sp->dec_codep;
		residue = codep->length - sp->dec_restart;
		if (residue > occ) {
			
			sp->dec_restart += occ;
			do {
				codep = codep->next;
			} while (--residue > occ && codep);
			if (codep) {
				tp = op + occ;
				do {
					*--tp = codep->value;
					codep = codep->next;
				} while (--occ && codep);
			}
			return (1);
		}
		
		op += residue, occ -= residue;
		tp = op;
		do {
			int t;
			--tp;
			t = codep->value;
			codep = codep->next;
			*tp = t;
		} while (--residue && codep);
		sp->dec_restart = 0;
	}

	bp = (unsigned char *)tif->tif_rawcp;
	nbits = sp->lzw_nbits;
	nextdata = sp->lzw_nextdata;
	nextbits = sp->lzw_nextbits;
	nbitsmask = sp->dec_nbitsmask;
	oldcodep = sp->dec_oldcodep;
	free_entp = sp->dec_free_entp;
	maxcodep = sp->dec_maxcodep;

	while (occ > 0) {
		NextCode(tif, sp, bp, code, GetNextCode);
		if (code == CODE_EOI)
			break;
		if (code == CODE_CLEAR) {
			free_entp = sp->dec_codetab + CODE_FIRST;
			_TIFFmemset(free_entp, 0,
				    (CSIZE - CODE_FIRST) * sizeof (code_t));
			nbits = BITS_MIN;
			nbitsmask = MAXCODE(BITS_MIN);
			maxcodep = sp->dec_codetab + nbitsmask-1;
			NextCode(tif, sp, bp, code, GetNextCode);
			if (code == CODE_EOI)
				break;
			if (code == CODE_CLEAR) {
				TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
				"LZWDecode: Corrupted LZW table at scanline %d",
					     tif->tif_row);
				return (0);
			}
			*op++ = (char)code, occ--;
			oldcodep = sp->dec_codetab + code;
			continue;
		}
		codep = sp->dec_codetab + code;

		
		if (free_entp < &sp->dec_codetab[0] ||
			free_entp >= &sp->dec_codetab[CSIZE]) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			"LZWDecode: Corrupted LZW table at scanline %d",
			tif->tif_row);
			return (0);
		}

		free_entp->next = oldcodep;
		if (free_entp->next < &sp->dec_codetab[0] ||
			free_entp->next >= &sp->dec_codetab[CSIZE]) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			"LZWDecode: Corrupted LZW table at scanline %d",
			tif->tif_row);
			return (0);
		}
		free_entp->firstchar = free_entp->next->firstchar;
		free_entp->length = free_entp->next->length+1;
		free_entp->value = (codep < free_entp) ?
		    codep->firstchar : free_entp->firstchar;
		if (++free_entp > maxcodep) {
			if (++nbits > BITS_MAX)		
				nbits = BITS_MAX;
			nbitsmask = MAXCODE(nbits);
			maxcodep = sp->dec_codetab + nbitsmask-1;
		}
		oldcodep = codep;
		if (code >= 256) {
			
			if(codep->length == 0) {
				TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
	    		    "LZWDecode: Wrong length of decoded string: "
			    "data probably corrupted at scanline %d",
			    tif->tif_row);	
			    return (0);
			}
			if (codep->length > occ) {
				
				sp->dec_codep = codep;
				do {
					codep = codep->next;
				} while (codep && codep->length > occ);
				if (codep) {
					sp->dec_restart = occ;
					tp = op + occ;
					do  {
						*--tp = codep->value;
						codep = codep->next;
					}  while (--occ && codep);
					if (codep)
						codeLoop(tif);
				}
				break;
			}
			len = codep->length;
			tp = op + len;
			do {
				int t;
				--tp;
				t = codep->value;
				codep = codep->next;
				*tp = t;
			} while (codep && tp > op);
			if (codep) {
			    codeLoop(tif);
			    break;
			}
			op += len, occ -= len;
		} else
			*op++ = (char)code, occ--;
	}

	tif->tif_rawcp = (tidata_t) bp;
	sp->lzw_nbits = (unsigned short) nbits;
	sp->lzw_nextdata = nextdata;
	sp->lzw_nextbits = nextbits;
	sp->dec_nbitsmask = nbitsmask;
	sp->dec_oldcodep = oldcodep;
	sp->dec_free_entp = free_entp;
	sp->dec_maxcodep = maxcodep;

	if (occ > 0) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		"LZWDecode: Not enough data at scanline %d (short %ld bytes)",
		    tif->tif_row, occ);
		return (0);
	}
	return (1);
}

#ifdef LZW_COMPAT

#define	GetNextCodeCompat(sp, bp, code) {			\
	nextdata |= (unsigned long) *(bp)++ << nextbits;	\
	nextbits += 8;						\
	if (nextbits < nbits) {					\
		nextdata |= (unsigned long) *(bp)++ << nextbits;\
		nextbits += 8;					\
	}							\
	code = (hcode_t)(nextdata & nbitsmask);			\
	nextdata >>= nbits;					\
	nextbits -= nbits;					\
}

static int
LZWDecodeCompat(TIFF* tif, tidata_t op0, tsize_t occ0, tsample_t s)
{
	LZWCodecState *sp = DecoderState(tif);
	char *op = (char*) op0;
	long occ = (long) occ0;
	char *tp;
	unsigned char *bp;
	int code, nbits;
	long nextbits, nextdata, nbitsmask;
	code_t *codep, *free_entp, *maxcodep, *oldcodep;

	(void) s;
	assert(sp != NULL);
	
	if (sp->dec_restart) {
		long residue;

		codep = sp->dec_codep;
		residue = codep->length - sp->dec_restart;
		if (residue > occ) {
			
			sp->dec_restart += occ;
			do {
				codep = codep->next;
			} while (--residue > occ);
			tp = op + occ;
			do {
				*--tp = codep->value;
				codep = codep->next;
			} while (--occ);
			return (1);
		}
		
		op += residue, occ -= residue;
		tp = op;
		do {
			*--tp = codep->value;
			codep = codep->next;
		} while (--residue);
		sp->dec_restart = 0;
	}

	bp = (unsigned char *)tif->tif_rawcp;
	nbits = sp->lzw_nbits;
	nextdata = sp->lzw_nextdata;
	nextbits = sp->lzw_nextbits;
	nbitsmask = sp->dec_nbitsmask;
	oldcodep = sp->dec_oldcodep;
	free_entp = sp->dec_free_entp;
	maxcodep = sp->dec_maxcodep;

	while (occ > 0) {
		NextCode(tif, sp, bp, code, GetNextCodeCompat);
		if (code == CODE_EOI)
			break;
		if (code == CODE_CLEAR) {
			free_entp = sp->dec_codetab + CODE_FIRST;
			_TIFFmemset(free_entp, 0,
				    (CSIZE - CODE_FIRST) * sizeof (code_t));
			nbits = BITS_MIN;
			nbitsmask = MAXCODE(BITS_MIN);
			maxcodep = sp->dec_codetab + nbitsmask;
			NextCode(tif, sp, bp, code, GetNextCodeCompat);
			if (code == CODE_EOI)
				break;
			if (code == CODE_CLEAR) {
				TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
				"LZWDecode: Corrupted LZW table at scanline %d",
					     tif->tif_row);
				return (0);
			}
			*op++ = code, occ--;
			oldcodep = sp->dec_codetab + code;
			continue;
		}
		codep = sp->dec_codetab + code;

		
		if (free_entp < &sp->dec_codetab[0] ||
			free_entp >= &sp->dec_codetab[CSIZE]) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			"LZWDecodeCompat: Corrupted LZW table at scanline %d",
			tif->tif_row);
			return (0);
		}

		free_entp->next = oldcodep;
		if (free_entp->next < &sp->dec_codetab[0] ||
			free_entp->next >= &sp->dec_codetab[CSIZE]) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			"LZWDecodeCompat: Corrupted LZW table at scanline %d",
			tif->tif_row);
			return (0);
		}
		free_entp->firstchar = free_entp->next->firstchar;
		free_entp->length = free_entp->next->length+1;
		free_entp->value = (codep < free_entp) ?
		    codep->firstchar : free_entp->firstchar;
		if (++free_entp > maxcodep) {
			if (++nbits > BITS_MAX)		
				nbits = BITS_MAX;
			nbitsmask = MAXCODE(nbits);
			maxcodep = sp->dec_codetab + nbitsmask;
		}
		oldcodep = codep;
		if (code >= 256) {
			char *op_orig = op;
			
			if(codep->length == 0) {
				TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
	    		    "LZWDecodeCompat: Wrong length of decoded "
			    "string: data probably corrupted at scanline %d",
			    tif->tif_row);	
			    return (0);
			}
			if (codep->length > occ) {
				
				sp->dec_codep = codep;
				do {
					codep = codep->next;
				} while (codep->length > occ);
				sp->dec_restart = occ;
				tp = op + occ;
				do  {
					*--tp = codep->value;
					codep = codep->next;
				}  while (--occ);
				break;
			}
			op += codep->length, occ -= codep->length;
			tp = op;
			do {
				*--tp = codep->value;
			} while( (codep = codep->next) != NULL && tp > op_orig);
		} else
			*op++ = code, occ--;
	}

	tif->tif_rawcp = (tidata_t) bp;
	sp->lzw_nbits = nbits;
	sp->lzw_nextdata = nextdata;
	sp->lzw_nextbits = nextbits;
	sp->dec_nbitsmask = nbitsmask;
	sp->dec_oldcodep = oldcodep;
	sp->dec_free_entp = free_entp;
	sp->dec_maxcodep = maxcodep;

	if (occ > 0) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
	    "LZWDecodeCompat: Not enough data at scanline %d (short %ld bytes)",
		    tif->tif_row, occ);
		return (0);
	}
	return (1);
}
#endif 

static int
LZWSetupEncode(TIFF* tif)
{
	LZWCodecState* sp = EncoderState(tif);
	static const char module[] = "LZWSetupEncode";

	assert(sp != NULL);
	sp->enc_hashtab = (hash_t*) _TIFFmalloc(HSIZE*sizeof (hash_t));
	if (sp->enc_hashtab == NULL) {
		TIFFErrorExt(tif->tif_clientdata, module, "No space for LZW hash table");
		return (0);
	}
	return (1);
}

static int
LZWPreEncode(TIFF* tif, tsample_t s)
{
	LZWCodecState *sp = EncoderState(tif);

	(void) s;
	assert(sp != NULL);
        
        if( sp->enc_hashtab == NULL )
        {
            tif->tif_setupencode( tif );
        }

	sp->lzw_nbits = BITS_MIN;
	sp->lzw_maxcode = MAXCODE(BITS_MIN);
	sp->lzw_free_ent = CODE_FIRST;
	sp->lzw_nextbits = 0;
	sp->lzw_nextdata = 0;
	sp->enc_checkpoint = CHECK_GAP;
	sp->enc_ratio = 0;
	sp->enc_incount = 0;
	sp->enc_outcount = 0;
	
	sp->enc_rawlimit = tif->tif_rawdata + tif->tif_rawdatasize-1 - 4;
	cl_hash(sp);		
	sp->enc_oldcode = (hcode_t) -1;	
	return (1);
}

#define	CALCRATIO(sp, rat) {					\
	if (incount > 0x007fffff) { \
		rat = outcount >> 8;				\
		rat = (rat == 0 ? 0x7fffffff : incount/rat);	\
	} else							\
		rat = (incount<<8) / outcount;			\
}
#define	PutNextCode(op, c) {					\
	nextdata = (nextdata << nbits) | c;			\
	nextbits += nbits;					\
	*op++ = (unsigned char)(nextdata >> (nextbits-8));		\
	nextbits -= 8;						\
	if (nextbits >= 8) {					\
		*op++ = (unsigned char)(nextdata >> (nextbits-8));	\
		nextbits -= 8;					\
	}							\
	outcount += nbits;					\
}

static int
LZWEncode(TIFF* tif, tidata_t bp, tsize_t cc, tsample_t s)
{
	register LZWCodecState *sp = EncoderState(tif);
	register long fcode;
	register hash_t *hp;
	register int h, c;
	hcode_t ent;
	long disp;
	long incount, outcount, checkpoint;
	long nextdata, nextbits;
	int free_ent, maxcode, nbits;
	tidata_t op, limit;

	(void) s;
	if (sp == NULL)
		return (0);

        assert(sp->enc_hashtab != NULL);

	
	incount = sp->enc_incount;
	outcount = sp->enc_outcount;
	checkpoint = sp->enc_checkpoint;
	nextdata = sp->lzw_nextdata;
	nextbits = sp->lzw_nextbits;
	free_ent = sp->lzw_free_ent;
	maxcode = sp->lzw_maxcode;
	nbits = sp->lzw_nbits;
	op = tif->tif_rawcp;
	limit = sp->enc_rawlimit;
	ent = sp->enc_oldcode;

	if (ent == (hcode_t) -1 && cc > 0) {
		
		PutNextCode(op, CODE_CLEAR);
		ent = *bp++; cc--; incount++;
	}
	while (cc > 0) {
		c = *bp++; cc--; incount++;
		fcode = ((long)c << BITS_MAX) + ent;
		h = (c << HSHIFT) ^ ent;	
#ifdef _WINDOWS
		
		if (h >= HSIZE)
			h -= HSIZE;
#endif
		hp = &sp->enc_hashtab[h];
		if (hp->hash == fcode) {
			ent = hp->code;
			continue;
		}
		if (hp->hash >= 0) {
			
			disp = HSIZE - h;
			if (h == 0)
				disp = 1;
			do {
				
				if ((h -= disp) < 0)
					h += HSIZE;
				hp = &sp->enc_hashtab[h];
				if (hp->hash == fcode) {
					ent = hp->code;
					goto hit;
				}
			} while (hp->hash >= 0);
		}
		
		
		if (op > limit) {
			tif->tif_rawcc = (tsize_t)(op - tif->tif_rawdata);
			TIFFFlushData1(tif);
			op = tif->tif_rawdata;
		}
		PutNextCode(op, ent);
		ent = c;
		hp->code = free_ent++;
		hp->hash = fcode;
		if (free_ent == CODE_MAX-1) {
			
			cl_hash(sp);
			sp->enc_ratio = 0;
			incount = 0;
			outcount = 0;
			free_ent = CODE_FIRST;
			PutNextCode(op, CODE_CLEAR);
			nbits = BITS_MIN;
			maxcode = MAXCODE(BITS_MIN);
		} else {
			
			if (free_ent > maxcode) {
				nbits++;
				assert(nbits <= BITS_MAX);
				maxcode = (int) MAXCODE(nbits);
			} else if (incount >= checkpoint) {
				long rat;
				
				checkpoint = incount+CHECK_GAP;
				CALCRATIO(sp, rat);
				if (rat <= sp->enc_ratio) {
					cl_hash(sp);
					sp->enc_ratio = 0;
					incount = 0;
					outcount = 0;
					free_ent = CODE_FIRST;
					PutNextCode(op, CODE_CLEAR);
					nbits = BITS_MIN;
					maxcode = MAXCODE(BITS_MIN);
				} else
					sp->enc_ratio = rat;
			}
		}
	hit:
		;
	}

	
	sp->enc_incount = incount;
	sp->enc_outcount = outcount;
	sp->enc_checkpoint = checkpoint;
	sp->enc_oldcode = ent;
	sp->lzw_nextdata = nextdata;
	sp->lzw_nextbits = nextbits;
	sp->lzw_free_ent = free_ent;
	sp->lzw_maxcode = maxcode;
	sp->lzw_nbits = nbits;
	tif->tif_rawcp = op;
	return (1);
}

static int
LZWPostEncode(TIFF* tif)
{
	register LZWCodecState *sp = EncoderState(tif);
	tidata_t op = tif->tif_rawcp;
	long nextbits = sp->lzw_nextbits;
	long nextdata = sp->lzw_nextdata;
	long outcount = sp->enc_outcount;
	int nbits = sp->lzw_nbits;

	if (op > sp->enc_rawlimit) {
		tif->tif_rawcc = (tsize_t)(op - tif->tif_rawdata);
		TIFFFlushData1(tif);
		op = tif->tif_rawdata;
	}
	if (sp->enc_oldcode != (hcode_t) -1) {
		PutNextCode(op, sp->enc_oldcode);
		sp->enc_oldcode = (hcode_t) -1;
	}
	PutNextCode(op, CODE_EOI);
	if (nextbits > 0) 
		*op++ = (unsigned char)(nextdata << (8-nextbits));
	tif->tif_rawcc = (tsize_t)(op - tif->tif_rawdata);
	return (1);
}

static void
cl_hash(LZWCodecState* sp)
{
	register hash_t *hp = &sp->enc_hashtab[HSIZE-1];
	register long i = HSIZE-8;

 	do {
		i -= 8;
		hp[-7].hash = -1;
		hp[-6].hash = -1;
		hp[-5].hash = -1;
		hp[-4].hash = -1;
		hp[-3].hash = -1;
		hp[-2].hash = -1;
		hp[-1].hash = -1;
		hp[ 0].hash = -1;
		hp -= 8;
	} while (i >= 0);
    	for (i += 8; i > 0; i--, hp--)
		hp->hash = -1;
}

static void
LZWCleanup(TIFF* tif)
{
	(void)TIFFPredictorCleanup(tif);

	assert(tif->tif_data != 0);

	if (DecoderState(tif)->dec_codetab)
		_TIFFfree(DecoderState(tif)->dec_codetab);

	if (EncoderState(tif)->enc_hashtab)
		_TIFFfree(EncoderState(tif)->enc_hashtab);

	_TIFFfree(tif->tif_data);
	tif->tif_data = NULL;

	_TIFFSetDefaultCompressionState(tif);
}

int
TIFFInitLZW(TIFF* tif, int scheme)
{
	assert(scheme == COMPRESSION_LZW);
	
	tif->tif_data = (tidata_t) _TIFFmalloc(sizeof (LZWCodecState));
	if (tif->tif_data == NULL)
		goto bad;
	DecoderState(tif)->dec_codetab = NULL;
	DecoderState(tif)->dec_decode = NULL;
	EncoderState(tif)->enc_hashtab = NULL;
        LZWState(tif)->rw_mode = tif->tif_mode;

	
	tif->tif_setupdecode = LZWSetupDecode;
	tif->tif_predecode = LZWPreDecode;
	tif->tif_decoderow = LZWDecode;
	tif->tif_decodestrip = LZWDecode;
	tif->tif_decodetile = LZWDecode;
	tif->tif_setupencode = LZWSetupEncode;
	tif->tif_preencode = LZWPreEncode;
	tif->tif_postencode = LZWPostEncode;
	tif->tif_encoderow = LZWEncode;
	tif->tif_encodestrip = LZWEncode;
	tif->tif_encodetile = LZWEncode;
	tif->tif_cleanup = LZWCleanup;
	
	(void) TIFFPredictorInit(tif);
	return (1);
bad:
	TIFFErrorExt(tif->tif_clientdata, "TIFFInitLZW", 
		     "No space for LZW state block");
	return (0);
}

#endif 

