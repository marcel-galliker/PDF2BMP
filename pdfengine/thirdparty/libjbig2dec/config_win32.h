

#define PACKAGE "jbig2dec"
#define VERSION "0.11"

#if defined(_MSC_VER) || (defined(__BORLANDC__) && defined(__WIN32__))
  
  typedef signed char             int8_t;
  typedef short int               int16_t;
  typedef int                     int32_t;
  typedef unsigned long long      int64_t;

  typedef unsigned char             uint8_t;
  typedef unsigned short int        uint16_t;
  typedef unsigned int              uint32_t;
  

#  if defined(_MSC_VER)
#   if _MSC_VER < 1500	
#    define vsnprintf _vsnprintf
#   endif
#  endif
#  define snprintf _snprintf

#endif 
