

#include "tiffiop.h"

static int
DumpModeEncode(TIFF* tif, tidata_t pp, tsize_t cc, tsample_t s)
{
	(void) s;
	while (cc > 0) {
		tsize_t n;

		n = cc;
		if (tif->tif_rawcc + n > tif->tif_rawdatasize)
			n = tif->tif_rawdatasize - tif->tif_rawcc;

		assert( n > 0 );
                
		
		if (tif->tif_rawcp != pp)
			_TIFFmemcpy(tif->tif_rawcp, pp, n);
		tif->tif_rawcp += n;
		tif->tif_rawcc += n;
		pp += n;
		cc -= n;
		if (tif->tif_rawcc >= tif->tif_rawdatasize &&
		    !TIFFFlushData1(tif))
			return (-1);
	}
	return (1);
}

static int
DumpModeDecode(TIFF* tif, tidata_t buf, tsize_t cc, tsample_t s)
{
	(void) s;

	if (tif->tif_rawcc < cc) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		    "DumpModeDecode: Not enough data for scanline %d",
		    tif->tif_row);
		return (0);
	}
	
	if (tif->tif_rawcp != buf)
		_TIFFmemcpy(buf, tif->tif_rawcp, cc);
	tif->tif_rawcp += cc;
	tif->tif_rawcc -= cc;
	return (1);
}

static int
DumpModeSeek(TIFF* tif, uint32 nrows)
{
	tif->tif_rawcp += nrows * tif->tif_scanlinesize;
	tif->tif_rawcc -= nrows * tif->tif_scanlinesize;
	return (1);
}

int
TIFFInitDumpMode(TIFF* tif, int scheme)
{
	(void) scheme;
	tif->tif_decoderow = DumpModeDecode;
	tif->tif_decodestrip = DumpModeDecode;
	tif->tif_decodetile = DumpModeDecode;
	tif->tif_encoderow = DumpModeEncode;
	tif->tif_encodestrip = DumpModeEncode;
	tif->tif_encodetile = DumpModeEncode;
	tif->tif_seek = DumpModeSeek;
	return (1);
}

