

#define BITS_IN_JSAMPLE  8	

#define MAX_COMPONENTS  10	

#if BITS_IN_JSAMPLE == 8

#ifdef HAVE_UNSIGNED_CHAR

typedef unsigned char JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#else 

typedef char JSAMPLE;
#ifdef CHAR_IS_UNSIGNED
#define GETJSAMPLE(value)  ((int) (value))
#else
#define GETJSAMPLE(value)  ((int) (value) & 0xFF)
#endif 

#endif 

#define MAXJSAMPLE	255
#define CENTERJSAMPLE	128

#endif 

#if BITS_IN_JSAMPLE == 12

typedef short JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#define MAXJSAMPLE	4095
#define CENTERJSAMPLE	2048

#endif 

typedef short JCOEF;

#ifdef HAVE_UNSIGNED_CHAR

typedef unsigned char JOCTET;
#define GETJOCTET(value)  (value)

#else 

typedef char JOCTET;
#ifdef CHAR_IS_UNSIGNED
#define GETJOCTET(value)  (value)
#else
#define GETJOCTET(value)  ((value) & 0xFF)
#endif 

#endif 

#ifdef HAVE_UNSIGNED_CHAR
typedef unsigned char UINT8;
#else 
#ifdef CHAR_IS_UNSIGNED
typedef char UINT8;
#else 
typedef short UINT8;
#endif 
#endif 

#ifdef HAVE_UNSIGNED_SHORT
typedef unsigned short UINT16;
#else 
typedef unsigned int UINT16;
#endif 

#ifndef XMD_H			
typedef short INT16;
#endif

#ifndef XMD_H			
#ifndef _BASETSD_H_		
#ifndef _BASETSD_H		
#ifndef QGLOBAL_H		
typedef long INT32;
#endif
#endif
#endif
#endif

typedef unsigned int JDIMENSION;

#define JPEG_MAX_DIMENSION  65500L  

#define METHODDEF(type)		static type

#define LOCAL(type)		static type

#define GLOBAL(type)		type

#define EXTERN(type)		extern type

#ifdef HAVE_PROTOTYPES
#define JMETHOD(type,methodname,arglist)  type (*methodname) arglist
#else
#define JMETHOD(type,methodname,arglist)  type (*methodname) ()
#endif

#ifndef FAR
#ifdef NEED_FAR_POINTERS
#define FAR  far
#else
#define FAR
#endif
#endif

#ifndef HAVE_BOOLEAN
typedef int boolean;
#endif
#ifndef FALSE			
#define FALSE	0		
#endif
#ifndef TRUE
#define TRUE	1
#endif

#ifdef JPEG_INTERNALS
#define JPEG_INTERNAL_OPTIONS
#endif

#ifdef JPEG_INTERNAL_OPTIONS

#define DCT_ISLOW_SUPPORTED	
#define DCT_IFAST_SUPPORTED	
#define DCT_FLOAT_SUPPORTED	

#define C_ARITH_CODING_SUPPORTED    
#define C_MULTISCAN_FILES_SUPPORTED 
#define C_PROGRESSIVE_SUPPORTED	    
#define DCT_SCALING_SUPPORTED	    
#define ENTROPY_OPT_SUPPORTED	    

#define INPUT_SMOOTHING_SUPPORTED   

#define D_ARITH_CODING_SUPPORTED    
#define D_MULTISCAN_FILES_SUPPORTED 
#define D_PROGRESSIVE_SUPPORTED	    
#define IDCT_SCALING_SUPPORTED	    
#define SAVE_MARKERS_SUPPORTED	    
#define BLOCK_SMOOTHING_SUPPORTED   
#undef  UPSAMPLE_SCALING_SUPPORTED  
#define UPSAMPLE_MERGING_SUPPORTED  
#define QUANT_1PASS_SUPPORTED	    
#define QUANT_2PASS_SUPPORTED	    

#define RGB_RED		0	
#define RGB_GREEN	1	
#define RGB_BLUE	2	
#define RGB_PIXELSIZE	3	

#ifndef INLINE
#ifdef __GNUC__			
#define INLINE __inline__
#endif
#ifndef INLINE
#define INLINE			
#endif
#endif

#ifndef MULTIPLIER
#define MULTIPLIER  int		
#endif

#ifndef FAST_FLOAT
#ifdef HAVE_PROTOTYPES
#define FAST_FLOAT  float
#else
#define FAST_FLOAT  double
#endif
#endif

#endif 
