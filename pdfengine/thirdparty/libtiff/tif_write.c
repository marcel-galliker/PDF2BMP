

#include "tiffiop.h"
#include <stdio.h>

#define	STRIPINCR	20		

#define	WRITECHECKSTRIPS(tif, module)				\
	(((tif)->tif_flags&TIFF_BEENWRITING) || TIFFWriteCheck((tif),0,module))
#define	WRITECHECKTILES(tif, module)				\
	(((tif)->tif_flags&TIFF_BEENWRITING) || TIFFWriteCheck((tif),1,module))
#define	BUFFERCHECK(tif)					\
	((((tif)->tif_flags & TIFF_BUFFERSETUP) && tif->tif_rawdata) ||	\
	    TIFFWriteBufferSetup((tif), NULL, (tsize_t) -1))

static	int TIFFGrowStrips(TIFF*, int, const char*);
static	int TIFFAppendToStrip(TIFF*, tstrip_t, tidata_t, tsize_t);

int
TIFFWriteScanline(TIFF* tif, tdata_t buf, uint32 row, tsample_t sample)
{
	static const char module[] = "TIFFWriteScanline";
	register TIFFDirectory *td;
	int status, imagegrew = 0;
	tstrip_t strip;

	if (!WRITECHECKSTRIPS(tif, module))
		return (-1);
	
	if (!BUFFERCHECK(tif))
		return (-1);
	td = &tif->tif_dir;
	
	if (row >= td->td_imagelength) {	
		if (td->td_planarconfig == PLANARCONFIG_SEPARATE) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		"Can not change \"ImageLength\" when using separate planes");
			return (-1);
		}
		td->td_imagelength = row+1;
		imagegrew = 1;
	}
	
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE) {
		if (sample >= td->td_samplesperpixel) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			    "%d: Sample out of range, max %d",
			    sample, td->td_samplesperpixel);
			return (-1);
		}
		strip = sample*td->td_stripsperimage + row/td->td_rowsperstrip;
	} else
		strip = row / td->td_rowsperstrip;
	
	if (strip >= td->td_nstrips && !TIFFGrowStrips(tif, 1, module))
		return (-1);
	if (strip != tif->tif_curstrip) {
		
		if (!TIFFFlushData(tif))
			return (-1);
		tif->tif_curstrip = strip;
		
		if (strip >= td->td_stripsperimage && imagegrew)
			td->td_stripsperimage =
			    TIFFhowmany(td->td_imagelength,td->td_rowsperstrip);
		tif->tif_row =
		    (strip % td->td_stripsperimage) * td->td_rowsperstrip;
		if ((tif->tif_flags & TIFF_CODERSETUP) == 0) {
			if (!(*tif->tif_setupencode)(tif))
				return (-1);
			tif->tif_flags |= TIFF_CODERSETUP;
		}
        
		tif->tif_rawcc = 0;
		tif->tif_rawcp = tif->tif_rawdata;

		if( td->td_stripbytecount[strip] > 0 )
		{
			
			td->td_stripbytecount[strip] = 0;

			
			tif->tif_curoff = 0;
		}

		if (!(*tif->tif_preencode)(tif, sample))
			return (-1);
		tif->tif_flags |= TIFF_POSTENCODE;
	}
	
	if (row != tif->tif_row) {
		if (row < tif->tif_row) {
			
			tif->tif_row = (strip % td->td_stripsperimage) *
			    td->td_rowsperstrip;
			tif->tif_rawcp = tif->tif_rawdata;
		}
		
		if (!(*tif->tif_seek)(tif, row - tif->tif_row))
			return (-1);
		tif->tif_row = row;
	}

        
        tif->tif_postdecode( tif, (tidata_t) buf, tif->tif_scanlinesize );

	status = (*tif->tif_encoderow)(tif, (tidata_t) buf,
	    tif->tif_scanlinesize, sample);

        
	tif->tif_row = row + 1;
	return (status);
}

tsize_t
TIFFWriteEncodedStrip(TIFF* tif, tstrip_t strip, tdata_t data, tsize_t cc)
{
	static const char module[] = "TIFFWriteEncodedStrip";
	TIFFDirectory *td = &tif->tif_dir;
	tsample_t sample;

	if (!WRITECHECKSTRIPS(tif, module))
		return ((tsize_t) -1);
	
	if (strip >= td->td_nstrips) {
		if (td->td_planarconfig == PLANARCONFIG_SEPARATE) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		"Can not grow image by strips when using separate planes");
			return ((tsize_t) -1);
		}
		if (!TIFFGrowStrips(tif, 1, module))
			return ((tsize_t) -1);
		td->td_stripsperimage =
		    TIFFhowmany(td->td_imagelength, td->td_rowsperstrip);
	}
	
	if (!BUFFERCHECK(tif))
		return ((tsize_t) -1);
	tif->tif_curstrip = strip;
	tif->tif_row = (strip % td->td_stripsperimage) * td->td_rowsperstrip;
	if ((tif->tif_flags & TIFF_CODERSETUP) == 0) {
		if (!(*tif->tif_setupencode)(tif))
			return ((tsize_t) -1);
		tif->tif_flags |= TIFF_CODERSETUP;
	}
        
	tif->tif_rawcc = 0;
	tif->tif_rawcp = tif->tif_rawdata;

        if( td->td_stripbytecount[strip] > 0 )
        {
	    
            tif->tif_curoff = 0;
        }
        
	tif->tif_flags &= ~TIFF_POSTENCODE;
	sample = (tsample_t)(strip / td->td_stripsperimage);
	if (!(*tif->tif_preencode)(tif, sample))
		return ((tsize_t) -1);

        
        tif->tif_postdecode( tif, (tidata_t) data, cc );

	if (!(*tif->tif_encodestrip)(tif, (tidata_t) data, cc, sample))
		return ((tsize_t) 0);
	if (!(*tif->tif_postencode)(tif))
		return ((tsize_t) -1);
	if (!isFillOrder(tif, td->td_fillorder) &&
	    (tif->tif_flags & TIFF_NOBITREV) == 0)
		TIFFReverseBits(tif->tif_rawdata, tif->tif_rawcc);
	if (tif->tif_rawcc > 0 &&
	    !TIFFAppendToStrip(tif, strip, tif->tif_rawdata, tif->tif_rawcc))
		return ((tsize_t) -1);
	tif->tif_rawcc = 0;
	tif->tif_rawcp = tif->tif_rawdata;
	return (cc);
}

tsize_t
TIFFWriteRawStrip(TIFF* tif, tstrip_t strip, tdata_t data, tsize_t cc)
{
	static const char module[] = "TIFFWriteRawStrip";
	TIFFDirectory *td = &tif->tif_dir;

	if (!WRITECHECKSTRIPS(tif, module))
		return ((tsize_t) -1);
	
	if (strip >= td->td_nstrips) {
		if (td->td_planarconfig == PLANARCONFIG_SEPARATE) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		"Can not grow image by strips when using separate planes");
			return ((tsize_t) -1);
		}
		
		if (strip >= td->td_stripsperimage)
			td->td_stripsperimage =
			    TIFFhowmany(td->td_imagelength,td->td_rowsperstrip);
		if (!TIFFGrowStrips(tif, 1, module))
			return ((tsize_t) -1);
	}
	tif->tif_curstrip = strip;
	tif->tif_row = (strip % td->td_stripsperimage) * td->td_rowsperstrip;
	return (TIFFAppendToStrip(tif, strip, (tidata_t) data, cc) ?
	    cc : (tsize_t) -1);
}

tsize_t
TIFFWriteTile(TIFF* tif,
    tdata_t buf, uint32 x, uint32 y, uint32 z, tsample_t s)
{
	if (!TIFFCheckTile(tif, x, y, z, s))
		return (-1);
	
	return (TIFFWriteEncodedTile(tif,
	    TIFFComputeTile(tif, x, y, z, s), buf, (tsize_t) -1));
}

tsize_t
TIFFWriteEncodedTile(TIFF* tif, ttile_t tile, tdata_t data, tsize_t cc)
{
	static const char module[] = "TIFFWriteEncodedTile";
	TIFFDirectory *td;
	tsample_t sample;

	if (!WRITECHECKTILES(tif, module))
		return ((tsize_t) -1);
	td = &tif->tif_dir;
	if (tile >= td->td_nstrips) {
		TIFFErrorExt(tif->tif_clientdata, module, "%s: Tile %lu out of range, max %lu",
		    tif->tif_name, (unsigned long) tile, (unsigned long) td->td_nstrips);
		return ((tsize_t) -1);
	}
	
	if (!BUFFERCHECK(tif))
		return ((tsize_t) -1);
	tif->tif_curtile = tile;

	tif->tif_rawcc = 0;
	tif->tif_rawcp = tif->tif_rawdata;

        if( td->td_stripbytecount[tile] > 0 )
        {
	    
            tif->tif_curoff = 0;
        }
        
	
	tif->tif_row = (tile % TIFFhowmany(td->td_imagelength, td->td_tilelength))
		* td->td_tilelength;
	tif->tif_col = (tile % TIFFhowmany(td->td_imagewidth, td->td_tilewidth))
		* td->td_tilewidth;

	if ((tif->tif_flags & TIFF_CODERSETUP) == 0) {
		if (!(*tif->tif_setupencode)(tif))
			return ((tsize_t) -1);
		tif->tif_flags |= TIFF_CODERSETUP;
	}
	tif->tif_flags &= ~TIFF_POSTENCODE;
	sample = (tsample_t)(tile/td->td_stripsperimage);
	if (!(*tif->tif_preencode)(tif, sample))
		return ((tsize_t) -1);
	
	if ( cc < 1 || cc > tif->tif_tilesize)
		cc = tif->tif_tilesize;

        
        tif->tif_postdecode( tif, (tidata_t) data, cc );

	if (!(*tif->tif_encodetile)(tif, (tidata_t) data, cc, sample))
		return ((tsize_t) 0);
	if (!(*tif->tif_postencode)(tif))
		return ((tsize_t) -1);
	if (!isFillOrder(tif, td->td_fillorder) &&
	    (tif->tif_flags & TIFF_NOBITREV) == 0)
		TIFFReverseBits((unsigned char *)tif->tif_rawdata, tif->tif_rawcc);
	if (tif->tif_rawcc > 0 && !TIFFAppendToStrip(tif, tile,
	    tif->tif_rawdata, tif->tif_rawcc))
		return ((tsize_t) -1);
	tif->tif_rawcc = 0;
	tif->tif_rawcp = tif->tif_rawdata;
	return (cc);
}

tsize_t
TIFFWriteRawTile(TIFF* tif, ttile_t tile, tdata_t data, tsize_t cc)
{
	static const char module[] = "TIFFWriteRawTile";

	if (!WRITECHECKTILES(tif, module))
		return ((tsize_t) -1);
	if (tile >= tif->tif_dir.td_nstrips) {
		TIFFErrorExt(tif->tif_clientdata, module, "%s: Tile %lu out of range, max %lu",
		    tif->tif_name, (unsigned long) tile,
		    (unsigned long) tif->tif_dir.td_nstrips);
		return ((tsize_t) -1);
	}
	return (TIFFAppendToStrip(tif, tile, (tidata_t) data, cc) ?
	    cc : (tsize_t) -1);
}

#define	isUnspecified(tif, f) \
    (TIFFFieldSet(tif,f) && (tif)->tif_dir.td_imagelength == 0)

int
TIFFSetupStrips(TIFF* tif)
{
	TIFFDirectory* td = &tif->tif_dir;

	if (isTiled(tif))
		td->td_stripsperimage =
		    isUnspecified(tif, FIELD_TILEDIMENSIONS) ?
			td->td_samplesperpixel : TIFFNumberOfTiles(tif);
	else
		td->td_stripsperimage =
		    isUnspecified(tif, FIELD_ROWSPERSTRIP) ?
			td->td_samplesperpixel : TIFFNumberOfStrips(tif);
	td->td_nstrips = td->td_stripsperimage;
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE)
		td->td_stripsperimage /= td->td_samplesperpixel;
	td->td_stripoffset = (uint32 *)
	    _TIFFmalloc(td->td_nstrips * sizeof (uint32));
	td->td_stripbytecount = (uint32 *)
	    _TIFFmalloc(td->td_nstrips * sizeof (uint32));
	if (td->td_stripoffset == NULL || td->td_stripbytecount == NULL)
		return (0);
	
	_TIFFmemset(td->td_stripoffset, 0, td->td_nstrips*sizeof (uint32));
	_TIFFmemset(td->td_stripbytecount, 0, td->td_nstrips*sizeof (uint32));
	TIFFSetFieldBit(tif, FIELD_STRIPOFFSETS);
	TIFFSetFieldBit(tif, FIELD_STRIPBYTECOUNTS);
	return (1);
}
#undef isUnspecified

int
TIFFWriteCheck(TIFF* tif, int tiles, const char* module)
{
	if (tif->tif_mode == O_RDONLY) {
		TIFFErrorExt(tif->tif_clientdata, module, "%s: File not open for writing",
		    tif->tif_name);
		return (0);
	}
	if (tiles ^ isTiled(tif)) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name, tiles ?
		    "Can not write tiles to a stripped image" :
		    "Can not write scanlines to a tiled image");
		return (0);
	}
        
	
	if (!TIFFFieldSet(tif, FIELD_IMAGEDIMENSIONS)) {
		TIFFErrorExt(tif->tif_clientdata, module,
		    "%s: Must set \"ImageWidth\" before writing data",
		    tif->tif_name);
		return (0);
	}
	if (tif->tif_dir.td_samplesperpixel == 1) {
		
		if (!TIFFFieldSet(tif, FIELD_PLANARCONFIG))
                    tif->tif_dir.td_planarconfig = PLANARCONFIG_CONTIG;
	} else {
		if (!TIFFFieldSet(tif, FIELD_PLANARCONFIG)) {
			TIFFErrorExt(tif->tif_clientdata, module,
		    "%s: Must set \"PlanarConfiguration\" before writing data",
			    tif->tif_name);
			return (0);
		}
	}
	if (tif->tif_dir.td_stripoffset == NULL && !TIFFSetupStrips(tif)) {
		tif->tif_dir.td_nstrips = 0;
		TIFFErrorExt(tif->tif_clientdata, module, "%s: No space for %s arrays",
		    tif->tif_name, isTiled(tif) ? "tile" : "strip");
		return (0);
	}
	tif->tif_tilesize = isTiled(tif) ? TIFFTileSize(tif) : (tsize_t) -1;
	tif->tif_scanlinesize = TIFFScanlineSize(tif);
	tif->tif_flags |= TIFF_BEENWRITING;
	return (1);
}

int
TIFFWriteBufferSetup(TIFF* tif, tdata_t bp, tsize_t size)
{
	static const char module[] = "TIFFWriteBufferSetup";

	if (tif->tif_rawdata) {
		if (tif->tif_flags & TIFF_MYBUFFER) {
			_TIFFfree(tif->tif_rawdata);
			tif->tif_flags &= ~TIFF_MYBUFFER;
		}
		tif->tif_rawdata = NULL;
	}
	if (size == (tsize_t) -1) {
		size = (isTiled(tif) ?
		    tif->tif_tilesize : TIFFStripSize(tif));
		
		if (size < 8*1024)
			size = 8*1024;
		bp = NULL;			
	}
	if (bp == NULL) {
		bp = _TIFFmalloc(size);
		if (bp == NULL) {
			TIFFErrorExt(tif->tif_clientdata, module, "%s: No space for output buffer",
			    tif->tif_name);
			return (0);
		}
		tif->tif_flags |= TIFF_MYBUFFER;
	} else
		tif->tif_flags &= ~TIFF_MYBUFFER;
	tif->tif_rawdata = (tidata_t) bp;
	tif->tif_rawdatasize = size;
	tif->tif_rawcc = 0;
	tif->tif_rawcp = tif->tif_rawdata;
	tif->tif_flags |= TIFF_BUFFERSETUP;
	return (1);
}

static int
TIFFGrowStrips(TIFF* tif, int delta, const char* module)
{
	TIFFDirectory	*td = &tif->tif_dir;
	uint32		*new_stripoffset, *new_stripbytecount;

	assert(td->td_planarconfig == PLANARCONFIG_CONTIG);
	new_stripoffset = (uint32*)_TIFFrealloc(td->td_stripoffset,
		(td->td_nstrips + delta) * sizeof (uint32));
	new_stripbytecount = (uint32*)_TIFFrealloc(td->td_stripbytecount,
		(td->td_nstrips + delta) * sizeof (uint32));
	if (new_stripoffset == NULL || new_stripbytecount == NULL) {
		if (new_stripoffset)
			_TIFFfree(new_stripoffset);
		if (new_stripbytecount)
			_TIFFfree(new_stripbytecount);
		td->td_nstrips = 0;
		TIFFErrorExt(tif->tif_clientdata, module, "%s: No space to expand strip arrays",
			  tif->tif_name);
		return (0);
	}
	td->td_stripoffset = new_stripoffset;
	td->td_stripbytecount = new_stripbytecount;
	_TIFFmemset(td->td_stripoffset + td->td_nstrips,
		    0, delta*sizeof (uint32));
	_TIFFmemset(td->td_stripbytecount + td->td_nstrips,
		    0, delta*sizeof (uint32));
	td->td_nstrips += delta;
	return (1);
}

static int
TIFFAppendToStrip(TIFF* tif, tstrip_t strip, tidata_t data, tsize_t cc)
{
	static const char module[] = "TIFFAppendToStrip";
	TIFFDirectory *td = &tif->tif_dir;

	if (td->td_stripoffset[strip] == 0 || tif->tif_curoff == 0) {
            assert(td->td_nstrips > 0);

            if( td->td_stripbytecount[strip] != 0 
                && td->td_stripoffset[strip] != 0 
                && td->td_stripbytecount[strip] >= cc )
            {
                
                if (!SeekOK(tif, td->td_stripoffset[strip])) {
                    TIFFErrorExt(tif->tif_clientdata, module,
                                 "Seek error at scanline %lu",
                                 (unsigned long)tif->tif_row);
                    return (0);
                }
            }
            else
            {
                
                td->td_stripoffset[strip] = TIFFSeekFile(tif, 0, SEEK_END);
            }

            tif->tif_curoff = td->td_stripoffset[strip];

            
            td->td_stripbytecount[strip] = 0;
	}

	if (!WriteOK(tif, data, cc)) {
		TIFFErrorExt(tif->tif_clientdata, module, "Write error at scanline %lu",
		    (unsigned long) tif->tif_row);
		    return (0);
	}
	tif->tif_curoff =  tif->tif_curoff+cc;
	td->td_stripbytecount[strip] += cc;
	return (1);
}

int
TIFFFlushData1(TIFF* tif)
{
	if (tif->tif_rawcc > 0) {
		if (!isFillOrder(tif, tif->tif_dir.td_fillorder) &&
		    (tif->tif_flags & TIFF_NOBITREV) == 0)
			TIFFReverseBits((unsigned char *)tif->tif_rawdata,
			    tif->tif_rawcc);
		if (!TIFFAppendToStrip(tif,
		    isTiled(tif) ? tif->tif_curtile : tif->tif_curstrip,
		    tif->tif_rawdata, tif->tif_rawcc))
			return (0);
		tif->tif_rawcc = 0;
		tif->tif_rawcp = tif->tif_rawdata;
	}
	return (1);
}

void
TIFFSetWriteOffset(TIFF* tif, toff_t off)
{
	tif->tif_curoff = off;
}

