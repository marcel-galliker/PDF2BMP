

#ifndef MEMENTO_H

#include <memory.h>

#define MEMENTO_H

#ifndef MEMENTO_UNDERLYING_MALLOC
#define MEMENTO_UNDERLYING_MALLOC malloc
#endif
#ifndef MEMENTO_UNDERLYING_FREE
#define MEMENTO_UNDERLYING_FREE free
#endif
#ifndef MEMENTO_UNDERLYING_REALLOC
#define MEMENTO_UNDERLYING_REALLOC realloc
#endif
#ifndef MEMENTO_UNDERLYING_CALLOC
#define MEMENTO_UNDERLYING_CALLOC calloc
#endif

#ifndef MEMENTO_MAXALIGN
#define MEMENTO_MAXALIGN (sizeof(int))
#endif

#define MEMENTO_PREFILL   0xa6
#define MEMENTO_POSTFILL  0xa7
#define MEMENTO_ALLOCFILL 0xa8
#define MEMENTO_FREEFILL  0xa9

#define MEMENTO_FREELIST_MAX 0x2000000

int Memento_checkBlock(void *);
int Memento_checkAllMemory(void);
int Memento_check(void);

int Memento_setParanoia(int);
int Memento_paranoidAt(int);
int Memento_breakAt(int);
int Memento_getBlockNum(void *);
int Memento_find(void *a);
void Memento_breakpoint(void);
int Memento_failAt(int);
int Memento_failThisEvent(void);
void Memento_listBlocks(void);
void Memento_listNewBlocks(void);
size_t Memento_setMax(size_t);
void Memento_stats(void);
void *Memento_label(void *, const char *);

void *Memento_malloc(size_t s);
void *Memento_realloc(void *, size_t s);
void  Memento_free(void *);
void *Memento_calloc(size_t, size_t);

#ifdef MEMENTO

#ifndef COMPILING_MEMENTO_C
#define malloc  Memento_malloc
#define free    Memento_free
#define realloc Memento_realloc
#define calloc  Memento_calloc
#endif

#else

#define Memento_malloc  MEMENTO_UNDERLYING_MALLOC
#define Memento_free    MEMENTO_UNDERLYING_FREE
#define Memento_realloc MEMENTO_UNDERLYING_REALLOC
#define Memento_calloc  MEMENTO_UNDERLYING_CALLOC

#define Memento_checkBlock(A)    0
#define Memento_checkAllMemory() 0
#define Memento_check()          0
#define Memento_setParanoia(A)   0
#define Memento_paranoidAt(A)    0
#define Memento_breakAt(A)       0
#define Memento_getBlockNum(A)   0
#define Memento_find(A)          0
#define Memento_breakpoint()     do {} while (0)
#define Memento_failAt(A)        0
#define Memento_failThisEvent()  0
#define Memento_listBlocks()     do {} while (0)
#define Memento_listNewBlocks()  do {} while (0)
#define Memento_setMax(A)        0
#define Memento_stats()          do {} while (0)
#define Memento_label(A,B)       (A)

#endif 

#endif 
