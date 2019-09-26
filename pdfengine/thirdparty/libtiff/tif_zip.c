

#include "tiffiop.h"
#ifdef ZIP_SUPPORT

#include "tif_predict.h"
#include "zlib.h"

#include <stdio.h>

#if !defined(Z_NO_COMPRESSION) || !defined(Z_DEFLATED)
#error "Antiquated ZLIB software; you must use version 1.0 or later"
#endif

typedef	struct {
	TIFFPredictorState predict;
	z_stream	stream;
	int		zipquality;		
	int		state;			
#define ZSTATE_INIT_DECODE 0x01
#define ZSTATE_INIT_ENCODE 0x02

	TIFFVGetMethod	vgetparent;		
	TIFFVSetMethod	vsetparent;		
} ZIPState;

#define	ZState(tif)		((ZIPState*) (tif)->tif_data)
#define	DecoderState(tif)	ZState(tif)
#define	EncoderState(tif)	ZState(tif)

static	int ZIPEncode(TIFF*, tidata_t, tsize_t, tsample_t);
static	int ZIPDecode(TIFF*, tidata_t, tsize_t, tsample_t);

static int
ZIPSetupDecode(TIFF* tif)
{
	ZIPState* sp = DecoderState(tif);
	static const char module[] = "ZIPSetupDecode";

	assert(sp != NULL);
        
        
	if (sp->state & ZSTATE_INIT_ENCODE) {
            deflateEnd(&sp->stream);
            sp->state = 0;
        }

	if (inflateInit(&sp->stream) != Z_OK) {
		TIFFErrorExt(tif->tif_clientdata, module, "%s: %s", tif->tif_name, sp->stream.msg);
		return (0);
	} else {
		sp->state |= ZSTATE_INIT_DECODE;
		return (1);
	}
}

static int
ZIPPreDecode(TIFF* tif, tsample_t s)
{
	ZIPState* sp = DecoderState(tif);

	(void) s;
	assert(sp != NULL);

        if( (sp->state & ZSTATE_INIT_DECODE) == 0 )
            tif->tif_setupdecode( tif );

	sp->stream.next_in = tif->tif_rawdata;
	sp->stream.avail_in = tif->tif_rawcc;
	return (inflateReset(&sp->stream) == Z_OK);
}

static int
ZIPDecode(TIFF* tif, tidata_t op, tsize_t occ, tsample_t s)
{
	ZIPState* sp = DecoderState(tif);
	static const char module[] = "ZIPDecode";

	(void) s;
	assert(sp != NULL);
        assert(sp->state == ZSTATE_INIT_DECODE);

	sp->stream.next_out = op;
	sp->stream.avail_out = occ;
	do {
		int state = inflate(&sp->stream, Z_PARTIAL_FLUSH);
		if (state == Z_STREAM_END)
			break;
		if (state == Z_DATA_ERROR) {
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Decoding error at scanline %d, %s",
			    tif->tif_name, tif->tif_row, sp->stream.msg);
			if (inflateSync(&sp->stream) != Z_OK)
				return (0);
			continue;
		}
		if (state != Z_OK) {
			TIFFErrorExt(tif->tif_clientdata, module, "%s: zlib error: %s",
			    tif->tif_name, sp->stream.msg);
			return (0);
		}
	} while (sp->stream.avail_out > 0);
	if (sp->stream.avail_out != 0) {
		TIFFErrorExt(tif->tif_clientdata, module,
		    "%s: Not enough data at scanline %d (short %d bytes)",
		    tif->tif_name, tif->tif_row, sp->stream.avail_out);
		return (0);
	}
	return (1);
}

static int
ZIPSetupEncode(TIFF* tif)
{
	ZIPState* sp = EncoderState(tif);
	static const char module[] = "ZIPSetupEncode";

	assert(sp != NULL);
	if (sp->state & ZSTATE_INIT_DECODE) {
            inflateEnd(&sp->stream);
            sp->state = 0;
        }

	if (deflateInit(&sp->stream, sp->zipquality) != Z_OK) {
		TIFFErrorExt(tif->tif_clientdata, module, "%s: %s", tif->tif_name, sp->stream.msg);
		return (0);
	} else {
		sp->state |= ZSTATE_INIT_ENCODE;
		return (1);
	}
}

static int
ZIPPreEncode(TIFF* tif, tsample_t s)
{
	ZIPState *sp = EncoderState(tif);

	(void) s;
	assert(sp != NULL);
        if( sp->state != ZSTATE_INIT_ENCODE )
            tif->tif_setupencode( tif );

	sp->stream.next_out = tif->tif_rawdata;
	sp->stream.avail_out = tif->tif_rawdatasize;
	return (deflateReset(&sp->stream) == Z_OK);
}

static int
ZIPEncode(TIFF* tif, tidata_t bp, tsize_t cc, tsample_t s)
{
	ZIPState *sp = EncoderState(tif);
	static const char module[] = "ZIPEncode";

        assert(sp != NULL);
        assert(sp->state == ZSTATE_INIT_ENCODE);

	(void) s;
	sp->stream.next_in = bp;
	sp->stream.avail_in = cc;
	do {
		if (deflate(&sp->stream, Z_NO_FLUSH) != Z_OK) {
			TIFFErrorExt(tif->tif_clientdata, module, "%s: Encoder error: %s",
			    tif->tif_name, sp->stream.msg);
			return (0);
		}
		if (sp->stream.avail_out == 0) {
			tif->tif_rawcc = tif->tif_rawdatasize;
			TIFFFlushData1(tif);
			sp->stream.next_out = tif->tif_rawdata;
			sp->stream.avail_out = tif->tif_rawdatasize;
		}
	} while (sp->stream.avail_in > 0);
	return (1);
}

static int
ZIPPostEncode(TIFF* tif)
{
	ZIPState *sp = EncoderState(tif);
	static const char module[] = "ZIPPostEncode";
	int state;

	sp->stream.avail_in = 0;
	do {
		state = deflate(&sp->stream, Z_FINISH);
		switch (state) {
		case Z_STREAM_END:
		case Z_OK:
		    if ((int)sp->stream.avail_out != (int)tif->tif_rawdatasize)
                    {
			    tif->tif_rawcc =
				tif->tif_rawdatasize - sp->stream.avail_out;
			    TIFFFlushData1(tif);
			    sp->stream.next_out = tif->tif_rawdata;
			    sp->stream.avail_out = tif->tif_rawdatasize;
		    }
		    break;
		default:
			TIFFErrorExt(tif->tif_clientdata, module, "%s: zlib error: %s",
			tif->tif_name, sp->stream.msg);
		    return (0);
		}
	} while (state != Z_STREAM_END);
	return (1);
}

static void
ZIPCleanup(TIFF* tif)
{
	ZIPState* sp = ZState(tif);

	assert(sp != 0);

	(void)TIFFPredictorCleanup(tif);

	tif->tif_tagmethods.vgetfield = sp->vgetparent;
	tif->tif_tagmethods.vsetfield = sp->vsetparent;

	if (sp->state & ZSTATE_INIT_ENCODE) {
            deflateEnd(&sp->stream);
            sp->state = 0;
        } else if( sp->state & ZSTATE_INIT_DECODE) {
            inflateEnd(&sp->stream);
            sp->state = 0;
	}
	_TIFFfree(sp);
	tif->tif_data = NULL;

	_TIFFSetDefaultCompressionState(tif);
}

static int
ZIPVSetField(TIFF* tif, ttag_t tag, va_list ap)
{
	ZIPState* sp = ZState(tif);
	static const char module[] = "ZIPVSetField";

	switch (tag) {
	case TIFFTAG_ZIPQUALITY:
		sp->zipquality = va_arg(ap, int);
		if ( sp->state&ZSTATE_INIT_ENCODE ) {
			if (deflateParams(&sp->stream,
			    sp->zipquality, Z_DEFAULT_STRATEGY) != Z_OK) {
				TIFFErrorExt(tif->tif_clientdata, module, "%s: zlib error: %s",
				    tif->tif_name, sp->stream.msg);
				return (0);
			}
		}
		return (1);
	default:
		return (*sp->vsetparent)(tif, tag, ap);
	}
	
}

static int
ZIPVGetField(TIFF* tif, ttag_t tag, va_list ap)
{
	ZIPState* sp = ZState(tif);

	switch (tag) {
	case TIFFTAG_ZIPQUALITY:
		*va_arg(ap, int*) = sp->zipquality;
		break;
	default:
		return (*sp->vgetparent)(tif, tag, ap);
	}
	return (1);
}

static const TIFFFieldInfo zipFieldInfo[] = {
    { TIFFTAG_ZIPQUALITY,	 0, 0,	TIFF_ANY,	FIELD_PSEUDO,
      TRUE,	FALSE,	"" },
};

int
TIFFInitZIP(TIFF* tif, int scheme)
{
	static const char module[] = "TIFFInitZIP";
	ZIPState* sp;

	assert( (scheme == COMPRESSION_DEFLATE)
		|| (scheme == COMPRESSION_ADOBE_DEFLATE));

	
	if (!_TIFFMergeFieldInfo(tif, zipFieldInfo,
				 TIFFArrayCount(zipFieldInfo))) {
		TIFFErrorExt(tif->tif_clientdata, module,
			     "Merging Deflate codec-specific tags failed");
		return 0;
	}

	
	tif->tif_data = (tidata_t) _TIFFmalloc(sizeof (ZIPState));
	if (tif->tif_data == NULL)
		goto bad;
	sp = ZState(tif);
	sp->stream.zalloc = NULL;
	sp->stream.zfree = NULL;
	sp->stream.opaque = NULL;
	sp->stream.data_type = Z_BINARY;

	
	sp->vgetparent = tif->tif_tagmethods.vgetfield;
	tif->tif_tagmethods.vgetfield = ZIPVGetField; 
	sp->vsetparent = tif->tif_tagmethods.vsetfield;
	tif->tif_tagmethods.vsetfield = ZIPVSetField; 

	
	sp->zipquality = Z_DEFAULT_COMPRESSION;	
	sp->state = 0;

	
	tif->tif_setupdecode = ZIPSetupDecode;
	tif->tif_predecode = ZIPPreDecode;
	tif->tif_decoderow = ZIPDecode;
	tif->tif_decodestrip = ZIPDecode;
	tif->tif_decodetile = ZIPDecode;
	tif->tif_setupencode = ZIPSetupEncode;
	tif->tif_preencode = ZIPPreEncode;
	tif->tif_postencode = ZIPPostEncode;
	tif->tif_encoderow = ZIPEncode;
	tif->tif_encodestrip = ZIPEncode;
	tif->tif_encodetile = ZIPEncode;
	tif->tif_cleanup = ZIPCleanup;
	
	(void) TIFFPredictorInit(tif);
	return (1);
bad:
	TIFFErrorExt(tif->tif_clientdata, module,
		     "No space for ZIP state block");
	return (0);
}
#endif 

