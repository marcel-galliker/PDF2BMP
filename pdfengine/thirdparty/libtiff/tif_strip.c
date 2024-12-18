

#include "tiffiop.h"

static uint32
summarize(TIFF* tif, size_t summand1, size_t summand2, const char* where)
{
	
	uint32	bytes = summand1 + summand2;

	if (bytes - summand1 != summand2) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "Integer overflow in %s", where);
		bytes = 0;
	}

	return (bytes);
}

static uint32
multiply(TIFF* tif, size_t nmemb, size_t elem_size, const char* where)
{
	uint32	bytes = nmemb * elem_size;

	if (elem_size && bytes / elem_size != nmemb) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "Integer overflow in %s", where);
		bytes = 0;
	}

	return (bytes);
}

tstrip_t
TIFFComputeStrip(TIFF* tif, uint32 row, tsample_t sample)
{
	TIFFDirectory *td = &tif->tif_dir;
	tstrip_t strip;

	strip = row / td->td_rowsperstrip;
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE) {
		if (sample >= td->td_samplesperpixel) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			    "%lu: Sample out of range, max %lu",
			    (unsigned long) sample, (unsigned long) td->td_samplesperpixel);
			return ((tstrip_t) 0);
		}
		strip += sample*td->td_stripsperimage;
	}
	return (strip);
}

tstrip_t
TIFFNumberOfStrips(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;
	tstrip_t nstrips;

	nstrips = (td->td_rowsperstrip == (uint32) -1 ? 1 :
	     TIFFhowmany(td->td_imagelength, td->td_rowsperstrip));
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE)
		nstrips = multiply(tif, nstrips, td->td_samplesperpixel,
				   "TIFFNumberOfStrips");
	return (nstrips);
}

tsize_t
TIFFVStripSize(TIFF* tif, uint32 nrows)
{
	TIFFDirectory *td = &tif->tif_dir;

	if (nrows == (uint32) -1)
		nrows = td->td_imagelength;
	if (td->td_planarconfig == PLANARCONFIG_CONTIG &&
	    td->td_photometric == PHOTOMETRIC_YCBCR &&
	    !isUpSampled(tif)) {
		
		uint16 ycbcrsubsampling[2];
		tsize_t w, scanline, samplingarea;

		TIFFGetField( tif, TIFFTAG_YCBCRSUBSAMPLING,
			      ycbcrsubsampling + 0,
			      ycbcrsubsampling + 1 );

		samplingarea = ycbcrsubsampling[0]*ycbcrsubsampling[1];
		if (samplingarea == 0) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
				     "Invalid YCbCr subsampling");
			return 0;
		}

		w = TIFFroundup(td->td_imagewidth, ycbcrsubsampling[0]);
		scanline = TIFFhowmany8(multiply(tif, w, td->td_bitspersample,
						 "TIFFVStripSize"));
		nrows = TIFFroundup(nrows, ycbcrsubsampling[1]);
		
		scanline = multiply(tif, nrows, scanline, "TIFFVStripSize");
		return ((tsize_t)
		    summarize(tif, scanline,
			      multiply(tif, 2, scanline / samplingarea,
				       "TIFFVStripSize"), "TIFFVStripSize"));
	} else
		return ((tsize_t) multiply(tif, nrows, TIFFScanlineSize(tif),
					   "TIFFVStripSize"));
}

tsize_t
TIFFRawStripSize(TIFF* tif, tstrip_t strip)
{
	TIFFDirectory* td = &tif->tif_dir;
	tsize_t bytecount = td->td_stripbytecount[strip];

	if (bytecount <= 0) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			  "%lu: Invalid strip byte count, strip %lu",
			  (unsigned long) bytecount, (unsigned long) strip);
		bytecount = (tsize_t) -1;
	}

	return bytecount;
}

tsize_t
TIFFStripSize(TIFF* tif)
{
	TIFFDirectory* td = &tif->tif_dir;
	uint32 rps = td->td_rowsperstrip;
	if (rps > td->td_imagelength)
		rps = td->td_imagelength;
	return (TIFFVStripSize(tif, rps));
}

uint32
TIFFDefaultStripSize(TIFF* tif, uint32 request)
{
	return (*tif->tif_defstripsize)(tif, request);
}

uint32
_TIFFDefaultStripSize(TIFF* tif, uint32 s)
{
	if ((int32) s < 1) {
		
		tsize_t scanline = TIFFScanlineSize(tif);
		s = (uint32)STRIP_SIZE_DEFAULT / (scanline == 0 ? 1 : scanline);
		if (s == 0)		
			s = 1;
	}
	return (s);
}

tsize_t
TIFFScanlineSize(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;
	tsize_t scanline;

	if (td->td_planarconfig == PLANARCONFIG_CONTIG) {
		if (td->td_photometric == PHOTOMETRIC_YCBCR
		    && !isUpSampled(tif)) {
			uint16 ycbcrsubsampling[2];

			TIFFGetField(tif, TIFFTAG_YCBCRSUBSAMPLING,
				     ycbcrsubsampling + 0,
				     ycbcrsubsampling + 1);

			if (ycbcrsubsampling[0] == 0) {
				TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
					     "Invalid YCbCr subsampling");
				return 0;
			}

			scanline = TIFFroundup(td->td_imagewidth,
					       ycbcrsubsampling[0]);
			scanline = TIFFhowmany8(multiply(tif, scanline,
							 td->td_bitspersample,
							 "TIFFScanlineSize"));
			return ((tsize_t)
				summarize(tif, scanline,
					  multiply(tif, 2,
						scanline / ycbcrsubsampling[0],
						"TIFFVStripSize"),
					  "TIFFVStripSize"));
		} else {
			scanline = multiply(tif, td->td_imagewidth,
					    td->td_samplesperpixel,
					    "TIFFScanlineSize");
		}
	} else
		scanline = td->td_imagewidth;
	return ((tsize_t) TIFFhowmany8(multiply(tif, scanline,
						td->td_bitspersample,
						"TIFFScanlineSize")));
}

tsize_t
TIFFOldScanlineSize(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;
	tsize_t scanline;

	scanline = multiply (tif, td->td_bitspersample, td->td_imagewidth,
			     "TIFFScanlineSize");
	if (td->td_planarconfig == PLANARCONFIG_CONTIG)
		scanline = multiply (tif, scanline, td->td_samplesperpixel,
				     "TIFFScanlineSize");
	return ((tsize_t) TIFFhowmany8(scanline));
}

tsize_t
TIFFNewScanlineSize(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;
	tsize_t scanline;

	if (td->td_planarconfig == PLANARCONFIG_CONTIG) {
		if (td->td_photometric == PHOTOMETRIC_YCBCR
		    && !isUpSampled(tif)) {
			uint16 ycbcrsubsampling[2];

			TIFFGetField(tif, TIFFTAG_YCBCRSUBSAMPLING,
				     ycbcrsubsampling + 0,
				     ycbcrsubsampling + 1);

			if (ycbcrsubsampling[0]*ycbcrsubsampling[1] == 0) {
				TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
					     "Invalid YCbCr subsampling");
				return 0;
			}

			return((tsize_t) ((((td->td_imagewidth+ycbcrsubsampling[0]-1)
					    /ycbcrsubsampling[0])
					   *(ycbcrsubsampling[0]*ycbcrsubsampling[1]+2)
					   *td->td_bitspersample+7)
					  /8)/ycbcrsubsampling[1]);

		} else {
			scanline = multiply(tif, td->td_imagewidth,
					    td->td_samplesperpixel,
					    "TIFFScanlineSize");
		}
	} else
		scanline = td->td_imagewidth;
	return ((tsize_t) TIFFhowmany8(multiply(tif, scanline,
						td->td_bitspersample,
						"TIFFScanlineSize")));
}

tsize_t
TIFFRasterScanlineSize(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;
	tsize_t scanline;
	
	scanline = multiply (tif, td->td_bitspersample, td->td_imagewidth,
			     "TIFFRasterScanlineSize");
	if (td->td_planarconfig == PLANARCONFIG_CONTIG) {
		scanline = multiply (tif, scanline, td->td_samplesperpixel,
				     "TIFFRasterScanlineSize");
		return ((tsize_t) TIFFhowmany8(scanline));
	} else
		return ((tsize_t) multiply (tif, TIFFhowmany8(scanline),
					    td->td_samplesperpixel,
					    "TIFFRasterScanlineSize"));
}

