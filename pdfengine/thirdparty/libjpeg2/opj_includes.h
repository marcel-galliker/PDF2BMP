
#ifndef OPJ_INCLUDES_H
#define OPJ_INCLUDES_H

#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "openjpeg.h"

#ifndef __GNUC__
	#define __attribute__(x) 
#endif

#ifndef INLINE
	#if defined(_MSC_VER)
		#if defined(_QT_VER)
			#define INLINE inline
		#else 
			#define INLINE __forceinline
		#endif 
	#elif defined(__GNUC__)
		#define INLINE __inline__
	#elif defined(__MWERKS__)
		#define INLINE inline
	#else 
		
		#define INLINE 
	#endif 
#endif 

#if (__STDC_VERSION__ != 199901L)
	
	#ifdef __GNUC__
		#define restrict __restrict__
	#else
		#define restrict 
	#endif
#endif

#if defined(_MSC_VER) || defined(__BORLANDC__)
#if !defined(_QT_VER)
static INLINE long lrintf(float f){
#ifdef _M_X64
    return (long)((f>0.0f) ? (f + 0.5f):(f -0.5f));
#else
    int i;
 
    _asm{
        fld f
        fistp i
    };
 
    return i;
#endif
}
#endif 
#endif

#include "j2k_lib.h"
#include "opj_malloc.h"
#include "event.h"
#include "bio.h"
#include "cio.h"

#include "image.h"
#include "j2k.h"
#include "jp2.h"
#include "jpt.h"

#include "mqc.h"
#include "raw.h"
#include "bio.h"
#include "tgt.h"
#include "pi.h"
#include "tcd.h"
#include "t1.h"
#include "dwt.h"
#include "t2.h"
#include "mct.h"
#include "int.h"
#include "fix.h"

#include "cidx_manager.h"
#include "indexbox_manager.h"

#ifdef USE_JPWL
#include "./jpwl/jpwl.h"
#endif 

#endif 
