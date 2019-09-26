
#ifndef __OPJ_MALLOC_H
#define __OPJ_MALLOC_H

#ifdef ALLOC_PERF_OPT
void * OPJ_CALLCONV opj_malloc(size_t size);
#else
#define opj_malloc(size) malloc(size)
#endif

#ifdef ALLOC_PERF_OPT
void * OPJ_CALLCONV opj_calloc(size_t _NumOfElements, size_t _SizeOfElements);
#else
#define opj_calloc(num, size) calloc(num, size)
#endif

#ifdef _WIN32
	
	#ifdef __GNUC__
		#include <mm_malloc.h>
		#define HAVE_MM_MALLOC
	#else 
		#include <malloc.h>
		#ifdef _mm_malloc
			#define HAVE_MM_MALLOC
		#endif
	#endif
#else 
	#if defined(__sun)
		#define HAVE_MEMALIGN
	
	#elif !defined(__amd64__) && !defined(__APPLE__)	
		#define HAVE_MEMALIGN
		#include <malloc.h>			
	#endif
#endif

#define opj_aligned_malloc(size) malloc(size)
#define opj_aligned_free(m) free(m)

#ifdef HAVE_MM_MALLOC
	#undef opj_aligned_malloc
	#define opj_aligned_malloc(size) _mm_malloc(size, 16)
	#undef opj_aligned_free
	#define opj_aligned_free(m) _mm_free(m)
#endif

#ifdef HAVE_MEMALIGN
	extern void* memalign(size_t, size_t);
	#undef opj_aligned_malloc
	#define opj_aligned_malloc(size) memalign(16, (size))
	#undef opj_aligned_free
	#define opj_aligned_free(m) free(m)
#endif

#ifdef HAVE_POSIX_MEMALIGN
	#undef opj_aligned_malloc
	extern int posix_memalign(void**, size_t, size_t);

	static INLINE void* __attribute__ ((malloc)) opj_aligned_malloc(size_t size){
		void* mem = NULL;
		posix_memalign(&mem, 16, size);
		return mem;
	}
	#undef opj_aligned_free
	#define opj_aligned_free(m) free(m)
#endif

#ifdef ALLOC_PERF_OPT
	#undef opj_aligned_malloc
	#define opj_aligned_malloc(size) opj_malloc(size)
	#undef opj_aligned_free
	#define opj_aligned_free(m) opj_free(m)
#endif

#ifdef ALLOC_PERF_OPT
void * OPJ_CALLCONV opj_realloc(void * m, size_t s);
#else
#define opj_realloc(m, s) realloc(m, s)
#endif

#ifdef ALLOC_PERF_OPT
void OPJ_CALLCONV opj_free(void * m);
#else
#define opj_free(m) free(m)
#endif

#ifdef __GNUC__
#pragma GCC poison malloc calloc realloc free
#endif

#endif 

