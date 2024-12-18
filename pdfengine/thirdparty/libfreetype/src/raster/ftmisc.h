

  
  
  
  
  
  

#ifndef __FTMISC_H__
#define __FTMISC_H__

  
#include FT_CONFIG_STANDARD_LIBRARY_H

#define FT_BEGIN_HEADER
#define FT_END_HEADER

#define FT_LOCAL_DEF( x )   static x

  

  typedef unsigned char  FT_Byte;
  typedef signed int     FT_Int;
  typedef unsigned int   FT_UInt;
  typedef signed long    FT_Long;
  typedef unsigned long  FT_ULong;
  typedef signed long    FT_F26Dot6;
  typedef int            FT_Error;

#define FT_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (FT_ULong)_x1 << 24 ) |     \
            ( (FT_ULong)_x2 << 16 ) |     \
            ( (FT_ULong)_x3 <<  8 ) |     \
              (FT_ULong)_x4         )

  

  typedef struct FT_MemoryRec_*  FT_Memory;

  typedef void* (*FT_Alloc_Func)( FT_Memory  memory,
                                  long       size );

  typedef void (*FT_Free_Func)( FT_Memory  memory,
                                void*      block );

  typedef void* (*FT_Realloc_Func)( FT_Memory  memory,
                                    long       cur_size,
                                    long       new_size,
                                    void*      block );

  typedef struct FT_MemoryRec_
  {
    void*            user;

    FT_Alloc_Func    alloc;
    FT_Free_Func     free;
    FT_Realloc_Func  realloc;

  } FT_MemoryRec;

  

#if ( defined _WIN32 || defined _WIN64 )

  typedef __int64  FT_Int64;

#else

#include "inttypes.h"

  typedef int64_t  FT_Int64;

#endif

  static FT_Long
  FT_MulDiv( FT_Long  a,
             FT_Long  b,
             FT_Long  c )
  {
    FT_Int   s;
    FT_Long  d;

    s = 1;
    if ( a < 0 ) { a = -a; s = -1; }
    if ( b < 0 ) { b = -b; s = -s; }
    if ( c < 0 ) { c = -c; s = -s; }

    d = (FT_Long)( c > 0 ? ( (FT_Int64)a * b + ( c >> 1 ) ) / c
                         : 0x7FFFFFFFL );

    return ( s > 0 ) ? d : -d;
  }

#endif 

