

#undef MEMENTO_LEAKONLY

#ifndef MEMENTO_STACKTRACE_METHOD
#ifdef __GNUC__
#define MEMENTO_STACKTRACE_METHOD 1
#endif
#endif

#define MEMENTO_FREELIST_MAX_SINGLE_BLOCK (MEMENTO_FREELIST_MAX/4)

#define COMPILING_MEMENTO_C
#include "pdf_memento.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(__linux__)
#define MEMENTO_HAS_FORK
#elif defined(__APPLE__) && defined(__MACH__)
#define MEMENTO_HAS_FORK
#endif

void *MEMENTO_UNDERLYING_MALLOC(size_t);
void MEMENTO_UNDERLYING_FREE(void *);
void *MEMENTO_UNDERLYING_REALLOC(void *,size_t);
void *MEMENTO_UNDERLYING_CALLOC(size_t,size_t);

int atoi(const char *);
char *getenv(const char *);

#define MEMENTO_PTRSEARCH 65536

#ifndef MEMENTO_MAXPATTERN
#define MEMENTO_MAXPATTERN 0
#endif

#ifdef MEMENTO

#ifdef HAVE_VALGRIND
#include "valgrind/memcheck.h"
#else
#define VALGRIND_MAKE_MEM_NOACCESS(p,s)  do { } while (0==1)
#define VALGRIND_MAKE_MEM_UNDEFINED(p,s)  do { } while (0==1)
#define VALGRIND_MAKE_MEM_DEFINED(p,s)  do { } while (0==1)
#endif

enum {
    Memento_PreSize  = 16,
    Memento_PostSize = 16
};

enum {
    Memento_Flag_OldBlock = 1,
    Memento_Flag_HasParent = 2
};

typedef struct Memento_BlkHeader Memento_BlkHeader;

struct Memento_BlkHeader
{
    size_t             rawsize;
    int                sequence;
    int                lastCheckedOK;
    int                flags;
    Memento_BlkHeader *next;

    const char        *label;

    
    Memento_BlkHeader *child;
    Memento_BlkHeader *sibling;

    char               preblk[Memento_PreSize];
};

typedef struct Memento_Blocks
{
    Memento_BlkHeader  *head;
    Memento_BlkHeader **tail;
} Memento_Blocks;

static struct {
    int            inited;
    Memento_Blocks used;
    Memento_Blocks free;
    size_t         freeListSize;
    int            sequence;
    int            paranoia;
    int            paranoidAt;
    int            countdown;
    int            lastChecked;
    int            breakAt;
    int            failAt;
    int            failing;
    int            nextFailAt;
    int            squeezeAt;
    int            squeezing;
    int            segv;
    int            pattern;
    int            nextPattern;
    int            patternBit;
    size_t         maxMemory;
    size_t         alloc;
    size_t         peakAlloc;
    size_t         totalAlloc;
    size_t         numMallocs;
    size_t         numFrees;
    size_t         numReallocs;
} globals;

#define MEMENTO_EXTRASIZE (sizeof(Memento_BlkHeader) + Memento_PostSize)

#define MEMENTO_ROUNDUP(S,N) ((S + N-1)&~(N-1))

#define MEMBLK_SIZE(s) MEMENTO_ROUNDUP(s + MEMENTO_EXTRASIZE, MEMENTO_MAXALIGN)

#define MEMBLK_FROMBLK(B)   (&((Memento_BlkHeader*)(void *)(B))[-1])
#define MEMBLK_TOBLK(B)     ((void*)(&((Memento_BlkHeader*)(void*)(B))[1]))
#define MEMBLK_POSTPTR(B) \
          (&((char *)(void *)(B))[(B)->rawsize + sizeof(Memento_BlkHeader)])

void Memento_breakpoint(void)
{
    
#if 0 
#ifdef DEBUG
#ifdef _MSC_VER
    __asm int 3;
#endif
#endif
#endif
}

static void Memento_addBlockHead(Memento_Blocks    *blks,
                                 Memento_BlkHeader *b,
                                 int                type)
{
    if (blks->tail == &blks->head) {
        
        blks->tail = &b->next;
    }
    b->next    = blks->head;
    blks->head = b;
#ifndef MEMENTO_LEAKONLY
    memset(b->preblk, MEMENTO_PREFILL, Memento_PreSize);
    memset(MEMBLK_POSTPTR(b), MEMENTO_POSTFILL, Memento_PostSize);
#endif
    VALGRIND_MAKE_MEM_NOACCESS(MEMBLK_POSTPTR(b), Memento_PostSize);
    if (type == 0) { 
        VALGRIND_MAKE_MEM_UNDEFINED(MEMBLK_TOBLK(b), b->rawsize);
    } else if (type == 1) { 
        VALGRIND_MAKE_MEM_NOACCESS(MEMBLK_TOBLK(b), b->rawsize);
    }
    VALGRIND_MAKE_MEM_NOACCESS(b, sizeof(Memento_BlkHeader));
}

static void Memento_addBlockTail(Memento_Blocks    *blks,
                                 Memento_BlkHeader *b,
                                 int                type)
{
    VALGRIND_MAKE_MEM_DEFINED(blks->tail, sizeof(Memento_BlkHeader *));
    *blks->tail = b;
    blks->tail  = &b->next;
    b->next = NULL;
    VALGRIND_MAKE_MEM_NOACCESS(blks->tail, sizeof(Memento_BlkHeader *));
#ifndef MEMENTO_LEAKONLY
    memset(b->preblk, MEMENTO_PREFILL, Memento_PreSize);
    memset(MEMBLK_POSTPTR(b), MEMENTO_POSTFILL, Memento_PostSize);
#endif
    VALGRIND_MAKE_MEM_NOACCESS(MEMBLK_POSTPTR(b), Memento_PostSize);
    if (type == 0) { 
        VALGRIND_MAKE_MEM_UNDEFINED(MEMBLK_TOBLK(b), b->rawsize);
    } else if (type == 1) { 
        VALGRIND_MAKE_MEM_NOACCESS(MEMBLK_TOBLK(b), b->rawsize);
    }
    VALGRIND_MAKE_MEM_NOACCESS(b, sizeof(Memento_BlkHeader));
}

typedef struct BlkCheckData {
    int found;
    int preCorrupt;
    int postCorrupt;
    int freeCorrupt;
    int index;
} BlkCheckData;

static int Memento_Internal_checkAllocedBlock(Memento_BlkHeader *b, void *arg)
{
#ifndef MEMENTO_LEAKONLY
    int           i;
    char         *p;
    int           corrupt = 0;
    BlkCheckData *data = (BlkCheckData *)arg;

    p = b->preblk;
    i = Memento_PreSize;
    do {
        corrupt |= (*p++ ^ (char)MEMENTO_PREFILL);
    } while (--i);
    if (corrupt) {
        data->preCorrupt = 1;
    }
    p = MEMBLK_POSTPTR(b);
    i = Memento_PreSize;
    do {
        corrupt |= (*p++ ^ (char)MEMENTO_POSTFILL);
    } while (--i);
    if (corrupt) {
        data->postCorrupt = 1;
    }
    if ((data->freeCorrupt | data->preCorrupt | data->postCorrupt) == 0) {
        b->lastCheckedOK = globals.sequence;
    }
    data->found |= 1;
#endif
    return 0;
}

static int Memento_Internal_checkFreedBlock(Memento_BlkHeader *b, void *arg)
{
#ifndef MEMENTO_LEAKONLY
    int           i;
    char         *p;
    BlkCheckData *data = (BlkCheckData *)arg;

    p = MEMBLK_TOBLK(b);
    i = b->rawsize;
    
    do {
        if (((size_t)p) & 1) {
            if (*p++ != (char)MEMENTO_FREEFILL)
                break;
            i--;
            if (i == 0)
                break;
        }
        if ((i >= 2) && (((size_t)p) & 2)) {
            if (*(short *)p != (short)(MEMENTO_FREEFILL | (MEMENTO_FREEFILL<<8)))
                goto mismatch;
            p += 2;
            i -= 2;
            if (i == 0)
                break;
        }
        i -= 4;
        while (i >= 0) {
            if (*(int *)p != (MEMENTO_FREEFILL |
                              (MEMENTO_FREEFILL<<8) |
                              (MEMENTO_FREEFILL<<16) |
                              (MEMENTO_FREEFILL<<24)))
                goto mismatch;
            p += 4;
            i -= 4;
        }
        i += 4;
        if ((i >= 2) && (((size_t)p) & 2)) {
            if (*(short *)p != (short)(MEMENTO_FREEFILL | (MEMENTO_FREEFILL<<8)))
                goto mismatch;
            p += 2;
            i -= 2;
        }
mismatch:
        while (i) {
            if (*p++ != (char)MEMENTO_FREEFILL)
                break;
            i--;
        }
    } while (0);
    if (i) {
        data->freeCorrupt = 1;
        data->index       = b->rawsize-i;
    }
    return Memento_Internal_checkAllocedBlock(b, arg);
#else
    return 0;
#endif
}

static void Memento_removeBlock(Memento_Blocks    *blks,
                                Memento_BlkHeader *b)
{
    Memento_BlkHeader *head = blks->head;
    Memento_BlkHeader *prev = NULL;
    while ((head) && (head != b)) {
        VALGRIND_MAKE_MEM_DEFINED(head, sizeof(*head));
        prev = head;
        head = head->next;
        VALGRIND_MAKE_MEM_NOACCESS(prev, sizeof(*prev));
    }
    if (head == NULL) {
        
        return;
    }
    if (*blks->tail == head) {
        
        if (prev == NULL) {
            
            blks->tail = &blks->head;
        } else {
            
            blks->tail = &prev->next;
        }
    }
    if (prev == NULL) {
        
        VALGRIND_MAKE_MEM_DEFINED(head, sizeof(*head));
        blks->head = head->next;
        VALGRIND_MAKE_MEM_NOACCESS(head, sizeof(*head));
    } else {
        
        VALGRIND_MAKE_MEM_DEFINED(head, sizeof(*head));
        VALGRIND_MAKE_MEM_DEFINED(prev, sizeof(*prev));
        prev->next = head->next;
        VALGRIND_MAKE_MEM_NOACCESS(head, sizeof(*head));
        VALGRIND_MAKE_MEM_NOACCESS(prev, sizeof(*prev));
    }
}

static int Memento_Internal_makeSpace(size_t space)
{
    
    if (space > MEMENTO_FREELIST_MAX_SINGLE_BLOCK)
        return 0;
    
    globals.freeListSize += space;
    
    while (globals.freeListSize > MEMENTO_FREELIST_MAX) {
        Memento_BlkHeader *head = globals.free.head;
        VALGRIND_MAKE_MEM_DEFINED(head, sizeof(*head));
        globals.free.head = head->next;
        globals.freeListSize -= MEMBLK_SIZE(head->rawsize);
        MEMENTO_UNDERLYING_FREE(head);
    }
    
    
    if (globals.free.head == NULL)
        globals.free.tail = &globals.free.head;
    return 1;
}

static int Memento_appBlocks(Memento_Blocks *blks,
                             int             (*app)(Memento_BlkHeader *,
                                                    void *),
                             void           *arg)
{
    Memento_BlkHeader *head = blks->head;
    Memento_BlkHeader *next;
    int                result;
    while (head) {
        VALGRIND_MAKE_MEM_DEFINED(head, sizeof(Memento_BlkHeader));
        VALGRIND_MAKE_MEM_DEFINED(MEMBLK_TOBLK(head),
                                  head->rawsize + Memento_PostSize);
        result = app(head, arg);
        next = head->next;
        VALGRIND_MAKE_MEM_NOACCESS(MEMBLK_POSTPTR(head), Memento_PostSize);
        VALGRIND_MAKE_MEM_NOACCESS(head, sizeof(Memento_BlkHeader));
        if (result)
            return result;
        head = next;
    }
    return 0;
}

static int Memento_appBlock(Memento_Blocks    *blks,
                            int                (*app)(Memento_BlkHeader *,
                                                      void *),
                            void              *arg,
                            Memento_BlkHeader *b)
{
    Memento_BlkHeader *head = blks->head;
    Memento_BlkHeader *next;
    int                result;
    while (head && head != b) {
        VALGRIND_MAKE_MEM_DEFINED(head, sizeof(Memento_BlkHeader));
        next = head->next;
        VALGRIND_MAKE_MEM_NOACCESS(MEMBLK_POSTPTR(head), Memento_PostSize);
        head = next;
    }
    if (head == b) {
        VALGRIND_MAKE_MEM_DEFINED(head, sizeof(Memento_BlkHeader));
        VALGRIND_MAKE_MEM_DEFINED(MEMBLK_TOBLK(head),
                                  head->rawsize + Memento_PostSize);
        result = app(head, arg);
        VALGRIND_MAKE_MEM_NOACCESS(MEMBLK_POSTPTR(head), Memento_PostSize);
        VALGRIND_MAKE_MEM_NOACCESS(head, sizeof(Memento_BlkHeader));
        return result;
    }
    return 0;
}

static void showBlock(Memento_BlkHeader *b, int space)
{
    fprintf(stderr, "0x%p:(size=%d,num=%d)",
            MEMBLK_TOBLK(b), (int)b->rawsize, b->sequence);
    if (b->label)
        fprintf(stderr, "%c(%s)", space, b->label);
}

static void blockDisplay(Memento_BlkHeader *b, int n)
{
    int i = 0;
    n++;
    while(i < n)
    {
        fprintf(stderr, "%s", &"                                "[32-((n-i)&31)]);
        i += ((n-i)&31);
    }
    showBlock(b, '\t');
    fprintf(stderr, "\n");
}

static int Memento_listBlock(Memento_BlkHeader *b,
                             void              *arg)
{
    int *counts = (int *)arg;
    blockDisplay(b, 0);
    counts[0]++;
    counts[1]+= b->rawsize;
    return 0;
}

static void doNestedDisplay(Memento_BlkHeader *b,
                            int depth)
{
    blockDisplay(b, depth);
    for (b = b->child; b; b = b->sibling)
        doNestedDisplay(b, depth+1);
}

static int ptrcmp(const void *a_, const void *b_)
{
    const char **a = (const char **)a_;
    const char **b = (const char **)b_;
    return (int)(*a-*b);
}

int Memento_listBlocksNested(void)
{
    int count, size, i;
    Memento_BlkHeader *b;
    void **blocks, *minptr, *maxptr;
    int mask;

    
    count = 0;
    size = 0;
    for (b = globals.used.head; b; b = b->next) {
        size += b->rawsize;
        count++;
    }

    
    blocks = MEMENTO_UNDERLYING_MALLOC(sizeof(void *) * count);
    if (blocks == NULL)
        return 1;

    
    b = globals.used.head;
    minptr = maxptr = MEMBLK_TOBLK(b);
    mask = (int)minptr;
    for (i = 0; b; b = b->next, i++) {
        void *p = MEMBLK_TOBLK(b);
        mask &= (int)p;
        if (p < minptr)
            minptr = p;
        if (p > maxptr)
            maxptr = p;
        blocks[i] = p;
        b->flags &= ~Memento_Flag_HasParent;
        b->child   = NULL;
        b->sibling = NULL;
    }
    qsort(blocks, count, sizeof(void *), ptrcmp);

    
    for (b = globals.used.head; b; b = b->next) {
        char *p = MEMBLK_TOBLK(b);
        int end = (b->rawsize < MEMENTO_PTRSEARCH ? b->rawsize : MEMENTO_PTRSEARCH);
        for (i = 0; i < end; i += sizeof(void *)) {
            void *q = *(void **)(&p[i]);
            void **r;

            
            if ((mask & (int)q) != mask || q < minptr || q > maxptr)
                continue;

            
            r = bsearch(&q, blocks, count, sizeof(void *), ptrcmp);
            if (r) {
                
                Memento_BlkHeader *child = MEMBLK_FROMBLK(*r);

                
                if (child->flags & Memento_Flag_HasParent)
                    continue;

                child->sibling = b->child;
                b->child = child;
                child->flags |= Memento_Flag_HasParent;
            }
        }
    }

    
    for (b = globals.used.head; b; b = b->next) {
        if ((b->flags & Memento_Flag_HasParent) == 0)
            doNestedDisplay(b, 0);
    }
    fprintf(stderr, " Total number of blocks = %d\n", count);
    fprintf(stderr, " Total size of blocks = %d\n", size);

    MEMENTO_UNDERLYING_FREE(blocks);
    return 0;
}

void Memento_listBlocks(void)
{
    fprintf(stderr, "Allocated blocks:\n");
    if (Memento_listBlocksNested())
    {
        int counts[2];
        counts[0] = 0;
        counts[1] = 0;
        Memento_appBlocks(&globals.used, Memento_listBlock, &counts[0]);
        fprintf(stderr, " Total number of blocks = %d\n", counts[0]);
        fprintf(stderr, " Total size of blocks = %d\n", counts[1]);
    }
}

static int Memento_listNewBlock(Memento_BlkHeader *b,
                                void              *arg)
{
    if (b->flags & Memento_Flag_OldBlock)
        return 0;
    b->flags |= Memento_Flag_OldBlock;
    return Memento_listBlock(b, arg);
}

void Memento_listNewBlocks(void) {
    int counts[2];
    counts[0] = 0;
    counts[1] = 0;
    fprintf(stderr, "Blocks allocated and still extant since last list:\n");
    Memento_appBlocks(&globals.used, Memento_listNewBlock, &counts[0]);
    fprintf(stderr, "  Total number of blocks = %d\n", counts[0]);
    fprintf(stderr, "  Total size of blocks = %d\n", counts[1]);
}

static void Memento_endStats(void)
{
    fprintf(stderr, "Total memory malloced = %u bytes\n", (unsigned int)globals.totalAlloc);
    fprintf(stderr, "Peak memory malloced = %u bytes\n", (unsigned int)globals.peakAlloc);
    fprintf(stderr, "%u mallocs, %u frees, %u reallocs\n", (unsigned int)globals.numMallocs,
            (unsigned int)globals.numFrees, (unsigned int)globals.numReallocs);
    fprintf(stderr, "Average allocation size %u bytes\n", (unsigned int)
            (globals.numMallocs != 0 ? globals.totalAlloc/globals.numMallocs: 0));
}

void Memento_stats(void)
{
    fprintf(stderr, "Current memory malloced = %u bytes\n", (unsigned int)globals.alloc);
    Memento_endStats();
}

static void Memento_fin(void)
{
    Memento_checkAllMemory();
    Memento_endStats();
    if (globals.used.head != NULL) {
        Memento_listBlocks();
        Memento_breakpoint();
    }
    if (globals.segv) {
        fprintf(stderr, "Memory dumped on SEGV while squeezing @ %d\n", globals.failAt);
    } else if (globals.squeezing) {
        if (globals.pattern == 0)
            fprintf(stderr, "Memory squeezing @ %d complete\n", globals.squeezeAt);
        else
            fprintf(stderr, "Memory squeezing @ %d (%d) complete\n", globals.squeezeAt, globals.pattern);
    }
    if (globals.failing)
    {
        fprintf(stderr, "MEMENTO_FAILAT=%d\n", globals.failAt);
        fprintf(stderr, "MEMENTO_PATTERN=%d\n", globals.pattern);
    }
    if (globals.nextFailAt != 0)
    {
        fprintf(stderr, "MEMENTO_NEXTFAILAT=%d\n", globals.nextFailAt);
        fprintf(stderr, "MEMENTO_NEXTPATTERN=%d\n", globals.nextPattern);
    }
}

static void Memento_inited(void)
{
    
}

static void Memento_init(void)
{
    char *env;
    memset(&globals, 0, sizeof(globals));
    globals.inited    = 1;
    globals.used.head = NULL;
    globals.used.tail = &globals.used.head;
    globals.free.head = NULL;
    globals.free.tail = &globals.free.head;
    globals.sequence  = 0;
    globals.countdown = 1024;

    env = getenv("MEMENTO_FAILAT");
    globals.failAt = (env ? atoi(env) : 0);

    env = getenv("MEMENTO_PARANOIA");
    globals.paranoia = (env ? atoi(env) : 0);
    if (globals.paranoia == 0)
        globals.paranoia = 1024;

    env = getenv("MEMENTO_PARANOIDAT");
    globals.paranoidAt = (env ? atoi(env) : 0);

    env = getenv("MEMENTO_SQUEEZEAT");
    globals.squeezeAt = (env ? atoi(env) : 0);

    env = getenv("MEMENTO_PATTERN");
    globals.pattern = (env ? atoi(env) : 0);

    env = getenv("MEMENTO_MAXMEMORY");
    globals.maxMemory = (env ? atoi(env) : 0);

    atexit(Memento_fin);

    Memento_inited();
}

#ifdef MEMENTO_HAS_FORK
#include <unistd.h>
#include <sys/wait.h>
#ifdef MEMENTO_STACKTRACE_METHOD
#if MEMENTO_STACKTRACE_METHOD == 1
#include <signal.h>
#endif
#endif

#define OPEN_MAX 10240

int stashed_map[OPEN_MAX];

extern size_t backtrace(void **, int);
extern void backtrace_symbols_fd(void **, size_t, int);

static void Memento_signal(void)
{
    fprintf(stderr, "SEGV after Memory squeezing @ %d\n", globals.squeezeAt);

#ifdef MEMENTO_STACKTRACE_METHOD
#if MEMENTO_STACKTRACE_METHOD == 1
    {
      void *array[100];
      size_t size;

      size = backtrace(array, 100);
      fprintf(stderr, "------------------------------------------------------------------------\n");
      fprintf(stderr, "Backtrace:\n");
      backtrace_symbols_fd(array, size, 2);
      fprintf(stderr, "------------------------------------------------------------------------\n");
    }
#endif
#endif

    exit(1);
}

static int squeeze(void)
{
    pid_t pid;
    int i, status;

    if (globals.patternBit < 0)
        return 1;
    if (globals.squeezing && globals.patternBit >= MEMENTO_MAXPATTERN)
        return 1;

    if (globals.patternBit == 0)
        globals.squeezeAt = globals.sequence;

    if (!globals.squeezing) {
        fprintf(stderr, "Memory squeezing @ %d\n", globals.squeezeAt);
    } else
        fprintf(stderr, "Memory squeezing @ %d (%x,%x)\n", globals.squeezeAt, globals.pattern, globals.patternBit);

    
    for (i = 0; i < OPEN_MAX; i++) {
        if (stashed_map[i] == 0) {
            int j = dup(i);
            stashed_map[j] = i+1;
        }
    }

    pid = fork();
    if (pid == 0) {
        
        signal(SIGSEGV, Memento_signal);
        
        if (globals.patternBit == 0) {
            globals.patternBit = 1;
        } else
            globals.patternBit <<= 1;
        globals.squeezing = 1;
        return 1;
    }

    
    globals.pattern |= globals.patternBit;
    globals.patternBit <<= 1;

    
    waitpid(pid, &status, 0);

    if (status != 0) {
        fprintf(stderr, "Child status=%d\n", status);
    }

    
    for (i = 0; i < OPEN_MAX; i++) {
        if (stashed_map[i] != 0) {
            dup2(i, stashed_map[i]-1);
            close(i);
            stashed_map[i] = 0;
        }
    }

    return 0;
}
#else
#include <signal.h>

static void Memento_signal(void)
{
    globals.segv = 1;
    
    if (getenv("MEMENTO_NOJIT"))
        exit(1);
    else
        Memento_fin();
}

int squeeze(void)
{
    fprintf(stderr, "Memento memory squeezing disabled as no fork!\n");
    return 0;
}
#endif

static void Memento_startFailing(void)
{
    if (!globals.failing) {
        fprintf(stderr, "Starting to fail...\n");
        fflush(stderr);
        globals.failing = 1;
        globals.failAt = globals.sequence;
        globals.nextFailAt = globals.sequence+1;
        globals.pattern = 0;
        globals.patternBit = 0;
        signal(SIGSEGV, Memento_signal);
        signal(SIGABRT, Memento_signal);
        Memento_breakpoint();
    }
}

static void Memento_event(void)
{
    globals.sequence++;
    if ((globals.sequence >= globals.paranoidAt) && (globals.paranoidAt != 0)) {
        globals.paranoia = 1;
        globals.countdown = 1;
    }
    if (--globals.countdown == 0) {
        Memento_checkAllMemory();
        globals.countdown = globals.paranoia;
    }

    if (globals.sequence == globals.breakAt) {
        fprintf(stderr, "Breaking at event %d\n", globals.breakAt);
        Memento_breakpoint();
    }
}

int Memento_breakAt(int event)
{
    globals.breakAt = event;
    return event;
}

void *Memento_label(void *ptr, const char *label)
{
    Memento_BlkHeader *block;

    if (ptr == NULL)
        return NULL;
    block = MEMBLK_FROMBLK(ptr);
    block->label = label;
    return ptr;
}

int Memento_failThisEvent(void)
{
    int failThisOne;

    if (!globals.inited)
        Memento_init();

    Memento_event();

    if ((globals.sequence >= globals.failAt) && (globals.failAt != 0))
        Memento_startFailing();
    if ((globals.sequence >= globals.squeezeAt) && (globals.squeezeAt != 0)) {
        return squeeze();
    }

    if (!globals.failing)
        return 0;
    failThisOne = ((globals.patternBit & globals.pattern) == 0);
    
    if (globals.failing &&
        ((~(globals.patternBit-1) & globals.pattern) == 0) &&
        (globals.patternBit != 0) &&
        globals.nextPattern == 0)
    {
        
        globals.nextFailAt = globals.failAt;
        globals.nextPattern = globals.pattern | globals.patternBit;
    }
    globals.patternBit = (globals.patternBit ? globals.patternBit << 1 : 1);

    return failThisOne;
}

void *Memento_malloc(size_t s)
{
    Memento_BlkHeader *memblk;
    size_t             smem = MEMBLK_SIZE(s);

    if (Memento_failThisEvent())
        return NULL;

    if (s == 0)
        return NULL;

    globals.numMallocs++;

    if (globals.maxMemory != 0 && globals.alloc + s > globals.maxMemory)
        return NULL;

    memblk = MEMENTO_UNDERLYING_MALLOC(smem);
    if (memblk == NULL)
        return NULL;

    globals.alloc      += s;
    globals.totalAlloc += s;
    if (globals.peakAlloc < globals.alloc)
        globals.peakAlloc = globals.alloc;
#ifndef MEMENTO_LEAKONLY
    memset(MEMBLK_TOBLK(memblk), MEMENTO_ALLOCFILL, s);
#endif
    memblk->rawsize       = s;
    memblk->sequence      = globals.sequence;
    memblk->lastCheckedOK = memblk->sequence;
    memblk->flags         = 0;
    memblk->label         = 0;
    memblk->child         = NULL;
    memblk->sibling       = NULL;
    Memento_addBlockHead(&globals.used, memblk, 0);
    return MEMBLK_TOBLK(memblk);
}

void *Memento_calloc(size_t n, size_t s)
{
    void *block = Memento_malloc(n*s);

    if (block)
        memset(block, 0, n*s);
    return block;
}

static int checkBlock(Memento_BlkHeader *memblk, const char *action)
{
#ifndef MEMENTO_LEAKONLY
    BlkCheckData data;

    memset(&data, 0, sizeof(data));
    Memento_appBlock(&globals.used, Memento_Internal_checkAllocedBlock,
                     &data, memblk);
    if (!data.found) {
        
        fprintf(stderr, "Attempt to %s block ", action);
        showBlock(memblk, 32);
        Memento_breakpoint();
        return 1;
    } else if (data.preCorrupt || data.postCorrupt) {
        fprintf(stderr, "Block ");
        showBlock(memblk, ' ');
        fprintf(stderr, " found to be corrupted on %s!\n", action);
        if (data.preCorrupt) {
            fprintf(stderr, "Preguard corrupted\n");
        }
        if (data.postCorrupt) {
            fprintf(stderr, "Postguard corrupted\n");
        }
        fprintf(stderr, "Block last checked OK at allocation %d. Now %d.\n",
                memblk->lastCheckedOK, globals.sequence);
        Memento_breakpoint();
        return 1;
    }
#endif
    return 0;
}

void Memento_free(void *blk)
{
    Memento_BlkHeader *memblk;

    if (!globals.inited)
        Memento_init();

    Memento_event();

    if (blk == NULL)
        return;

    memblk = MEMBLK_FROMBLK(blk);
    VALGRIND_MAKE_MEM_DEFINED(memblk, sizeof(*memblk));
    if (checkBlock(memblk, "free"))
        return;

    VALGRIND_MAKE_MEM_DEFINED(memblk, sizeof(*memblk));
    globals.alloc -= memblk->rawsize;
    globals.numFrees++;

    Memento_removeBlock(&globals.used, memblk);

    VALGRIND_MAKE_MEM_DEFINED(memblk, sizeof(*memblk));
    if (Memento_Internal_makeSpace(MEMBLK_SIZE(memblk->rawsize))) {
        VALGRIND_MAKE_MEM_DEFINED(memblk, sizeof(*memblk));
        VALGRIND_MAKE_MEM_DEFINED(MEMBLK_TOBLK(memblk),
                                  memblk->rawsize + Memento_PostSize);
#ifndef MEMENTO_LEAKONLY
        memset(MEMBLK_TOBLK(memblk), MEMENTO_FREEFILL, memblk->rawsize);
#endif
        Memento_addBlockTail(&globals.free, memblk, 1);
    } else {
        MEMENTO_UNDERLYING_FREE(memblk);
    }
}

void *Memento_realloc(void *blk, size_t newsize)
{
    Memento_BlkHeader *memblk, *newmemblk;
    size_t             newsizemem;

    if (blk == NULL)
        return Memento_malloc(newsize);
    if (newsize == 0) {
        Memento_free(blk);
        return NULL;
    }

    if (Memento_failThisEvent())
        return NULL;

    memblk     = MEMBLK_FROMBLK(blk);
    if (checkBlock(memblk, "realloc"))
        return NULL;

    if (globals.maxMemory != 0 && globals.alloc - memblk->rawsize + newsize > globals.maxMemory)
        return NULL;

    newsizemem = MEMBLK_SIZE(newsize);
    Memento_removeBlock(&globals.used, memblk);
    newmemblk  = MEMENTO_UNDERLYING_REALLOC(memblk, newsizemem);
    if (newmemblk == NULL)
    {
        Memento_addBlockHead(&globals.used, memblk, 2);
        return NULL;
    }
    globals.numReallocs++;
    globals.totalAlloc += newsize;
    globals.alloc      -= newmemblk->rawsize;
    globals.alloc      += newsize;
    if (globals.peakAlloc < globals.alloc)
        globals.peakAlloc = globals.alloc;
    if (newmemblk->rawsize < newsize) {
        char *newbytes = ((char *)MEMBLK_TOBLK(newmemblk))+newmemblk->rawsize;
#ifndef MEMENTO_LEAKONLY
        memset(newbytes, MEMENTO_ALLOCFILL, newsize - newmemblk->rawsize);
#endif
        VALGRIND_MAKE_MEM_UNDEFINED(newbytes, newsize - newmemblk->rawsize);
    }
    newmemblk->rawsize = newsize;
#ifndef MEMENTO_LEAKONLY
    memset(newmemblk->preblk, MEMENTO_PREFILL, Memento_PreSize);
    memset(MEMBLK_POSTPTR(newmemblk), MEMENTO_POSTFILL, Memento_PostSize);
#endif
    Memento_addBlockHead(&globals.used, newmemblk, 2);
    return MEMBLK_TOBLK(newmemblk);
}

int Memento_checkBlock(void *blk)
{
    Memento_BlkHeader *memblk;

    if (blk == NULL)
        return 0;
    memblk = MEMBLK_FROMBLK(blk);
    return checkBlock(memblk, "check");
}

static int Memento_Internal_checkAllAlloced(Memento_BlkHeader *memblk, void *arg)
{
    BlkCheckData *data = (BlkCheckData *)arg;

    Memento_Internal_checkAllocedBlock(memblk, data);
    if (data->preCorrupt || data->postCorrupt) {
        if ((data->found & 2) == 0) {
            fprintf(stderr, "Allocated blocks:\n");
            data->found |= 2;
        }
        fprintf(stderr, "  Block ");
        showBlock(memblk, ' ');
        if (data->preCorrupt) {
            fprintf(stderr, " Preguard ");
        }
        if (data->postCorrupt) {
            fprintf(stderr, "%s Postguard ",
                    (data->preCorrupt ? "&" : ""));
        }
        fprintf(stderr, "corrupted.\n    "
                "Block last checked OK at allocation %d. Now %d.\n",
                memblk->lastCheckedOK, globals.sequence);
        data->preCorrupt  = 0;
        data->postCorrupt = 0;
        data->freeCorrupt = 0;
    }
    else
        memblk->lastCheckedOK = globals.sequence;
    return 0;
}

static int Memento_Internal_checkAllFreed(Memento_BlkHeader *memblk, void *arg)
{
    BlkCheckData *data = (BlkCheckData *)arg;

    Memento_Internal_checkFreedBlock(memblk, data);
    if (data->preCorrupt || data->postCorrupt || data->freeCorrupt) {
        if ((data->found & 4) == 0) {
            fprintf(stderr, "Freed blocks:\n");
            data->found |= 4;
        }
        fprintf(stderr, "  ");
        showBlock(memblk, ' ');
        if (data->freeCorrupt) {
            fprintf(stderr, " index %d (address 0x%p) onwards", data->index,
                    &((char *)MEMBLK_TOBLK(memblk))[data->index]);
            if (data->preCorrupt) {
                fprintf(stderr, "+ preguard");
            }
            if (data->postCorrupt) {
                fprintf(stderr, "+ postguard");
            }
        } else {
            if (data->preCorrupt) {
                fprintf(stderr, " preguard");
            }
            if (data->postCorrupt) {
                fprintf(stderr, "%s Postguard",
                        (data->preCorrupt ? "+" : ""));
            }
        }
        fprintf(stderr, " corrupted.\n"
                "    Block last checked OK at allocation %d. Now %d.\n",
                memblk->lastCheckedOK, globals.sequence);
        data->preCorrupt  = 0;
        data->postCorrupt = 0;
        data->freeCorrupt = 0;
    }
    else
        memblk->lastCheckedOK = globals.sequence;
    return 0;
}

int Memento_checkAllMemory(void)
{
#ifndef MEMENTO_LEAKONLY
    BlkCheckData data;

    memset(&data, 0, sizeof(data));
    Memento_appBlocks(&globals.used, Memento_Internal_checkAllAlloced, &data);
    Memento_appBlocks(&globals.free, Memento_Internal_checkAllFreed, &data);
    if (data.found & 6) {
        Memento_breakpoint();
        return 1;
    }
#endif
    return 0;
}

int Memento_setParanoia(int i)
{
    globals.paranoia = i;
    globals.countdown = globals.paranoia;
    return i;
}

int Memento_paranoidAt(int i)
{
    globals.paranoidAt = i;
    return i;
}

int Memento_getBlockNum(void *b)
{
    Memento_BlkHeader *memblk;
    if (b == NULL)
        return 0;
    memblk = MEMBLK_FROMBLK(b);
    return (memblk->sequence);
}

int Memento_check(void)
{
    int result;

    fprintf(stderr, "Checking memory\n");
    result = Memento_checkAllMemory();
    fprintf(stderr, "Memory checked!\n");
    return result;
}

typedef struct findBlkData {
    void              *addr;
    Memento_BlkHeader *blk;
    int                flags;
} findBlkData;

static int Memento_containsAddr(Memento_BlkHeader *b,
                                void *arg)
{
    findBlkData *data = (findBlkData *)arg;
    char *blkend = &((char *)MEMBLK_TOBLK(b))[b->rawsize];
    if ((MEMBLK_TOBLK(b) <= data->addr) &&
        ((void *)blkend > data->addr)) {
        data->blk = b;
        data->flags = 1;
        return 1;
    }
    if (((void *)b <= data->addr) &&
        (MEMBLK_TOBLK(b) > data->addr)) {
        data->blk = b;
        data->flags = 2;
        return 1;
    }
    if (((void *)blkend <= data->addr) &&
        ((void *)(blkend + Memento_PostSize) > data->addr)) {
        data->blk = b;
        data->flags = 3;
        return 1;
    }
    return 0;
}

int Memento_find(void *a)
{
    findBlkData data;

    data.addr  = a;
    data.blk   = NULL;
    data.flags = 0;
    Memento_appBlocks(&globals.used, Memento_containsAddr, &data);
    if (data.blk != NULL) {
        fprintf(stderr, "Address 0x%p is in %sallocated block ",
                data.addr,
                (data.flags == 1 ? "" : (data.flags == 2 ?
                                         "preguard of " : "postguard of ")));
        showBlock(data.blk, ' ');
        fprintf(stderr, "\n");
        return data.blk->sequence;
    }
    data.blk   = NULL;
    data.flags = 0;
    Memento_appBlocks(&globals.free, Memento_containsAddr, &data);
    if (data.blk != NULL) {
        fprintf(stderr, "Address 0x%p is in %sfreed block ",
                data.addr,
                (data.flags == 1 ? "" : (data.flags == 2 ?
                                         "preguard of " : "postguard of ")));
        showBlock(data.blk, ' ');
        fprintf(stderr, "\n");
        return data.blk->sequence;
    }
    return 0;
}

int Memento_failAt(int i)
{
    globals.failAt = i;
    if ((globals.sequence > globals.failAt) &&
        (globals.failing != 0))
        Memento_startFailing();
    return i;
}

size_t Memento_setMax(size_t max)
{
    globals.maxMemory = max;
    return max;
}

#else

void (Memento_breakpoint)(void)
{
}

int (Memento_checkBlock)(void *b)
{
    return 0;
}

int (Memento_checkAllMemory)(void)
{
    return 0;
}

int (Memento_check)(void)
{
    return 0;
}

int (Memento_setParanoia)(int i)
{
    return 0;
}

int (Memento_paranoidAt)(int i)
{
    return 0;
}

int (Memento_breakAt)(int i)
{
    return 0;
}

int  (Memento_getBlockNum)(void *i)
{
    return 0;
}

int (Memento_find)(void *a)
{
    return 0;
}

int (Memento_failAt)(int i)
{
    return 0;
}

#undef Memento_malloc
#undef Memento_free
#undef Memento_realloc
#undef Memento_calloc

void *Memento_malloc(size_t size)
{
    return MEMENTO_UNDERLYING_MALLOC(size);
}

void Memento_free(void *b)
{
    MEMENTO_UNDERLYING_FREE(b);
}

void *Memento_realloc(void *b, size_t s)
{
    return MEMENTO_UNDERLYING_REALLOC(b, s);
}

void *Memento_calloc(size_t n, size_t s)
{
    return MEMENTO_UNDERLYING_CALLOC(n, s);
}

void (Memento_listBlocks)(void)
{
}

void (Memento_listNewBlocks)(void)
{
}

size_t (Memento_setMax)(size_t max)
{
    return 0;
}

void (Memento_stats)(void)
{
}

void *(Memento_label)(void *ptr, const char *label)
{
    return ptr;
}

#endif
