

#include "tiffiop.h"

static	int NotConfigured(TIFF*, int);

#ifndef	LZW_SUPPORT
#define	TIFFInitLZW		NotConfigured
#endif
#ifndef	PACKBITS_SUPPORT
#define	TIFFInitPackBits	NotConfigured
#endif
#ifndef	THUNDER_SUPPORT
#define	TIFFInitThunderScan	NotConfigured
#endif
#ifndef	NEXT_SUPPORT
#define	TIFFInitNeXT		NotConfigured
#endif
#ifndef	JPEG_SUPPORT
#define	TIFFInitJPEG		NotConfigured
#endif
#ifndef	OJPEG_SUPPORT
#define	TIFFInitOJPEG		NotConfigured
#endif
#ifndef	CCITT_SUPPORT
#define	TIFFInitCCITTRLE	NotConfigured
#define	TIFFInitCCITTRLEW	NotConfigured
#define	TIFFInitCCITTFax3	NotConfigured
#define	TIFFInitCCITTFax4	NotConfigured
#endif
#ifndef JBIG_SUPPORT
#define	TIFFInitJBIG		NotConfigured
#endif
#ifndef	ZIP_SUPPORT
#define	TIFFInitZIP		NotConfigured
#endif
#ifndef	PIXARLOG_SUPPORT
#define	TIFFInitPixarLog	NotConfigured
#endif
#ifndef LOGLUV_SUPPORT
#define TIFFInitSGILog		NotConfigured
#endif

#ifdef VMS
const TIFFCodec _TIFFBuiltinCODECS[] = {
#else
TIFFCodec _TIFFBuiltinCODECS[] = {
#endif
    { "None",		COMPRESSION_NONE,	TIFFInitDumpMode },
    { "LZW",		COMPRESSION_LZW,	TIFFInitLZW },
    { "PackBits",	COMPRESSION_PACKBITS,	TIFFInitPackBits },
    { "ThunderScan",	COMPRESSION_THUNDERSCAN,TIFFInitThunderScan },
    { "NeXT",		COMPRESSION_NEXT,	TIFFInitNeXT },
    { "JPEG",		COMPRESSION_JPEG,	TIFFInitJPEG },
    { "Old-style JPEG",	COMPRESSION_OJPEG,	TIFFInitOJPEG },
    { "CCITT RLE",	COMPRESSION_CCITTRLE,	TIFFInitCCITTRLE },
    { "CCITT RLE/W",	COMPRESSION_CCITTRLEW,	TIFFInitCCITTRLEW },
    { "CCITT Group 3",	COMPRESSION_CCITTFAX3,	TIFFInitCCITTFax3 },
    { "CCITT Group 4",	COMPRESSION_CCITTFAX4,	TIFFInitCCITTFax4 },
    { "ISO JBIG",	COMPRESSION_JBIG,	TIFFInitJBIG },
    { "Deflate",	COMPRESSION_DEFLATE,	TIFFInitZIP },
    { "AdobeDeflate",   COMPRESSION_ADOBE_DEFLATE , TIFFInitZIP }, 
    { "PixarLog",	COMPRESSION_PIXARLOG,	TIFFInitPixarLog },
    { "SGILog",		COMPRESSION_SGILOG,	TIFFInitSGILog },
    { "SGILog24",	COMPRESSION_SGILOG24,	TIFFInitSGILog },
    { NULL,             0,                      NULL }
};

static int
_notConfigured(TIFF* tif)
{
	const TIFFCodec* c = TIFFFindCODEC(tif->tif_dir.td_compression);
        char compression_code[20];
        
        sprintf( compression_code, "%d", tif->tif_dir.td_compression );
	TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
                     "%s compression support is not configured", 
                     c ? c->name : compression_code );
	return (0);
}

static int
NotConfigured(TIFF* tif, int scheme)
{
    (void) scheme;
    
    tif->tif_decodestatus = FALSE;
    tif->tif_setupdecode = _notConfigured;
    tif->tif_encodestatus = FALSE;
    tif->tif_setupencode = _notConfigured;
    return (1);
}

int
TIFFIsCODECConfigured(uint16 scheme)
{
	const TIFFCodec* codec = TIFFFindCODEC(scheme);

	if(codec == NULL) {
            return 0;
        }
        if(codec->init == NULL) {
            return 0;
        }
	if(codec->init != NotConfigured){
            return 1;
        }
	return 0;
}

