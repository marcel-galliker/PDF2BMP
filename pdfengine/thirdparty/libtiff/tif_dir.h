

#ifndef _TIFFDIR_
#define	_TIFFDIR_

typedef	struct {
#define	FIELD_SETLONGS	4
	
	unsigned long	td_fieldsset[FIELD_SETLONGS];

	uint32  td_imagewidth, td_imagelength, td_imagedepth;
	uint32  td_tilewidth, td_tilelength, td_tiledepth;
	uint32  td_subfiletype;
	uint16  td_bitspersample;
	uint16  td_sampleformat;
	uint16  td_compression;
	uint16  td_photometric;
	uint16  td_threshholding;
	uint16  td_fillorder;
	uint16  td_orientation;
	uint16  td_samplesperpixel;
	uint32  td_rowsperstrip;
	uint16  td_minsamplevalue, td_maxsamplevalue;
	double  td_sminsamplevalue, td_smaxsamplevalue;
	float   td_xresolution, td_yresolution;
	uint16  td_resolutionunit;
	uint16  td_planarconfig;
	float   td_xposition, td_yposition;
	uint16  td_pagenumber[2];
	uint16* td_colormap[3];
	uint16  td_halftonehints[2];
	uint16  td_extrasamples;
	uint16* td_sampleinfo;
	
	tstrile_t td_stripsperimage;
	tstrile_t td_nstrips;            
	toff_t* td_stripoffset;
	toff_t* td_stripbytecount;	 
	int     td_stripbytecountsorted; 
	uint16  td_nsubifd;
	uint32* td_subifd;
	
	uint16  td_ycbcrsubsampling[2];
	uint16  td_ycbcrpositioning;
	
	float*	td_refblackwhite;
	uint16* td_transferfunction[3];
	
	int     td_inknameslen;
	char*   td_inknames;

	int     td_customValueCount;
        TIFFTagValue *td_customValues;
} TIFFDirectory;

 
#define	FIELD_IGNORE			0

#define	FIELD_IMAGEDIMENSIONS		1
#define FIELD_TILEDIMENSIONS		2
#define	FIELD_RESOLUTION		3
#define	FIELD_POSITION			4

#define	FIELD_SUBFILETYPE		5
#define	FIELD_BITSPERSAMPLE		6
#define	FIELD_COMPRESSION		7
#define	FIELD_PHOTOMETRIC		8
#define	FIELD_THRESHHOLDING		9
#define	FIELD_FILLORDER			10
#define	FIELD_ORIENTATION		15
#define	FIELD_SAMPLESPERPIXEL		16
#define	FIELD_ROWSPERSTRIP		17
#define	FIELD_MINSAMPLEVALUE		18
#define	FIELD_MAXSAMPLEVALUE		19
#define	FIELD_PLANARCONFIG		20
#define	FIELD_RESOLUTIONUNIT		22
#define	FIELD_PAGENUMBER		23
#define	FIELD_STRIPBYTECOUNTS		24
#define	FIELD_STRIPOFFSETS		25
#define	FIELD_COLORMAP			26
#define	FIELD_EXTRASAMPLES		31
#define FIELD_SAMPLEFORMAT		32
#define	FIELD_SMINSAMPLEVALUE		33
#define	FIELD_SMAXSAMPLEVALUE		34
#define FIELD_IMAGEDEPTH		35
#define FIELD_TILEDEPTH			36
#define	FIELD_HALFTONEHINTS		37
#define FIELD_YCBCRSUBSAMPLING		39
#define FIELD_YCBCRPOSITIONING		40
#define	FIELD_REFBLACKWHITE		41
#define	FIELD_TRANSFERFUNCTION		44
#define	FIELD_INKNAMES			46
#define	FIELD_SUBIFD			49

#define	FIELD_CODEC			66	

#define	FIELD_PSEUDO			0

#define	FIELD_LAST			(32*FIELD_SETLONGS-1)

#define	TIFFExtractData(tif, type, v) \
    ((uint32) ((tif)->tif_header.tiff_magic == TIFF_BIGENDIAN ? \
        ((v) >> (tif)->tif_typeshift[type]) & (tif)->tif_typemask[type] : \
	(v) & (tif)->tif_typemask[type]))
#define	TIFFInsertData(tif, type, v) \
    ((uint32) ((tif)->tif_header.tiff_magic == TIFF_BIGENDIAN ? \
        ((v) & (tif)->tif_typemask[type]) << (tif)->tif_typeshift[type] : \
	(v) & (tif)->tif_typemask[type]))

#define BITn(n)				(((unsigned long)1L)<<((n)&0x1f)) 
#define BITFIELDn(tif, n)		((tif)->tif_dir.td_fieldsset[(n)/32]) 
#define TIFFFieldSet(tif, field)	(BITFIELDn(tif, field) & BITn(field)) 
#define TIFFSetFieldBit(tif, field)	(BITFIELDn(tif, field) |= BITn(field))
#define TIFFClrFieldBit(tif, field)	(BITFIELDn(tif, field) &= ~BITn(field))

#define	FieldSet(fields, f)		(fields[(f)/32] & BITn(f))
#define	ResetFieldBit(fields, f)	(fields[(f)/32] &= ~BITn(f))

#if defined(__cplusplus)
extern "C" {
#endif
extern	const TIFFFieldInfo *_TIFFGetFieldInfo(size_t *);
extern	const TIFFFieldInfo *_TIFFGetExifFieldInfo(size_t *);
extern	void _TIFFSetupFieldInfo(TIFF*, const TIFFFieldInfo[], size_t);
extern	int _TIFFMergeFieldInfo(TIFF*, const TIFFFieldInfo[], int);
extern	void _TIFFPrintFieldInfo(TIFF*, FILE*);
extern	TIFFDataType _TIFFSampleToTagType(TIFF*);
extern  const TIFFFieldInfo* _TIFFFindOrRegisterFieldInfo( TIFF *tif,
							   ttag_t tag,
							   TIFFDataType dt );
extern  TIFFFieldInfo* _TIFFCreateAnonFieldInfo( TIFF *tif, ttag_t tag,
                                                 TIFFDataType dt );

#define _TIFFFindFieldInfo	    TIFFFindFieldInfo
#define _TIFFFindFieldInfoByName    TIFFFindFieldInfoByName
#define _TIFFFieldWithTag	    TIFFFieldWithTag
#define _TIFFFieldWithName	    TIFFFieldWithName

#if defined(__cplusplus)
}
#endif
#endif 

