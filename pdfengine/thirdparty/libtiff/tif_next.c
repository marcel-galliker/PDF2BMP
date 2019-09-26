

#include "tiffiop.h"
#ifdef NEXT_SUPPORT

#define SETPIXEL(op, v) {			\
	switch (npixels++ & 3) {		\
	case 0:	op[0]  = (unsigned char) ((v) << 6); break;	\
	case 1:	op[0] |= (v) << 4; break;	\
	case 2:	op[0] |= (v) << 2; break;	\
	case 3:	*op++ |= (v);	   break;	\
	}					\
}

#define LITERALROW	0x00
#define LITERALSPAN	0x40
#define WHITE   	((1<<2)-1)

static int
NeXTDecode(TIFF* tif, tidata_t buf, tsize_t occ, tsample_t s)
{
	unsigned char *bp, *op;
	tsize_t cc;
	tidata_t row;
	tsize_t scanline, n;

	(void) s;
	
	for (op = buf, cc = occ; cc-- > 0;)
		*op++ = 0xff;

	bp = (unsigned char *)tif->tif_rawcp;
	cc = tif->tif_rawcc;
	scanline = tif->tif_scanlinesize;
	for (row = buf; occ > 0; occ -= scanline, row += scanline) {
		n = *bp++, cc--;
		switch (n) {
		case LITERALROW:
			
			if (cc < scanline)
				goto bad;
			_TIFFmemcpy(row, bp, scanline);
			bp += scanline;
			cc -= scanline;
			break;
		case LITERALSPAN: {
			tsize_t off;
			
			off = (bp[0] * 256) + bp[1];
			n = (bp[2] * 256) + bp[3];
			if (cc < 4+n || off+n > scanline)
				goto bad;
			_TIFFmemcpy(row+off, bp+4, n);
			bp += 4+n;
			cc -= 4+n;
			break;
		}
		default: {
			uint32 npixels = 0, grey;
			uint32 imagewidth = tif->tif_dir.td_imagewidth;

			
			op = row;
			for (;;) {
				grey = (n>>6) & 0x3;
				n &= 0x3f;
				
				while (n-- > 0 && npixels < imagewidth)
					SETPIXEL(op, grey);
				if (npixels >= imagewidth)
					break;
				if (cc == 0)
					goto bad;
				n = *bp++, cc--;
			}
			break;
		}
		}
	}
	tif->tif_rawcp = (tidata_t) bp;
	tif->tif_rawcc = cc;
	return (1);
bad:
	TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "NeXTDecode: Not enough data for scanline %ld",
	    (long) tif->tif_row);
	return (0);
}

int
TIFFInitNeXT(TIFF* tif, int scheme)
{
	(void) scheme;
	tif->tif_decoderow = NeXTDecode;
	tif->tif_decodestrip = NeXTDecode;
	tif->tif_decodetile = NeXTDecode;
	return (1);
}
#endif 

