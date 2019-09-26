

#include "tiffiop.h"
#ifdef THUNDER_SUPPORT

#define	THUNDER_DATA		0x3f	
#define	THUNDER_CODE		0xc0	

#define	THUNDER_RUN		0x00	
#define	THUNDER_2BITDELTAS	0x40	
#define	    DELTA2_SKIP		2	
#define	THUNDER_3BITDELTAS	0x80	
#define	    DELTA3_SKIP		4	
#define	THUNDER_RAW		0xc0	

static const int twobitdeltas[4] = { 0, 1, 0, -1 };
static const int threebitdeltas[8] = { 0, 1, 2, 3, 0, -3, -2, -1 };

#define	SETPIXEL(op, v) { \
	lastpixel = (v) & 0xf; \
	if (npixels++ & 1) \
	    *op++ |= lastpixel; \
	else \
	    op[0] = (tidataval_t) (lastpixel << 4); \
}

static int
ThunderDecode(TIFF* tif, tidata_t op, tsize_t maxpixels)
{
	register unsigned char *bp;
	register tsize_t cc;
	unsigned int lastpixel;
	tsize_t npixels;

	bp = (unsigned char *)tif->tif_rawcp;
	cc = tif->tif_rawcc;
	lastpixel = 0;
	npixels = 0;
	while (cc > 0 && npixels < maxpixels) {
		int n, delta;

		n = *bp++, cc--;
		switch (n & THUNDER_CODE) {
		case THUNDER_RUN:		
			
			if (npixels & 1) {
				op[0] |= lastpixel;
				lastpixel = *op++; npixels++; n--;
			} else
				lastpixel |= lastpixel << 4;
			npixels += n;
			if (npixels < maxpixels) {
				for (; n > 0; n -= 2)
					*op++ = (tidataval_t) lastpixel;
			}
			if (n == -1)
				*--op &= 0xf0;
			lastpixel &= 0xf;
			break;
		case THUNDER_2BITDELTAS:	
			if ((delta = ((n >> 4) & 3)) != DELTA2_SKIP)
				SETPIXEL(op, lastpixel + twobitdeltas[delta]);
			if ((delta = ((n >> 2) & 3)) != DELTA2_SKIP)
				SETPIXEL(op, lastpixel + twobitdeltas[delta]);
			if ((delta = (n & 3)) != DELTA2_SKIP)
				SETPIXEL(op, lastpixel + twobitdeltas[delta]);
			break;
		case THUNDER_3BITDELTAS:	
			if ((delta = ((n >> 3) & 7)) != DELTA3_SKIP)
				SETPIXEL(op, lastpixel + threebitdeltas[delta]);
			if ((delta = (n & 7)) != DELTA3_SKIP)
				SETPIXEL(op, lastpixel + threebitdeltas[delta]);
			break;
		case THUNDER_RAW:		
			SETPIXEL(op, n);
			break;
		}
	}
	tif->tif_rawcp = (tidata_t) bp;
	tif->tif_rawcc = cc;
	if (npixels != maxpixels) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		    "ThunderDecode: %s data at scanline %ld (%lu != %lu)",
		    npixels < maxpixels ? "Not enough" : "Too much",
		    (long) tif->tif_row, (long) npixels, (long) maxpixels);
		return (0);
	}
	return (1);
}

static int
ThunderDecodeRow(TIFF* tif, tidata_t buf, tsize_t occ, tsample_t s)
{
	tidata_t row = buf;
	
	(void) s;
	while ((long)occ > 0) {
		if (!ThunderDecode(tif, row, tif->tif_dir.td_imagewidth))
			return (0);
		occ -= tif->tif_scanlinesize;
		row += tif->tif_scanlinesize;
	}
	return (1);
}

int
TIFFInitThunderScan(TIFF* tif, int scheme)
{
	(void) scheme;
	tif->tif_decoderow = ThunderDecodeRow;
	tif->tif_decodestrip = ThunderDecodeRow;
	return (1);
}
#endif 

