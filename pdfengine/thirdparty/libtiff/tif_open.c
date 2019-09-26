

#include "tiffiop.h"

static const long typemask[13] = {
	(long)0L,		
	(long)0x000000ffL,	
	(long)0xffffffffL,	
	(long)0x0000ffffL,	
	(long)0xffffffffL,	
	(long)0xffffffffL,	
	(long)0x000000ffL,	
	(long)0x000000ffL,	
	(long)0x0000ffffL,	
	(long)0xffffffffL,	
	(long)0xffffffffL,	
	(long)0xffffffffL,	
	(long)0xffffffffL,	
};
static const int bigTypeshift[13] = {
	0,		
	24,		
	0,		
	16,		
	0,		
	0,		
	24,		
	24,		
	16,		
	0,		
	0,		
	0,		
	0,		
};
static const int litTypeshift[13] = {
	0,		
	0,		
	0,		
	0,		
	0,		
	0,		
	0,		
	0,		
	0,		
	0,		
	0,		
	0,		
	0,		
};

static int
_tiffDummyMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	(void) fd; (void) pbase; (void) psize;
	return (0);
}

static void
_tiffDummyUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
	(void) fd; (void) base; (void) size;
}

static void
TIFFInitOrder(TIFF* tif, int magic)
{
	tif->tif_typemask = typemask;
	if (magic == TIFF_BIGENDIAN) {
		tif->tif_typeshift = bigTypeshift;
#ifndef WORDS_BIGENDIAN
		tif->tif_flags |= TIFF_SWAB;
#endif
	} else {
		tif->tif_typeshift = litTypeshift;
#ifdef WORDS_BIGENDIAN
		tif->tif_flags |= TIFF_SWAB;
#endif
	}
}

int
_TIFFgetMode(const char* mode, const char* module)
{
	int m = -1;

	switch (mode[0]) {
	case 'r':
		m = O_RDONLY;
		if (mode[1] == '+')
			m = O_RDWR;
		break;
	case 'w':
	case 'a':
		m = O_RDWR|O_CREAT;
		if (mode[0] == 'w')
			m |= O_TRUNC;
		break;
	default:
		TIFFErrorExt(0, module, "\"%s\": Bad mode", mode);
		break;
	}
	return (m);
}

TIFF*
TIFFClientOpen(
	const char* name, const char* mode,
	thandle_t clientdata,
	TIFFReadWriteProc readproc,
	TIFFReadWriteProc writeproc,
	TIFFSeekProc seekproc,
	TIFFCloseProc closeproc,
	TIFFSizeProc sizeproc,
	TIFFMapFileProc mapproc,
	TIFFUnmapFileProc unmapproc
)
{
	static const char module[] = "TIFFClientOpen";
	TIFF *tif;
	int m;
	const char* cp;

	m = _TIFFgetMode(mode, module);
	if (m == -1)
		goto bad2;
	tif = (TIFF *)_TIFFmalloc(sizeof (TIFF) + strlen(name) + 1);
	if (tif == NULL) {
		TIFFErrorExt(clientdata, module, "%s: Out of memory (TIFF structure)", name);
		goto bad2;
	}
	_TIFFmemset(tif, 0, sizeof (*tif));
	tif->tif_name = (char *)tif + sizeof (TIFF);
	strcpy(tif->tif_name, name);
	tif->tif_mode = m &~ (O_CREAT|O_TRUNC);
	tif->tif_curdir = (tdir_t) -1;		
	tif->tif_curoff = 0;
	tif->tif_curstrip = (tstrip_t) -1;	
	tif->tif_row = (uint32) -1;		
	tif->tif_clientdata = clientdata;
	if (!readproc || !writeproc || !seekproc || !closeproc || !sizeproc) {
		TIFFErrorExt(clientdata, module,
			  "One of the client procedures is NULL pointer.");
		goto bad2;
	}
	tif->tif_readproc = readproc;
	tif->tif_writeproc = writeproc;
	tif->tif_seekproc = seekproc;
	tif->tif_closeproc = closeproc;
	tif->tif_sizeproc = sizeproc;
        if (mapproc)
		tif->tif_mapproc = mapproc;
	else
		tif->tif_mapproc = _tiffDummyMapProc;
	if (unmapproc)
		tif->tif_unmapproc = unmapproc;
	else
		tif->tif_unmapproc = _tiffDummyUnmapProc;
	_TIFFSetDefaultCompressionState(tif);	
	
	tif->tif_flags = FILLORDER_MSB2LSB;
	if (m == O_RDONLY )
		tif->tif_flags |= TIFF_MAPPED;

#ifdef STRIPCHOP_DEFAULT
	if (m == O_RDONLY || m == O_RDWR)
		tif->tif_flags |= STRIPCHOP_DEFAULT;
#endif

	
	for (cp = mode; *cp; cp++)
		switch (*cp) {
		case 'b':
#ifndef WORDS_BIGENDIAN
		    if (m&O_CREAT)
				tif->tif_flags |= TIFF_SWAB;
#endif
			break;
		case 'l':
#ifdef WORDS_BIGENDIAN
			if ((m&O_CREAT))
				tif->tif_flags |= TIFF_SWAB;
#endif
			break;
		case 'B':
			tif->tif_flags = (tif->tif_flags &~ TIFF_FILLORDER) |
			    FILLORDER_MSB2LSB;
			break;
		case 'L':
			tif->tif_flags = (tif->tif_flags &~ TIFF_FILLORDER) |
			    FILLORDER_LSB2MSB;
			break;
		case 'H':
			tif->tif_flags = (tif->tif_flags &~ TIFF_FILLORDER) |
			    HOST_FILLORDER;
			break;
		case 'M':
			if (m == O_RDONLY)
				tif->tif_flags |= TIFF_MAPPED;
			break;
		case 'm':
			if (m == O_RDONLY)
				tif->tif_flags &= ~TIFF_MAPPED;
			break;
		case 'C':
			if (m == O_RDONLY)
				tif->tif_flags |= TIFF_STRIPCHOP;
			break;
		case 'c':
			if (m == O_RDONLY)
				tif->tif_flags &= ~TIFF_STRIPCHOP;
			break;
		case 'h':
			tif->tif_flags |= TIFF_HEADERONLY;
			break;
		}
	
	if (tif->tif_mode & O_TRUNC ||
	    !ReadOK(tif, &tif->tif_header, sizeof (TIFFHeader))) {
		if (tif->tif_mode == O_RDONLY) {
			TIFFErrorExt(tif->tif_clientdata, name,
				     "Cannot read TIFF header");
			goto bad;
		}
		
#ifdef WORDS_BIGENDIAN
		tif->tif_header.tiff_magic = tif->tif_flags & TIFF_SWAB
		    ? TIFF_LITTLEENDIAN : TIFF_BIGENDIAN;
#else
		tif->tif_header.tiff_magic = tif->tif_flags & TIFF_SWAB
		    ? TIFF_BIGENDIAN : TIFF_LITTLEENDIAN;
#endif
		tif->tif_header.tiff_version = TIFF_VERSION;
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&tif->tif_header.tiff_version);
		tif->tif_header.tiff_diroff = 0;	

                
                TIFFSeekFile( tif, 0, SEEK_SET );
               
		if (!WriteOK(tif, &tif->tif_header, sizeof (TIFFHeader))) {
			TIFFErrorExt(tif->tif_clientdata, name,
				     "Error writing TIFF header");
			goto bad;
		}
		
		TIFFInitOrder(tif, tif->tif_header.tiff_magic);
		
		if (!TIFFDefaultDirectory(tif))
			goto bad;
		tif->tif_diroff = 0;
		tif->tif_dirlist = NULL;
		tif->tif_dirlistsize = 0;
		tif->tif_dirnumber = 0;
		return (tif);
	}
	
	if (tif->tif_header.tiff_magic != TIFF_BIGENDIAN &&
	    tif->tif_header.tiff_magic != TIFF_LITTLEENDIAN
#if MDI_SUPPORT
	    &&
#if HOST_BIGENDIAN
	    tif->tif_header.tiff_magic != MDI_BIGENDIAN
#else
	    tif->tif_header.tiff_magic != MDI_LITTLEENDIAN
#endif
	    ) {
		TIFFErrorExt(tif->tif_clientdata, name,
			"Not a TIFF or MDI file, bad magic number %d (0x%x)",
#else
	    ) {
		TIFFErrorExt(tif->tif_clientdata, name,
			     "Not a TIFF file, bad magic number %d (0x%x)",
#endif
		    tif->tif_header.tiff_magic,
		    tif->tif_header.tiff_magic);
		goto bad;
	}
	TIFFInitOrder(tif, tif->tif_header.tiff_magic);
	
	if (tif->tif_flags & TIFF_SWAB) {
		TIFFSwabShort(&tif->tif_header.tiff_version);
		TIFFSwabLong(&tif->tif_header.tiff_diroff);
	}
	
	if (tif->tif_header.tiff_version == TIFF_BIGTIFF_VERSION) {
		TIFFErrorExt(tif->tif_clientdata, name,
                          "This is a BigTIFF file.  This format not supported\n"
                          "by this version of libtiff." );
		goto bad;
	}
	if (tif->tif_header.tiff_version != TIFF_VERSION) {
		TIFFErrorExt(tif->tif_clientdata, name,
		    "Not a TIFF file, bad version number %d (0x%x)",
		    tif->tif_header.tiff_version,
		    tif->tif_header.tiff_version);
		goto bad;
	}
	tif->tif_flags |= TIFF_MYBUFFER;
	tif->tif_rawcp = tif->tif_rawdata = 0;
	tif->tif_rawdatasize = 0;

	
	if (tif->tif_flags & TIFF_HEADERONLY)
		return (tif);

	
	switch (mode[0]) {
	case 'r':
		tif->tif_nextdiroff = tif->tif_header.tiff_diroff;
		
		if ((tif->tif_flags & TIFF_MAPPED) &&
	!TIFFMapFileContents(tif, (tdata_t*) &tif->tif_base, &tif->tif_size))
			tif->tif_flags &= ~TIFF_MAPPED;
		if (TIFFReadDirectory(tif)) {
			tif->tif_rawcc = -1;
			tif->tif_flags |= TIFF_BUFFERSETUP;
			return (tif);
		}
		break;
	case 'a':
		
		if (!TIFFDefaultDirectory(tif))
			goto bad;
		return (tif);
	}
bad:
	tif->tif_mode = O_RDONLY;	
        TIFFCleanup(tif);
bad2:
	return ((TIFF*)0);
}

const char *
TIFFFileName(TIFF* tif)
{
	return (tif->tif_name);
}

const char *
TIFFSetFileName(TIFF* tif, const char *name)
{
	const char* old_name = tif->tif_name;
	tif->tif_name = (char *)name;
	return (old_name);
}

int
TIFFFileno(TIFF* tif)
{
	return (tif->tif_fd);
}

int
TIFFSetFileno(TIFF* tif, int fd)
{
        int old_fd = tif->tif_fd;
	tif->tif_fd = fd;
	return old_fd;
}

thandle_t
TIFFClientdata(TIFF* tif)
{
	return (tif->tif_clientdata);
}

thandle_t
TIFFSetClientdata(TIFF* tif, thandle_t newvalue)
{
	thandle_t m = tif->tif_clientdata;
	tif->tif_clientdata = newvalue;
	return m;
}

int
TIFFGetMode(TIFF* tif)
{
	return (tif->tif_mode);
}

int
TIFFSetMode(TIFF* tif, int mode)
{
	int old_mode = tif->tif_mode;
	tif->tif_mode = mode;
	return (old_mode);
}

int
TIFFIsTiled(TIFF* tif)
{
	return (isTiled(tif));
}

uint32
TIFFCurrentRow(TIFF* tif)
{
	return (tif->tif_row);
}

tdir_t
TIFFCurrentDirectory(TIFF* tif)
{
	return (tif->tif_curdir);
}

tstrip_t
TIFFCurrentStrip(TIFF* tif)
{
	return (tif->tif_curstrip);
}

ttile_t
TIFFCurrentTile(TIFF* tif)
{
	return (tif->tif_curtile);
}

int
TIFFIsByteSwapped(TIFF* tif)
{
	return ((tif->tif_flags & TIFF_SWAB) != 0);
}

int
TIFFIsUpSampled(TIFF* tif)
{
	return (isUpSampled(tif));
}

int
TIFFIsMSB2LSB(TIFF* tif)
{
	return (isFillOrder(tif, FILLORDER_MSB2LSB));
}

int
TIFFIsBigEndian(TIFF* tif)
{
	return (tif->tif_header.tiff_magic == TIFF_BIGENDIAN);
}

TIFFReadWriteProc
TIFFGetReadProc(TIFF* tif)
{
	return (tif->tif_readproc);
}

TIFFReadWriteProc
TIFFGetWriteProc(TIFF* tif)
{
	return (tif->tif_writeproc);
}

TIFFSeekProc
TIFFGetSeekProc(TIFF* tif)
{
	return (tif->tif_seekproc);
}

TIFFCloseProc
TIFFGetCloseProc(TIFF* tif)
{
	return (tif->tif_closeproc);
}

TIFFSizeProc
TIFFGetSizeProc(TIFF* tif)
{
	return (tif->tif_sizeproc);
}

TIFFMapFileProc
TIFFGetMapFileProc(TIFF* tif)
{
	return (tif->tif_mapproc);
}

TIFFUnmapFileProc
TIFFGetUnmapFileProc(TIFF* tif)
{
	return (tif->tif_unmapproc);
}

