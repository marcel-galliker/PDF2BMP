

#include "tiffiop.h"

#define DATATYPE_VOID		0       
#define DATATYPE_INT		1       
#define DATATYPE_UINT		2       
#define DATATYPE_IEEEFP		3       

static void
setByteArray(void** vpp, void* vp, size_t nmemb, size_t elem_size)
{
	if (*vpp)
		_TIFFfree(*vpp), *vpp = 0;
	if (vp) {
		tsize_t	bytes = nmemb * elem_size;
		if (elem_size && bytes / elem_size == nmemb)
			*vpp = (void*) _TIFFmalloc(bytes);
		if (*vpp)
			_TIFFmemcpy(*vpp, vp, bytes);
	}
}
void _TIFFsetByteArray(void** vpp, void* vp, uint32 n)
    { setByteArray(vpp, vp, n, 1); }
void _TIFFsetString(char** cpp, char* cp)
    { setByteArray((void**) cpp, (void*) cp, strlen(cp)+1, 1); }
void _TIFFsetNString(char** cpp, char* cp, uint32 n)
    { setByteArray((void**) cpp, (void*) cp, n, 1); }
void _TIFFsetShortArray(uint16** wpp, uint16* wp, uint32 n)
    { setByteArray((void**) wpp, (void*) wp, n, sizeof (uint16)); }
void _TIFFsetLongArray(uint32** lpp, uint32* lp, uint32 n)
    { setByteArray((void**) lpp, (void*) lp, n, sizeof (uint32)); }
void _TIFFsetFloatArray(float** fpp, float* fp, uint32 n)
    { setByteArray((void**) fpp, (void*) fp, n, sizeof (float)); }
void _TIFFsetDoubleArray(double** dpp, double* dp, uint32 n)
    { setByteArray((void**) dpp, (void*) dp, n, sizeof (double)); }

static int
setExtraSamples(TIFFDirectory* td, va_list ap, uint32* v)
{

#define EXTRASAMPLE_COREL_UNASSALPHA 999 

	uint16* va;
	uint32 i;

	*v = va_arg(ap, uint32);
	if ((uint16) *v > td->td_samplesperpixel)
		return 0;
	va = va_arg(ap, uint16*);
	if (*v > 0 && va == NULL)		
		return 0;
	for (i = 0; i < *v; i++) {
		if (va[i] > EXTRASAMPLE_UNASSALPHA) {
			
			if (va[i] == EXTRASAMPLE_COREL_UNASSALPHA)
				va[i] = EXTRASAMPLE_UNASSALPHA;
			else
				return 0;
		}
	}
	td->td_extrasamples = (uint16) *v;
	_TIFFsetShortArray(&td->td_sampleinfo, va, td->td_extrasamples);
	return 1;

#undef EXTRASAMPLE_COREL_UNASSALPHA
}

static uint32
checkInkNamesString(TIFF* tif, uint32 slen, const char* s)
{
	TIFFDirectory* td = &tif->tif_dir;
	uint16 i = td->td_samplesperpixel;

	if (slen > 0) {
		const char* ep = s+slen;
		const char* cp = s;
		for (; i > 0; i--) {
			for (; *cp != '\0'; cp++)
				if (cp >= ep)
					goto bad;
			cp++;				
		}
		return (cp-s);
	}
bad:
	TIFFErrorExt(tif->tif_clientdata, "TIFFSetField",
	    "%s: Invalid InkNames value; expecting %d names, found %d",
	    tif->tif_name,
	    td->td_samplesperpixel,
	    td->td_samplesperpixel-i);
	return (0);
}

static int
_TIFFVSetField(TIFF* tif, ttag_t tag, va_list ap)
{
	static const char module[] = "_TIFFVSetField";

	TIFFDirectory* td = &tif->tif_dir;
	int status = 1;
	uint32 v32, i, v;
	char* s;

	switch (tag) {
	case TIFFTAG_SUBFILETYPE:
		td->td_subfiletype = va_arg(ap, uint32);
		break;
	case TIFFTAG_IMAGEWIDTH:
		td->td_imagewidth = va_arg(ap, uint32);
		break;
	case TIFFTAG_IMAGELENGTH:
		td->td_imagelength = va_arg(ap, uint32);
		break;
	case TIFFTAG_BITSPERSAMPLE:
		td->td_bitspersample = (uint16) va_arg(ap, int);
		
		if (tif->tif_flags & TIFF_SWAB) {
			if (td->td_bitspersample == 16)
				tif->tif_postdecode = _TIFFSwab16BitData;
			else if (td->td_bitspersample == 24)
				tif->tif_postdecode = _TIFFSwab24BitData;
			else if (td->td_bitspersample == 32)
				tif->tif_postdecode = _TIFFSwab32BitData;
			else if (td->td_bitspersample == 64)
				tif->tif_postdecode = _TIFFSwab64BitData;
			else if (td->td_bitspersample == 128) 
				tif->tif_postdecode = _TIFFSwab64BitData;
		}
		break;
	case TIFFTAG_COMPRESSION:
		v = va_arg(ap, uint32) & 0xffff;
		
		if (TIFFFieldSet(tif, FIELD_COMPRESSION)) {
			if (td->td_compression == v)
				break;
			(*tif->tif_cleanup)(tif);
			tif->tif_flags &= ~TIFF_CODERSETUP;
		}
		
		if( (status = TIFFSetCompressionScheme(tif, v)) != 0 )
                    td->td_compression = (uint16) v;
                else
                    status = 0;
		break;
	case TIFFTAG_PHOTOMETRIC:
		td->td_photometric = (uint16) va_arg(ap, int);
		break;
	case TIFFTAG_THRESHHOLDING:
		td->td_threshholding = (uint16) va_arg(ap, int);
		break;
	case TIFFTAG_FILLORDER:
		v = va_arg(ap, uint32);
		if (v != FILLORDER_LSB2MSB && v != FILLORDER_MSB2LSB)
			goto badvalue;
		td->td_fillorder = (uint16) v;
		break;
	case TIFFTAG_ORIENTATION:
		v = va_arg(ap, uint32);
		if (v < ORIENTATION_TOPLEFT || ORIENTATION_LEFTBOT < v)
			goto badvalue;
		else
			td->td_orientation = (uint16) v;
		break;
	case TIFFTAG_SAMPLESPERPIXEL:
		
		v = va_arg(ap, uint32);
		if (v == 0)
			goto badvalue;
		td->td_samplesperpixel = (uint16) v;
		break;
	case TIFFTAG_ROWSPERSTRIP:
		v32 = va_arg(ap, uint32);
		if (v32 == 0)
			goto badvalue32;
		td->td_rowsperstrip = v32;
		if (!TIFFFieldSet(tif, FIELD_TILEDIMENSIONS)) {
			td->td_tilelength = v32;
			td->td_tilewidth = td->td_imagewidth;
		}
		break;
	case TIFFTAG_MINSAMPLEVALUE:
		td->td_minsamplevalue = (uint16) va_arg(ap, int);
		break;
	case TIFFTAG_MAXSAMPLEVALUE:
		td->td_maxsamplevalue = (uint16) va_arg(ap, int);
		break;
	case TIFFTAG_SMINSAMPLEVALUE:
		td->td_sminsamplevalue = va_arg(ap, double);
		break;
	case TIFFTAG_SMAXSAMPLEVALUE:
		td->td_smaxsamplevalue = va_arg(ap, double);
		break;
	case TIFFTAG_XRESOLUTION:
		td->td_xresolution = (float) va_arg(ap, double);
		break;
	case TIFFTAG_YRESOLUTION:
		td->td_yresolution = (float) va_arg(ap, double);
		break;
	case TIFFTAG_PLANARCONFIG:
		v = va_arg(ap, uint32);
		if (v != PLANARCONFIG_CONTIG && v != PLANARCONFIG_SEPARATE)
			goto badvalue;
		td->td_planarconfig = (uint16) v;
		break;
	case TIFFTAG_XPOSITION:
		td->td_xposition = (float) va_arg(ap, double);
		break;
	case TIFFTAG_YPOSITION:
		td->td_yposition = (float) va_arg(ap, double);
		break;
	case TIFFTAG_RESOLUTIONUNIT:
		v = va_arg(ap, uint32);
		if (v < RESUNIT_NONE || RESUNIT_CENTIMETER < v)
			goto badvalue;
		td->td_resolutionunit = (uint16) v;
		break;
	case TIFFTAG_PAGENUMBER:
		td->td_pagenumber[0] = (uint16) va_arg(ap, int);
		td->td_pagenumber[1] = (uint16) va_arg(ap, int);
		break;
	case TIFFTAG_HALFTONEHINTS:
		td->td_halftonehints[0] = (uint16) va_arg(ap, int);
		td->td_halftonehints[1] = (uint16) va_arg(ap, int);
		break;
	case TIFFTAG_COLORMAP:
		v32 = (uint32)(1L<<td->td_bitspersample);
		_TIFFsetShortArray(&td->td_colormap[0], va_arg(ap, uint16*), v32);
		_TIFFsetShortArray(&td->td_colormap[1], va_arg(ap, uint16*), v32);
		_TIFFsetShortArray(&td->td_colormap[2], va_arg(ap, uint16*), v32);
		break;
	case TIFFTAG_EXTRASAMPLES:
		if (!setExtraSamples(td, ap, &v))
			goto badvalue;
		break;
	case TIFFTAG_MATTEING:
		td->td_extrasamples = (uint16) (va_arg(ap, int) != 0);
		if (td->td_extrasamples) {
			uint16 sv = EXTRASAMPLE_ASSOCALPHA;
			_TIFFsetShortArray(&td->td_sampleinfo, &sv, 1);
		}
		break;
	case TIFFTAG_TILEWIDTH:
		v32 = va_arg(ap, uint32);
		if (v32 % 16) {
			if (tif->tif_mode != O_RDONLY)
				goto badvalue32;
			TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
				"Nonstandard tile width %d, convert file", v32);
		}
		td->td_tilewidth = v32;
		tif->tif_flags |= TIFF_ISTILED;
		break;
	case TIFFTAG_TILELENGTH:
		v32 = va_arg(ap, uint32);
		if (v32 % 16) {
			if (tif->tif_mode != O_RDONLY)
				goto badvalue32;
			TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
			    "Nonstandard tile length %d, convert file", v32);
		}
		td->td_tilelength = v32;
		tif->tif_flags |= TIFF_ISTILED;
		break;
	case TIFFTAG_TILEDEPTH:
		v32 = va_arg(ap, uint32);
		if (v32 == 0)
			goto badvalue32;
		td->td_tiledepth = v32;
		break;
	case TIFFTAG_DATATYPE:
		v = va_arg(ap, uint32);
		switch (v) {
		case DATATYPE_VOID:	v = SAMPLEFORMAT_VOID;	break;
		case DATATYPE_INT:	v = SAMPLEFORMAT_INT;	break;
		case DATATYPE_UINT:	v = SAMPLEFORMAT_UINT;	break;
		case DATATYPE_IEEEFP:	v = SAMPLEFORMAT_IEEEFP;break;
		default:		goto badvalue;
		}
		td->td_sampleformat = (uint16) v;
		break;
	case TIFFTAG_SAMPLEFORMAT:
		v = va_arg(ap, uint32);
		if (v < SAMPLEFORMAT_UINT || SAMPLEFORMAT_COMPLEXIEEEFP < v)
			goto badvalue;
		td->td_sampleformat = (uint16) v;

                
                if( td->td_sampleformat == SAMPLEFORMAT_COMPLEXINT 
                    && td->td_bitspersample == 32
                    && tif->tif_postdecode == _TIFFSwab32BitData )
                    tif->tif_postdecode = _TIFFSwab16BitData;
                else if( (td->td_sampleformat == SAMPLEFORMAT_COMPLEXINT 
                          || td->td_sampleformat == SAMPLEFORMAT_COMPLEXIEEEFP)
                         && td->td_bitspersample == 64
                         && tif->tif_postdecode == _TIFFSwab64BitData )
                    tif->tif_postdecode = _TIFFSwab32BitData;
		break;
	case TIFFTAG_IMAGEDEPTH:
		td->td_imagedepth = va_arg(ap, uint32);
		break;
	case TIFFTAG_SUBIFD:
		if ((tif->tif_flags & TIFF_INSUBIFD) == 0) {
			td->td_nsubifd = (uint16) va_arg(ap, int);
			_TIFFsetLongArray(&td->td_subifd, va_arg(ap, uint32*),
			    (long) td->td_nsubifd);
		} else {
			TIFFErrorExt(tif->tif_clientdata, module,
				     "%s: Sorry, cannot nest SubIFDs",
				     tif->tif_name);
			status = 0;
		}
		break;
	case TIFFTAG_YCBCRPOSITIONING:
		td->td_ycbcrpositioning = (uint16) va_arg(ap, int);
		break;
	case TIFFTAG_YCBCRSUBSAMPLING:
		td->td_ycbcrsubsampling[0] = (uint16) va_arg(ap, int);
		td->td_ycbcrsubsampling[1] = (uint16) va_arg(ap, int);
		break;
	case TIFFTAG_TRANSFERFUNCTION:
		v = (td->td_samplesperpixel - td->td_extrasamples) > 1 ? 3 : 1;
		for (i = 0; i < v; i++)
			_TIFFsetShortArray(&td->td_transferfunction[i],
			    va_arg(ap, uint16*), 1L<<td->td_bitspersample);
		break;
	case TIFFTAG_REFERENCEBLACKWHITE:
		
		_TIFFsetFloatArray(&td->td_refblackwhite, va_arg(ap, float*), 6);
		break;
	case TIFFTAG_INKNAMES:
		v = va_arg(ap, uint32);
		s = va_arg(ap, char*);
		v = checkInkNamesString(tif, v, s);
                status = v > 0;
		if( v > 0 ) {
			_TIFFsetNString(&td->td_inknames, s, v);
			td->td_inknameslen = v;
		}
		break;
        default: {
            TIFFTagValue *tv;
            int tv_size, iCustom;
	    const TIFFFieldInfo* fip = _TIFFFindFieldInfo(tif, tag, TIFF_ANY);

            
            if(fip == NULL || fip->field_bit != FIELD_CUSTOM) {
		TIFFErrorExt(tif->tif_clientdata, module,
			     "%s: Invalid %stag \"%s\" (not supported by codec)",
			     tif->tif_name, isPseudoTag(tag) ? "pseudo-" : "",
			     fip ? fip->field_name : "Unknown");
		status = 0;
		break;
            }

            
            tv = NULL;
            for (iCustom = 0; iCustom < td->td_customValueCount; iCustom++) {
		    if (td->td_customValues[iCustom].info->field_tag == tag) {
			    tv = td->td_customValues + iCustom;
			    if (tv->value != NULL) {
				    _TIFFfree(tv->value);
				    tv->value = NULL;
			    }
			    break;
		    }
            }

            
            if(tv == NULL) {
		TIFFTagValue	*new_customValues;
		
		td->td_customValueCount++;
		new_customValues = (TIFFTagValue *)
			_TIFFrealloc(td->td_customValues,
				     sizeof(TIFFTagValue) * td->td_customValueCount);
		if (!new_customValues) {
			TIFFErrorExt(tif->tif_clientdata, module,
		"%s: Failed to allocate space for list of custom values",
				  tif->tif_name);
			status = 0;
			goto end;
		}

		td->td_customValues = new_customValues;

                tv = td->td_customValues + (td->td_customValueCount - 1);
                tv->info = fip;
                tv->value = NULL;
                tv->count = 0;
            }

            
	    tv_size = _TIFFDataSize(fip->field_type);
	    if (tv_size == 0) {
		    status = 0;
		    TIFFErrorExt(tif->tif_clientdata, module,
				 "%s: Bad field type %d for \"%s\"",
				 tif->tif_name, fip->field_type,
				 fip->field_name);
		    goto end;
	    }
           
            if(fip->field_passcount) {
		    if (fip->field_writecount == TIFF_VARIABLE2)
			tv->count = (uint32) va_arg(ap, uint32);
		    else
			tv->count = (int) va_arg(ap, int);
	    } else if (fip->field_writecount == TIFF_VARIABLE
		       || fip->field_writecount == TIFF_VARIABLE2)
		tv->count = 1;
	    else if (fip->field_writecount == TIFF_SPP)
		tv->count = td->td_samplesperpixel;
	    else
                tv->count = fip->field_writecount;
            
    
	    if (fip->field_type == TIFF_ASCII)
		    _TIFFsetString((char **)&tv->value, va_arg(ap, char *));
	    else {
		tv->value = _TIFFCheckMalloc(tif, tv_size, tv->count,
					     "Tag Value");
		if (!tv->value) {
		    status = 0;
		    goto end;
		}

		if ((fip->field_passcount
		    || fip->field_writecount == TIFF_VARIABLE
		    || fip->field_writecount == TIFF_VARIABLE2
		    || fip->field_writecount == TIFF_SPP
		    || tv->count > 1)
		    && fip->field_tag != TIFFTAG_PAGENUMBER
		    && fip->field_tag != TIFFTAG_HALFTONEHINTS
		    && fip->field_tag != TIFFTAG_YCBCRSUBSAMPLING
		    && fip->field_tag != TIFFTAG_DOTRANGE) {
                    _TIFFmemcpy(tv->value, va_arg(ap, void *),
				tv->count * tv_size);
		} else {
		    
		    int i;
		    char *val = (char *)tv->value;

		    for (i = 0; i < tv->count; i++, val += tv_size) {
			    switch (fip->field_type) {
				case TIFF_BYTE:
				case TIFF_UNDEFINED:
				    {
					uint8 v = (uint8)va_arg(ap, int);
					_TIFFmemcpy(val, &v, tv_size);
				    }
				    break;
				case TIFF_SBYTE:
				    {
					int8 v = (int8)va_arg(ap, int);
					_TIFFmemcpy(val, &v, tv_size);
				    }
				    break;
				case TIFF_SHORT:
				    {
					uint16 v = (uint16)va_arg(ap, int);
					_TIFFmemcpy(val, &v, tv_size);
				    }
				    break;
				case TIFF_SSHORT:
				    {
					int16 v = (int16)va_arg(ap, int);
					_TIFFmemcpy(val, &v, tv_size);
				    }
				    break;
				case TIFF_LONG:
				case TIFF_IFD:
				    {
					uint32 v = va_arg(ap, uint32);
					_TIFFmemcpy(val, &v, tv_size);
				    }
				    break;
				case TIFF_SLONG:
				    {
					int32 v = va_arg(ap, int32);
					_TIFFmemcpy(val, &v, tv_size);
				    }
				    break;
				case TIFF_RATIONAL:
				case TIFF_SRATIONAL:
				case TIFF_FLOAT:
				    {
					float v = (float)va_arg(ap, double);
					_TIFFmemcpy(val, &v, tv_size);
				    }
				    break;
				case TIFF_DOUBLE:
				    {
					double v = va_arg(ap, double);
					_TIFFmemcpy(val, &v, tv_size);
				    }
				    break;
				default:
				    _TIFFmemset(val, 0, tv_size);
				    status = 0;
				    break;
			    }
		    }
		}
	    }
          }
	}
	if (status) {
		TIFFSetFieldBit(tif, _TIFFFieldWithTag(tif, tag)->field_bit);
		tif->tif_flags |= TIFF_DIRTYDIRECT;
	}

end:
	va_end(ap);
	return (status);
badvalue:
	TIFFErrorExt(tif->tif_clientdata, module,
		     "%s: Bad value %d for \"%s\" tag",
		     tif->tif_name, v,
		     _TIFFFieldWithTag(tif, tag)->field_name);
	va_end(ap);
	return (0);
badvalue32:
	TIFFErrorExt(tif->tif_clientdata, module,
		     "%s: Bad value %u for \"%s\" tag",
		     tif->tif_name, v32,
		     _TIFFFieldWithTag(tif, tag)->field_name);
	va_end(ap);
	return (0);
}

static int
OkToChangeTag(TIFF* tif, ttag_t tag)
{
	const TIFFFieldInfo* fip = _TIFFFindFieldInfo(tif, tag, TIFF_ANY);
	if (!fip) {			
		TIFFErrorExt(tif->tif_clientdata, "TIFFSetField", "%s: Unknown %stag %u",
		    tif->tif_name, isPseudoTag(tag) ? "pseudo-" : "", tag);
		return (0);
	}
	if (tag != TIFFTAG_IMAGELENGTH && (tif->tif_flags & TIFF_BEENWRITING) &&
	    !fip->field_oktochange) {
		
		TIFFErrorExt(tif->tif_clientdata, "TIFFSetField",
		    "%s: Cannot modify tag \"%s\" while writing",
		    tif->tif_name, fip->field_name);
		return (0);
	}
	return (1);
}

int
TIFFSetField(TIFF* tif, ttag_t tag, ...)
{
	va_list ap;
	int status;

	va_start(ap, tag);
	status = TIFFVSetField(tif, tag, ap);
	va_end(ap);
	return (status);
}

int
TIFFVSetField(TIFF* tif, ttag_t tag, va_list ap)
{
	return OkToChangeTag(tif, tag) ?
	    (*tif->tif_tagmethods.vsetfield)(tif, tag, ap) : 0;
}

static int
_TIFFVGetField(TIFF* tif, ttag_t tag, va_list ap)
{
    TIFFDirectory* td = &tif->tif_dir;
    int            ret_val = 1;

    switch (tag) {
	case TIFFTAG_SUBFILETYPE:
            *va_arg(ap, uint32*) = td->td_subfiletype;
            break;
	case TIFFTAG_IMAGEWIDTH:
            *va_arg(ap, uint32*) = td->td_imagewidth;
            break;
	case TIFFTAG_IMAGELENGTH:
            *va_arg(ap, uint32*) = td->td_imagelength;
            break;
	case TIFFTAG_BITSPERSAMPLE:
            *va_arg(ap, uint16*) = td->td_bitspersample;
            break;
	case TIFFTAG_COMPRESSION:
            *va_arg(ap, uint16*) = td->td_compression;
            break;
	case TIFFTAG_PHOTOMETRIC:
            *va_arg(ap, uint16*) = td->td_photometric;
            break;
	case TIFFTAG_THRESHHOLDING:
            *va_arg(ap, uint16*) = td->td_threshholding;
            break;
	case TIFFTAG_FILLORDER:
            *va_arg(ap, uint16*) = td->td_fillorder;
            break;
	case TIFFTAG_ORIENTATION:
            *va_arg(ap, uint16*) = td->td_orientation;
            break;
	case TIFFTAG_SAMPLESPERPIXEL:
            *va_arg(ap, uint16*) = td->td_samplesperpixel;
            break;
	case TIFFTAG_ROWSPERSTRIP:
            *va_arg(ap, uint32*) = td->td_rowsperstrip;
            break;
	case TIFFTAG_MINSAMPLEVALUE:
            *va_arg(ap, uint16*) = td->td_minsamplevalue;
            break;
	case TIFFTAG_MAXSAMPLEVALUE:
            *va_arg(ap, uint16*) = td->td_maxsamplevalue;
            break;
	case TIFFTAG_SMINSAMPLEVALUE:
            *va_arg(ap, double*) = td->td_sminsamplevalue;
            break;
	case TIFFTAG_SMAXSAMPLEVALUE:
            *va_arg(ap, double*) = td->td_smaxsamplevalue;
            break;
	case TIFFTAG_XRESOLUTION:
            *va_arg(ap, float*) = td->td_xresolution;
            break;
	case TIFFTAG_YRESOLUTION:
            *va_arg(ap, float*) = td->td_yresolution;
            break;
	case TIFFTAG_PLANARCONFIG:
            *va_arg(ap, uint16*) = td->td_planarconfig;
            break;
	case TIFFTAG_XPOSITION:
            *va_arg(ap, float*) = td->td_xposition;
            break;
	case TIFFTAG_YPOSITION:
            *va_arg(ap, float*) = td->td_yposition;
            break;
	case TIFFTAG_RESOLUTIONUNIT:
            *va_arg(ap, uint16*) = td->td_resolutionunit;
            break;
	case TIFFTAG_PAGENUMBER:
            *va_arg(ap, uint16*) = td->td_pagenumber[0];
            *va_arg(ap, uint16*) = td->td_pagenumber[1];
            break;
	case TIFFTAG_HALFTONEHINTS:
            *va_arg(ap, uint16*) = td->td_halftonehints[0];
            *va_arg(ap, uint16*) = td->td_halftonehints[1];
            break;
	case TIFFTAG_COLORMAP:
            *va_arg(ap, uint16**) = td->td_colormap[0];
            *va_arg(ap, uint16**) = td->td_colormap[1];
            *va_arg(ap, uint16**) = td->td_colormap[2];
            break;
	case TIFFTAG_STRIPOFFSETS:
	case TIFFTAG_TILEOFFSETS:
            *va_arg(ap, uint32**) = td->td_stripoffset;
            break;
	case TIFFTAG_STRIPBYTECOUNTS:
	case TIFFTAG_TILEBYTECOUNTS:
            *va_arg(ap, uint32**) = td->td_stripbytecount;
            break;
	case TIFFTAG_MATTEING:
            *va_arg(ap, uint16*) =
                (td->td_extrasamples == 1 &&
                 td->td_sampleinfo[0] == EXTRASAMPLE_ASSOCALPHA);
            break;
	case TIFFTAG_EXTRASAMPLES:
            *va_arg(ap, uint16*) = td->td_extrasamples;
            *va_arg(ap, uint16**) = td->td_sampleinfo;
            break;
	case TIFFTAG_TILEWIDTH:
            *va_arg(ap, uint32*) = td->td_tilewidth;
            break;
	case TIFFTAG_TILELENGTH:
            *va_arg(ap, uint32*) = td->td_tilelength;
            break;
	case TIFFTAG_TILEDEPTH:
            *va_arg(ap, uint32*) = td->td_tiledepth;
            break;
	case TIFFTAG_DATATYPE:
            switch (td->td_sampleformat) {
		case SAMPLEFORMAT_UINT:
                    *va_arg(ap, uint16*) = DATATYPE_UINT;
                    break;
		case SAMPLEFORMAT_INT:
                    *va_arg(ap, uint16*) = DATATYPE_INT;
                    break;
		case SAMPLEFORMAT_IEEEFP:
                    *va_arg(ap, uint16*) = DATATYPE_IEEEFP;
                    break;
		case SAMPLEFORMAT_VOID:
                    *va_arg(ap, uint16*) = DATATYPE_VOID;
                    break;
            }
            break;
	case TIFFTAG_SAMPLEFORMAT:
            *va_arg(ap, uint16*) = td->td_sampleformat;
            break;
	case TIFFTAG_IMAGEDEPTH:
            *va_arg(ap, uint32*) = td->td_imagedepth;
            break;
	case TIFFTAG_SUBIFD:
            *va_arg(ap, uint16*) = td->td_nsubifd;
            *va_arg(ap, uint32**) = td->td_subifd;
            break;
	case TIFFTAG_YCBCRPOSITIONING:
            *va_arg(ap, uint16*) = td->td_ycbcrpositioning;
            break;
	case TIFFTAG_YCBCRSUBSAMPLING:
            *va_arg(ap, uint16*) = td->td_ycbcrsubsampling[0];
            *va_arg(ap, uint16*) = td->td_ycbcrsubsampling[1];
            break;
	case TIFFTAG_TRANSFERFUNCTION:
            *va_arg(ap, uint16**) = td->td_transferfunction[0];
            if (td->td_samplesperpixel - td->td_extrasamples > 1) {
                *va_arg(ap, uint16**) = td->td_transferfunction[1];
                *va_arg(ap, uint16**) = td->td_transferfunction[2];
            }
            break;
	case TIFFTAG_REFERENCEBLACKWHITE:
	    *va_arg(ap, float**) = td->td_refblackwhite;
	    break;
	case TIFFTAG_INKNAMES:
            *va_arg(ap, char**) = td->td_inknames;
            break;
        default:
        {
            const TIFFFieldInfo* fip = _TIFFFindFieldInfo(tif, tag, TIFF_ANY);
            int           i;
            
            
            if( fip == NULL || fip->field_bit != FIELD_CUSTOM )
            {
		    TIFFErrorExt(tif->tif_clientdata, "_TIFFVGetField",
				 "%s: Invalid %stag \"%s\" "
				 "(not supported by codec)",
				 tif->tif_name,
				 isPseudoTag(tag) ? "pseudo-" : "",
				 fip ? fip->field_name : "Unknown");
		    ret_val = 0;
		    break;
            }

            
            ret_val = 0;
            for (i = 0; i < td->td_customValueCount; i++) {
		TIFFTagValue *tv = td->td_customValues + i;

		if (tv->info->field_tag != tag)
			continue;
                
		if (fip->field_passcount) {
			if (fip->field_readcount == TIFF_VARIABLE2) 
				*va_arg(ap, uint32*) = (uint32)tv->count;
			else	
				*va_arg(ap, uint16*) = (uint16)tv->count;
			*va_arg(ap, void **) = tv->value;
			ret_val = 1;
                } else {
			if ((fip->field_type == TIFF_ASCII
			    || fip->field_readcount == TIFF_VARIABLE
			    || fip->field_readcount == TIFF_VARIABLE2
			    || fip->field_readcount == TIFF_SPP
			    || tv->count > 1)
			    && fip->field_tag != TIFFTAG_PAGENUMBER
			    && fip->field_tag != TIFFTAG_HALFTONEHINTS
			    && fip->field_tag != TIFFTAG_YCBCRSUBSAMPLING
			    && fip->field_tag != TIFFTAG_DOTRANGE) {
				*va_arg(ap, void **) = tv->value;
				ret_val = 1;
			} else {
			    int j;
			    char *val = (char *)tv->value;

			    for (j = 0; j < tv->count;
				 j++, val += _TIFFDataSize(tv->info->field_type)) {
				switch (fip->field_type) {
					case TIFF_BYTE:
					case TIFF_UNDEFINED:
						*va_arg(ap, uint8*) =
							*(uint8 *)val;
						ret_val = 1;
						break;
					case TIFF_SBYTE:
						*va_arg(ap, int8*) =
							*(int8 *)val;
						ret_val = 1;
						break;
					case TIFF_SHORT:
						*va_arg(ap, uint16*) =
							*(uint16 *)val;
						ret_val = 1;
						break;
					case TIFF_SSHORT:
						*va_arg(ap, int16*) =
							*(int16 *)val;
						ret_val = 1;
						break;
					case TIFF_LONG:
					case TIFF_IFD:
						*va_arg(ap, uint32*) =
							*(uint32 *)val;
						ret_val = 1;
						break;
					case TIFF_SLONG:
						*va_arg(ap, int32*) =
							*(int32 *)val;
						ret_val = 1;
						break;
					case TIFF_RATIONAL:
					case TIFF_SRATIONAL:
					case TIFF_FLOAT:
						*va_arg(ap, float*) =
							*(float *)val;
						ret_val = 1;
						break;
					case TIFF_DOUBLE:
						*va_arg(ap, double*) =
							*(double *)val;
						ret_val = 1;
						break;
					default:
						ret_val = 0;
						break;
				}
			    }
			}
                }
		break;
            }
        }
    }
    return(ret_val);
}

int
TIFFGetField(TIFF* tif, ttag_t tag, ...)
{
	int status;
	va_list ap;

	va_start(ap, tag);
	status = TIFFVGetField(tif, tag, ap);
	va_end(ap);
	return (status);
}

int
TIFFVGetField(TIFF* tif, ttag_t tag, va_list ap)
{
	const TIFFFieldInfo* fip = _TIFFFindFieldInfo(tif, tag, TIFF_ANY);
	return (fip && (isPseudoTag(tag) || TIFFFieldSet(tif, fip->field_bit)) ?
	    (*tif->tif_tagmethods.vgetfield)(tif, tag, ap) : 0);
}

#define	CleanupField(member) {		\
    if (td->member) {			\
	_TIFFfree(td->member);		\
	td->member = 0;			\
    }					\
}

void
TIFFFreeDirectory(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;
	int            i;

	_TIFFmemset(td->td_fieldsset, 0, FIELD_SETLONGS);
	CleanupField(td_colormap[0]);
	CleanupField(td_colormap[1]);
	CleanupField(td_colormap[2]);
	CleanupField(td_sampleinfo);
	CleanupField(td_subifd);
	CleanupField(td_inknames);
	CleanupField(td_refblackwhite);
	CleanupField(td_transferfunction[0]);
	CleanupField(td_transferfunction[1]);
	CleanupField(td_transferfunction[2]);
	CleanupField(td_stripoffset);
	CleanupField(td_stripbytecount);
	TIFFClrFieldBit(tif, FIELD_YCBCRSUBSAMPLING);
	TIFFClrFieldBit(tif, FIELD_YCBCRPOSITIONING);

	
	for( i = 0; i < td->td_customValueCount; i++ ) {
		if (td->td_customValues[i].value)
			_TIFFfree(td->td_customValues[i].value);
	}

	td->td_customValueCount = 0;
	CleanupField(td_customValues);
}
#undef CleanupField

static TIFFExtendProc _TIFFextender = (TIFFExtendProc) NULL;

TIFFExtendProc
TIFFSetTagExtender(TIFFExtendProc extender)
{
	TIFFExtendProc prev = _TIFFextender;
	_TIFFextender = extender;
	return (prev);
}

int
TIFFCreateDirectory(TIFF* tif)
{
    TIFFDefaultDirectory(tif);
    tif->tif_diroff = 0;
    tif->tif_nextdiroff = 0;
    tif->tif_curoff = 0;
    tif->tif_row = (uint32) -1;
    tif->tif_curstrip = (tstrip_t) -1;

    return 0;
}

int
TIFFDefaultDirectory(TIFF* tif)
{
	register TIFFDirectory* td = &tif->tif_dir;

	size_t tiffFieldInfoCount;
	const TIFFFieldInfo *tiffFieldInfo =
	    _TIFFGetFieldInfo(&tiffFieldInfoCount);
	_TIFFSetupFieldInfo(tif, tiffFieldInfo, tiffFieldInfoCount);

	_TIFFmemset(td, 0, sizeof (*td));
	td->td_fillorder = FILLORDER_MSB2LSB;
	td->td_bitspersample = 1;
	td->td_threshholding = THRESHHOLD_BILEVEL;
	td->td_orientation = ORIENTATION_TOPLEFT;
	td->td_samplesperpixel = 1;
	td->td_rowsperstrip = (uint32) -1;
	td->td_tilewidth = 0;
	td->td_tilelength = 0;
	td->td_tiledepth = 1;
	td->td_stripbytecountsorted = 1; 
	td->td_resolutionunit = RESUNIT_INCH;
	td->td_sampleformat = SAMPLEFORMAT_UINT;
	td->td_imagedepth = 1;
	td->td_ycbcrsubsampling[0] = 2;
	td->td_ycbcrsubsampling[1] = 2;
	td->td_ycbcrpositioning = YCBCRPOSITION_CENTERED;
	tif->tif_postdecode = _TIFFNoPostDecode;
	tif->tif_foundfield = NULL;
	tif->tif_tagmethods.vsetfield = _TIFFVSetField;
	tif->tif_tagmethods.vgetfield = _TIFFVGetField;
	tif->tif_tagmethods.printdir = NULL;
	
	if (_TIFFextender)
		(*_TIFFextender)(tif);
	(void) TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	
	tif->tif_flags &= ~TIFF_DIRTYDIRECT;

	
	tif->tif_flags &= ~TIFF_ISTILED;
        
        tif->tif_tilesize = -1;
        tif->tif_scanlinesize = -1;

	return (1);
}

static int
TIFFAdvanceDirectory(TIFF* tif, uint32* nextdir, toff_t* off)
{
	static const char module[] = "TIFFAdvanceDirectory";
	uint16 dircount;
	if (isMapped(tif))
	{
		toff_t poff=*nextdir;
		if (poff+sizeof(uint16) > tif->tif_size)
		{
			TIFFErrorExt(tif->tif_clientdata, module, "%s: Error fetching directory count",
			    tif->tif_name);
			return (0);
		}
		_TIFFmemcpy(&dircount, tif->tif_base+poff, sizeof (uint16));
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&dircount);
		poff+=sizeof (uint16)+dircount*sizeof (TIFFDirEntry);
		if (off != NULL)
			*off = poff;
		if (((toff_t) (poff+sizeof (uint32))) > tif->tif_size)
		{
			TIFFErrorExt(tif->tif_clientdata, module, "%s: Error fetching directory link",
			    tif->tif_name);
			return (0);
		}
		_TIFFmemcpy(nextdir, tif->tif_base+poff, sizeof (uint32));
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong(nextdir);
		return (1);
	}
	else
	{
		if (!SeekOK(tif, *nextdir) ||
		    !ReadOK(tif, &dircount, sizeof (uint16))) {
			TIFFErrorExt(tif->tif_clientdata, module, "%s: Error fetching directory count",
			    tif->tif_name);
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&dircount);
		if (off != NULL)
			*off = TIFFSeekFile(tif,
			    dircount*sizeof (TIFFDirEntry), SEEK_CUR);
		else
			(void) TIFFSeekFile(tif,
			    dircount*sizeof (TIFFDirEntry), SEEK_CUR);
		if (!ReadOK(tif, nextdir, sizeof (uint32))) {
			TIFFErrorExt(tif->tif_clientdata, module, "%s: Error fetching directory link",
			    tif->tif_name);
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong(nextdir);
		return (1);
	}
}

tdir_t
TIFFNumberOfDirectories(TIFF* tif)
{
    toff_t nextdir = tif->tif_header.tiff_diroff;
    tdir_t n = 0;
    
    while (nextdir != 0 && TIFFAdvanceDirectory(tif, &nextdir, NULL))
        n++;
    return (n);
}

int
TIFFSetDirectory(TIFF* tif, tdir_t dirn)
{
	toff_t nextdir;
	tdir_t n;

	nextdir = tif->tif_header.tiff_diroff;
	for (n = dirn; n > 0 && nextdir != 0; n--)
		if (!TIFFAdvanceDirectory(tif, &nextdir, NULL))
			return (0);
	tif->tif_nextdiroff = nextdir;
	
	tif->tif_curdir = (dirn - n) - 1;
	
	tif->tif_dirnumber = 0;
	return (TIFFReadDirectory(tif));
}

int
TIFFSetSubDirectory(TIFF* tif, uint32 diroff)
{
	tif->tif_nextdiroff = diroff;
	
	tif->tif_dirnumber = 0;
	return (TIFFReadDirectory(tif));
}

uint32
TIFFCurrentDirOffset(TIFF* tif)
{
	return (tif->tif_diroff);
}

int
TIFFLastDirectory(TIFF* tif)
{
	return (tif->tif_nextdiroff == 0);
}

int
TIFFUnlinkDirectory(TIFF* tif, tdir_t dirn)
{
	static const char module[] = "TIFFUnlinkDirectory";
	toff_t nextdir;
	toff_t off;
	tdir_t n;

	if (tif->tif_mode == O_RDONLY) {
		TIFFErrorExt(tif->tif_clientdata, module,
                             "Can not unlink directory in read-only file");
		return (0);
	}
	
	nextdir = tif->tif_header.tiff_diroff;
	off = sizeof (uint16) + sizeof (uint16);
	for (n = dirn-1; n > 0; n--) {
		if (nextdir == 0) {
			TIFFErrorExt(tif->tif_clientdata, module, "Directory %d does not exist", dirn);
			return (0);
		}
		if (!TIFFAdvanceDirectory(tif, &nextdir, &off))
			return (0);
	}
	
	if (!TIFFAdvanceDirectory(tif, &nextdir, NULL))
		return (0);
	
	(void) TIFFSeekFile(tif, off, SEEK_SET);
	if (tif->tif_flags & TIFF_SWAB)
		TIFFSwabLong(&nextdir);
	if (!WriteOK(tif, &nextdir, sizeof (uint32))) {
		TIFFErrorExt(tif->tif_clientdata, module, "Error writing directory link");
		return (0);
	}
	
	(*tif->tif_cleanup)(tif);
	if ((tif->tif_flags & TIFF_MYBUFFER) && tif->tif_rawdata) {
		_TIFFfree(tif->tif_rawdata);
		tif->tif_rawdata = NULL;
		tif->tif_rawcc = 0;
	}
	tif->tif_flags &= ~(TIFF_BEENWRITING|TIFF_BUFFERSETUP|TIFF_POSTENCODE);
	TIFFFreeDirectory(tif);
	TIFFDefaultDirectory(tif);
	tif->tif_diroff = 0;			
	tif->tif_nextdiroff = 0;		
	tif->tif_curoff = 0;
	tif->tif_row = (uint32) -1;
	tif->tif_curstrip = (tstrip_t) -1;
	return (1);
}

int
TIFFReassignTagToIgnore (enum TIFFIgnoreSense task, int TIFFtagID)
{
    static int TIFFignoretags [FIELD_LAST];
    static int tagcount = 0 ;
    int		i;					
    int		j;					

    switch (task)
    {
      case TIS_STORE:
        if ( tagcount < (FIELD_LAST - 1) )
        {
            for ( j = 0 ; j < tagcount ; ++j )
            {					
                if ( TIFFignoretags [j] == TIFFtagID )
                    return (TRUE) ;
            }
            TIFFignoretags [tagcount++] = TIFFtagID ;
            return (TRUE) ;
        }
        break ;
        
      case TIS_EXTRACT:
        for ( i = 0 ; i < tagcount ; ++i )
        {
            if ( TIFFignoretags [i] == TIFFtagID )
                return (TRUE) ;
        }
        break;
        
      case TIS_EMPTY:
        tagcount = 0 ;			
        return (TRUE) ;
        
      default:
        break;
    }
    
    return (FALSE);
}

