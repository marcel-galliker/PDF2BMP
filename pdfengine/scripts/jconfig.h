

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT

#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS	
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN

#ifndef __RPCNDR_H__		
typedef unsigned char boolean;
#endif
#define HAVE_BOOLEAN		

#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED

#endif 

#ifdef JPEG_CJPEG_DJPEG

#define BMP_SUPPORTED		
#define GIF_SUPPORTED		
#define PPM_SUPPORTED		
#undef RLE_SUPPORTED		
#define TARGA_SUPPORTED		

#define TWO_FILE_COMMANDLINE	
#define USE_SETMODE		
#undef NEED_SIGNAL_CATCHER
#undef DONT_USE_B_MODE
#undef PROGRESS_REPORT		

#endif 
