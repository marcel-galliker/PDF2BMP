

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/times.h>
#endif 
#include "opj_includes.h"

double opj_clock(void) {
#ifdef _WIN32
	
    LARGE_INTEGER freq , t ;
    
    QueryPerformanceFrequency(&freq) ;
	
    
    QueryPerformanceCounter ( & t ) ;
    return ( t.QuadPart /(double) freq.QuadPart ) ;
#else
	
    struct rusage t;
    double procTime;
    
    getrusage(0,&t);
    
	
    procTime = t.ru_utime.tv_sec + t.ru_stime.tv_sec;
    
    return ( procTime + (t.ru_utime.tv_usec + t.ru_stime.tv_usec) * 1e-6 ) ;
#endif
}

