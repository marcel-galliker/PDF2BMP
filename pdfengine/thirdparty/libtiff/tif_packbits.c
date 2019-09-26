

#include "tiffiop.h"
#ifdef PACKBITS_SUPPORT

#include <stdio.h>

static int
PackBitsPreEncode(TIFF* tif, tsample_t s)
{
	(void) s;

        if (!(tif->tif_data = (tidata_t)_TIFFmalloc(sizeof(tsize_t))))
		return (0);
	
	if (isTiled(tif))
		*(tsize_t*)tif->tif_data = TIFFTileRowSize(tif);
	else
		*(tsize_t*)tif->tif_data = TIFFScanlineSize(tif);
	return (1);
}

static int
PackBitsPostEncode(TIFF* tif)
{
        if (tif->tif_data)
            _TIFFfree(tif->tif_data);
	return (1);
}

typedef unsigned char tidata;

static int
PackBitsEncode(TIFF* tif, tidata_t buf, tsize_t cc, tsample_t s)
{
	unsigned char* bp = (unsigned char*) buf;
	tidata_t op, ep, lastliteral;
	long n, slop;
	int b;
	enum { BASE, LITERAL, RUN, LITERAL_RUN } state;

	(void) s;
	op = tif->tif_rawcp;
	ep = tif->tif_rawdata + tif->tif_rawdatasize;
	state = BASE;
	lastliteral = 0;
	while (cc > 0) {
		
		b = *bp++, cc--, n = 1;
		for (; cc > 0 && b == *bp; cc--, bp++)
			n++;
	again:
		if (op + 2 >= ep) {		
			
			if (state == LITERAL || state == LITERAL_RUN) {
				slop = op - lastliteral;
				tif->tif_rawcc += lastliteral - tif->tif_rawcp;
				if (!TIFFFlushData1(tif))
					return (-1);
				op = tif->tif_rawcp;
				while (slop-- > 0)
					*op++ = *lastliteral++;
				lastliteral = tif->tif_rawcp;
			} else {
				tif->tif_rawcc += op - tif->tif_rawcp;
				if (!TIFFFlushData1(tif))
					return (-1);
				op = tif->tif_rawcp;
			}
		}
		switch (state) {
		case BASE:		
			if (n > 1) {
				state = RUN;
				if (n > 128) {
					*op++ = (tidata) -127;
					*op++ = (tidataval_t) b;
					n -= 128;
					goto again;
				}
				*op++ = (tidataval_t)(-(n-1));
				*op++ = (tidataval_t) b;
			} else {
				lastliteral = op;
				*op++ = 0;
				*op++ = (tidataval_t) b;
				state = LITERAL;
			}
			break;
		case LITERAL:		
			if (n > 1) {
				state = LITERAL_RUN;
				if (n > 128) {
					*op++ = (tidata) -127;
					*op++ = (tidataval_t) b;
					n -= 128;
					goto again;
				}
				*op++ = (tidataval_t)(-(n-1));	
				*op++ = (tidataval_t) b;
			} else {			
				if (++(*lastliteral) == 127)
					state = BASE;
				*op++ = (tidataval_t) b;
			}
			break;
		case RUN:		
			if (n > 1) {
				if (n > 128) {
					*op++ = (tidata) -127;
					*op++ = (tidataval_t) b;
					n -= 128;
					goto again;
				}
				*op++ = (tidataval_t)(-(n-1));
				*op++ = (tidataval_t) b;
			} else {
				lastliteral = op;
				*op++ = 0;
				*op++ = (tidataval_t) b;
				state = LITERAL;
			}
			break;
		case LITERAL_RUN:	
			
			if (n == 1 && op[-2] == (tidata) -1 &&
			    *lastliteral < 126) {
				state = (((*lastliteral) += 2) == 127 ?
				    BASE : LITERAL);
				op[-2] = op[-1];	
			} else
				state = RUN;
			goto again;
		}
	}
	tif->tif_rawcc += op - tif->tif_rawcp;
	tif->tif_rawcp = op;
	return (1);
}

static int
PackBitsEncodeChunk(TIFF* tif, tidata_t bp, tsize_t cc, tsample_t s)
{
	tsize_t rowsize = *(tsize_t*)tif->tif_data;

	while ((long)cc > 0) {
		int	chunk = rowsize;
		
		if( cc < chunk )
		    chunk = cc;

		if (PackBitsEncode(tif, bp, chunk, s) < 0)
		    return (-1);
		bp += chunk;
		cc -= chunk;
	}
	return (1);
}

static int
PackBitsDecode(TIFF* tif, tidata_t op, tsize_t occ, tsample_t s)
{
	char *bp;
	tsize_t cc;
	long n;
	int b;

	(void) s;
	bp = (char*) tif->tif_rawcp;
	cc = tif->tif_rawcc;
	while (cc > 0 && (long)occ > 0) {
		n = (long) *bp++, cc--;
		
		if (n >= 128)
			n -= 256;
		if (n < 0) {		
			if (n == -128)	
				continue;
                        n = -n + 1;
                        if( occ < n )
                        {
							TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
                                        "PackBitsDecode: discarding %ld bytes "
                                        "to avoid buffer overrun",
                                        n - occ);
                            n = occ;
                        }
			occ -= n;
			b = *bp++, cc--;
			while (n-- > 0)
				*op++ = (tidataval_t) b;
		} else {		
			if (occ < n + 1)
                        {
                            TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
                                        "PackBitsDecode: discarding %ld bytes "
                                        "to avoid buffer overrun",
                                        n - occ + 1);
                            n = occ - 1;
                        }
                        _TIFFmemcpy(op, bp, ++n);
			op += n; occ -= n;
			bp += n; cc -= n;
		}
	}
	tif->tif_rawcp = (tidata_t) bp;
	tif->tif_rawcc = cc;
	if (occ > 0) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		    "PackBitsDecode: Not enough data for scanline %ld",
		    (long) tif->tif_row);
		return (0);
	}
	return (1);
}

int
TIFFInitPackBits(TIFF* tif, int scheme)
{
	(void) scheme;
	tif->tif_decoderow = PackBitsDecode;
	tif->tif_decodestrip = PackBitsDecode;
	tif->tif_decodetile = PackBitsDecode;
	tif->tif_preencode = PackBitsPreEncode;
        tif->tif_postencode = PackBitsPostEncode;
	tif->tif_encoderow = PackBitsEncode;
	tif->tif_encodestrip = PackBitsEncodeChunk;
	tif->tif_encodetile = PackBitsEncodeChunk;
	return (1);
}
#endif 

