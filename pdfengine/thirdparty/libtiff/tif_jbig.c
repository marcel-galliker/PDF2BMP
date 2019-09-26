

#include "tiffiop.h"

#ifdef JBIG_SUPPORT
#include "jbig.h"

typedef struct
{
        uint32  recvparams;     
        char*   subaddress;     
        uint32  recvtime;       
        char*   faxdcs;         

        TIFFVGetMethod vgetparent;
        TIFFVSetMethod vsetparent;
} JBIGState;

#define GetJBIGState(tif) ((JBIGState*)(tif)->tif_data)

#define FIELD_RECVPARAMS        (FIELD_CODEC+0)
#define FIELD_SUBADDRESS        (FIELD_CODEC+1)
#define FIELD_RECVTIME          (FIELD_CODEC+2)
#define FIELD_FAXDCS            (FIELD_CODEC+3)

static const TIFFFieldInfo jbigFieldInfo[] = 
{
        {TIFFTAG_FAXRECVPARAMS,  1,  1, TIFF_LONG,  FIELD_RECVPARAMS, TRUE, FALSE, "FaxRecvParams"},
        {TIFFTAG_FAXSUBADDRESS, -1, -1, TIFF_ASCII, FIELD_SUBADDRESS, TRUE, FALSE, "FaxSubAddress"},
        {TIFFTAG_FAXRECVTIME,    1,  1, TIFF_LONG,  FIELD_RECVTIME,   TRUE, FALSE, "FaxRecvTime"},
        {TIFFTAG_FAXDCS,        -1, -1, TIFF_ASCII, FIELD_FAXDCS,     TRUE, FALSE, "FaxDcs"},
};

static int JBIGSetupDecode(TIFF* tif)
{
        if (TIFFNumberOfStrips(tif) != 1)
        {
                TIFFError("JBIG", "Multistrip images not supported in decoder");
                return 0;
        }

        return 1;
}

static int JBIGDecode(TIFF* tif, tidata_t buffer, tsize_t size, tsample_t s)
{
        struct jbg_dec_state decoder;
        int decodeStatus = 0;
        unsigned char* pImage = NULL;
	(void) size, (void) s;

        if (isFillOrder(tif, tif->tif_dir.td_fillorder))
        {
                TIFFReverseBits(tif->tif_rawdata, tif->tif_rawdatasize);
        }

        jbg_dec_init(&decoder);

#if defined(HAVE_JBG_NEWLEN)
        jbg_newlen(tif->tif_rawdata, tif->tif_rawdatasize);
        
#endif 

        decodeStatus = jbg_dec_in(&decoder, tif->tif_rawdata,
                                  tif->tif_rawdatasize, NULL);
        if (JBG_EOK != decodeStatus)
        {
		
                TIFFError("JBIG", "Error (%d) decoding: %s", decodeStatus,
#if defined(JBG_EN)
			  jbg_strerror(decodeStatus, JBG_EN)
#else
                          jbg_strerror(decodeStatus)
#endif
			 );
                return 0;
        }
        
        pImage = jbg_dec_getimage(&decoder, 0);
        _TIFFmemcpy(buffer, pImage, jbg_dec_getsize(&decoder));
        jbg_dec_free(&decoder);
        return 1;
}

static int JBIGSetupEncode(TIFF* tif)
{
        if (TIFFNumberOfStrips(tif) != 1)
        {
                TIFFError("JBIG", "Multistrip images not supported in encoder");
                return 0;
        }

        return 1;
}

static int JBIGCopyEncodedData(TIFF* tif, tidata_t pp, tsize_t cc, tsample_t s)
{
        (void) s;
        while (cc > 0) 
        {
                tsize_t n = cc;

                if (tif->tif_rawcc + n > tif->tif_rawdatasize)
                {
                        n = tif->tif_rawdatasize - tif->tif_rawcc;
                }

                assert(n > 0);
                _TIFFmemcpy(tif->tif_rawcp, pp, n);
                tif->tif_rawcp += n;
                tif->tif_rawcc += n;
                pp += n;
                cc -= n;
                if (tif->tif_rawcc >= tif->tif_rawdatasize &&
                    !TIFFFlushData1(tif))
                {
                        return (-1);
                }
        }

        return (1);
}

static void JBIGOutputBie(unsigned char* buffer, size_t len, void *userData)
{
        TIFF* tif = (TIFF*)userData;

        if (isFillOrder(tif, tif->tif_dir.td_fillorder))
        {
                TIFFReverseBits(buffer, len);
        }

        JBIGCopyEncodedData(tif, buffer, len, 0);
}

static int JBIGEncode(TIFF* tif, tidata_t buffer, tsize_t size, tsample_t s)
{
        TIFFDirectory* dir = &tif->tif_dir;
        struct jbg_enc_state encoder;

	(void) size, (void) s;

        jbg_enc_init(&encoder, 
                     dir->td_imagewidth, 
                     dir->td_imagelength, 
                     1, 
                     &buffer,
                     JBIGOutputBie,
                     tif);
        
        jbg_enc_out(&encoder);
        jbg_enc_free(&encoder);

        return 1;
}

static void JBIGCleanup(TIFF* tif)
{
        JBIGState *sp = GetJBIGState(tif);

        assert(sp != 0);

        tif->tif_tagmethods.vgetfield = sp->vgetparent;
        tif->tif_tagmethods.vsetfield = sp->vsetparent;

	_TIFFfree(tif->tif_data);
	tif->tif_data = NULL;

	_TIFFSetDefaultCompressionState(tif);
}

static void JBIGPrintDir(TIFF* tif, FILE* fd, long flags)
{
        JBIGState* codec = GetJBIGState(tif);
        (void)flags;

        if (TIFFFieldSet(tif, FIELD_RECVPARAMS))
        {
                fprintf(fd, 
                        "  Fax Receive Parameters: %08lx\n",
                        (unsigned long)codec->recvparams);
        }

        if (TIFFFieldSet(tif, FIELD_SUBADDRESS))
        {
                fprintf(fd, 
                        "  Fax SubAddress: %s\n", 
                        codec->subaddress);
        }

        if (TIFFFieldSet(tif, FIELD_RECVTIME))
        {
                fprintf(fd, 
                        "  Fax Receive Time: %lu secs\n",
                        (unsigned long)codec->recvtime);
        }

        if (TIFFFieldSet(tif, FIELD_FAXDCS))
        {
                fprintf(fd, 
                        "  Fax DCS: %s\n", 
                        codec->faxdcs);
        }
}

static int JBIGVGetField(TIFF* tif, ttag_t tag, va_list ap)
{
        JBIGState* codec = GetJBIGState(tif);

        switch (tag)
        {
                case TIFFTAG_FAXRECVPARAMS:
                        *va_arg(ap, uint32*) = codec->recvparams;
                        break;
                
                case TIFFTAG_FAXSUBADDRESS:
                        *va_arg(ap, char**) = codec->subaddress;
                        break;

                case TIFFTAG_FAXRECVTIME:
                        *va_arg(ap, uint32*) = codec->recvtime;
                        break;

                case TIFFTAG_FAXDCS:
                        *va_arg(ap, char**) = codec->faxdcs;
                        break;

                default:
                        return (*codec->vgetparent)(tif, tag, ap);
        }

        return 1;
}

static int JBIGVSetField(TIFF* tif, ttag_t tag, va_list ap)
{
        JBIGState* codec = GetJBIGState(tif);

        switch (tag)
        {
                case TIFFTAG_FAXRECVPARAMS:
                        codec->recvparams = va_arg(ap, uint32);
                        break;

                case TIFFTAG_FAXSUBADDRESS:
                        _TIFFsetString(&codec->subaddress, va_arg(ap, char*));
                        break;

                case TIFFTAG_FAXRECVTIME:
                        codec->recvtime = va_arg(ap, uint32);
                        break;

                case TIFFTAG_FAXDCS:
                        _TIFFsetString(&codec->faxdcs, va_arg(ap, char*));
                        break;

                default:
                        return (*codec->vsetparent)(tif, tag, ap);
        }

        TIFFSetFieldBit(tif, _TIFFFieldWithTag(tif, tag)->field_bit);
        tif->tif_flags |= TIFF_DIRTYDIRECT;
        return 1;
}

int TIFFInitJBIG(TIFF* tif, int scheme)
{
        JBIGState* codec = NULL;

	assert(scheme == COMPRESSION_JBIG);

	
	if (!_TIFFMergeFieldInfo(tif, jbigFieldInfo,
				 TIFFArrayCount(jbigFieldInfo))) {
		TIFFErrorExt(tif->tif_clientdata, "TIFFInitJBIG",
			     "Merging JBIG codec-specific tags failed");
		return 0;
	}

        
        tif->tif_data = (tdata_t)_TIFFmalloc(sizeof(JBIGState));
        if (tif->tif_data == NULL)
        {
                TIFFError("TIFFInitJBIG", "Not enough memory for JBIGState");
                return 0;
        }
        _TIFFmemset(tif->tif_data, 0, sizeof(JBIGState));
        codec = GetJBIGState(tif);

        
        codec->recvparams = 0;
        codec->subaddress = NULL;
        codec->faxdcs = NULL;
        codec->recvtime = 0;

	
        codec->vgetparent = tif->tif_tagmethods.vgetfield;
        codec->vsetparent = tif->tif_tagmethods.vsetfield;
        tif->tif_tagmethods.vgetfield = JBIGVGetField;
        tif->tif_tagmethods.vsetfield = JBIGVSetField;
        tif->tif_tagmethods.printdir = JBIGPrintDir;

        
        tif->tif_flags |= TIFF_NOBITREV;
        tif->tif_flags &= ~TIFF_MAPPED;

        
        tif->tif_setupdecode = JBIGSetupDecode;
        tif->tif_decodestrip = JBIGDecode;

        tif->tif_setupencode = JBIGSetupEncode;
        tif->tif_encodestrip = JBIGEncode;
        
        tif->tif_cleanup = JBIGCleanup;

        return 1;
}

#endif 

