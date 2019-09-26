
#define HAVE_ASSERT_H 1

#define HAVE_FCNTL_H 1

#define HAVE_IEEEFP 1

#define HAVE_JBG_NEWLEN 1

#define HAVE_STRING_H 1

#define HAVE_SYS_TYPES_H 1

#define HAVE_IO_H 1

#define HAVE_SEARCH_H 0

#define HAVE_SETMODE 0

#define SIZEOF_INT 4

#define SIZEOF_LONG 4

#ifdef _MSC_VER

#define TIFF_INT64_T signed __int64

#define TIFF_UINT64_T unsigned __int64
#else

#define TIFF_INT64_T long long

#define TIFF_UINT64_T unsigned long long
#endif

#define HOST_FILLORDER FILLORDER_LSB2MSB

#ifndef __cplusplus
# ifndef inline
#  define inline __inline
# endif
#endif

#define lfind _lfind

