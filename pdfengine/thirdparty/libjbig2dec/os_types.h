

#ifndef _JBIG2_OS_TYPES_H
#define _JBIG2_OS_TYPES_H

#if defined(__CYGWIN__) && !defined(HAVE_STDINT_H)
# include <sys/types.h>
# if defined(OLD_CYGWIN_SYS_TYPES)
  
   typedef u_int8_t uint8_t;
   typedef u_int16_t uint16_t;
   typedef u_int32_t uint32_t;
#endif
#elif defined(HAVE_CONFIG_H)
# include "config_types.h"
#elif defined(_WIN32) || defined(__WIN32__)
# include "config_win32.h"
#elif !defined(HAVE_STDINT_H)
   typedef unsigned char  uint8_t;
   typedef unsigned short uint16_t;
   typedef unsigned int   uint32_t;
   typedef signed char    int8_t;
   typedef signed short   int16_t;
   typedef signed int     int32_t;
#endif

#if defined(HAVE_STDINT_H) || defined(__MACOS__)
# include <stdint.h>
#elif defined(__VMS) || defined(__osf__)
# include <inttypes.h>
#endif

#ifdef __hpux
#include <sys/_inttypes.h>
#endif

#endif 
