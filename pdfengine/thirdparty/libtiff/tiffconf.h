

#ifndef _TIFFCONF_
#define _TIFFCONF_

#define SIZEOF_INT 4

#define SIZEOF_LONG 4

#define TIFF_INT64_FORMAT "%I64d"

#ifdef _MSC_VER
#define TIFF_INT64_T signed __int64
#else
#define TIFF_INT64_T long long
#endif

#define TIFF_UINT64_FORMAT "%I64u"

#ifdef _MSC_VER
#define TIFF_UINT64_T unsigned __int64
#else
#define TIFF_UINT64_T unsigned long long
#endif

#define HAVE_IEEEFP 1

#define HOST_FILLORDER FILLORDER_LSB2MSB

#define HOST_BIGENDIAN 0

#define CCITT_SUPPORT 1

#define LOGLUV_SUPPORT 1

#define LZW_SUPPORT 1

#define NEXT_SUPPORT 1

#define PACKBITS_SUPPORT 1

#define THUNDER_SUPPORT 1

#define STRIPCHOP_DEFAULT TIFF_STRIPCHOP

#define SUBIFD_SUPPORT 1

#define DEFAULT_EXTRASAMPLE_AS_ALPHA 1

#define CHECK_JPEG_YCBCR_SUBSAMPLING 1

#define COLORIMETRY_SUPPORT
#define YCBCR_SUPPORT
#define CMYK_SUPPORT
#define ICC_SUPPORT
#define PHOTOSHOP_SUPPORT
#define IPTC_SUPPORT

#endif 

