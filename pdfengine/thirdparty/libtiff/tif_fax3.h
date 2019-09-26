

#ifndef _FAX3_
#define	_FAX3_

#include "tiff.h"

typedef	void (*TIFFFaxFillFunc)(unsigned char*, uint32*, uint32*, uint32);

#if defined(__cplusplus)
extern "C" {
#endif
extern	void _TIFFFax3fillruns(unsigned char*, uint32*, uint32*, uint32);
#if defined(__cplusplus)
}
#endif

#define S_Null		0
#define S_Pass		1
#define S_Horiz		2
#define S_V0		3
#define S_VR		4
#define S_VL		5
#define S_Ext		6
#define S_TermW		7
#define S_TermB		8
#define S_MakeUpW	9
#define S_MakeUpB	10
#define S_MakeUp	11
#define S_EOL		12

typedef struct {		
	unsigned char State;	
	unsigned char Width;	
	uint32	Param;		
} TIFFFaxTabEnt;

extern	const TIFFFaxTabEnt TIFFFaxMainTable[];
extern	const TIFFFaxTabEnt TIFFFaxWhiteTable[];
extern	const TIFFFaxTabEnt TIFFFaxBlackTable[];

#ifndef EndOfData
#define EndOfData()	(cp >= ep)
#endif

#ifndef NeedBits8
#define NeedBits8(n,eoflab) do {					\
    if (BitsAvail < (n)) {						\
	if (EndOfData()) {						\
	    if (BitsAvail == 0)				\
		goto eoflab;						\
	    BitsAvail = (n);				\
	} else {							\
	    BitAcc |= ((uint32) bitmap[*cp++])<<BitsAvail;		\
	    BitsAvail += 8;						\
	}								\
    }									\
} while (0)
#endif
#ifndef NeedBits16
#define NeedBits16(n,eoflab) do {					\
    if (BitsAvail < (n)) {						\
	if (EndOfData()) {						\
	    if (BitsAvail == 0)				\
		goto eoflab;						\
	    BitsAvail = (n);				\
	} else {							\
	    BitAcc |= ((uint32) bitmap[*cp++])<<BitsAvail;		\
	    if ((BitsAvail += 8) < (n)) {				\
		if (EndOfData()) {					\
		    	\
		    BitsAvail = (n);			\
		} else {						\
		    BitAcc |= ((uint32) bitmap[*cp++])<<BitsAvail;	\
		    BitsAvail += 8;					\
		}							\
	    }								\
	}								\
    }									\
} while (0)
#endif
#define GetBits(n)	(BitAcc & ((1<<(n))-1))
#define ClrBits(n) do {							\
    BitsAvail -= (n);							\
    BitAcc >>= (n);							\
} while (0)

#ifdef FAX3_DEBUG
static const char* StateNames[] = {
    "Null   ",
    "Pass   ",
    "Horiz  ",
    "V0     ",
    "VR     ",
    "VL     ",
    "Ext    ",
    "TermW  ",
    "TermB  ",
    "MakeUpW",
    "MakeUpB",
    "MakeUp ",
    "EOL    ",
};
#define DEBUG_SHOW putchar(BitAcc & (1 << t) ? '1' : '0')
#define LOOKUP8(wid,tab,eoflab) do {					\
    int t;								\
    NeedBits8(wid,eoflab);						\
    TabEnt = tab + GetBits(wid);					\
    printf("%08lX/%d: %s%5d\t", (long) BitAcc, BitsAvail,		\
	   StateNames[TabEnt->State], TabEnt->Param);			\
    for (t = 0; t < TabEnt->Width; t++)					\
	DEBUG_SHOW;							\
    putchar('\n');							\
    fflush(stdout);							\
    ClrBits(TabEnt->Width);						\
} while (0)
#define LOOKUP16(wid,tab,eoflab) do {					\
    int t;								\
    NeedBits16(wid,eoflab);						\
    TabEnt = tab + GetBits(wid);					\
    printf("%08lX/%d: %s%5d\t", (long) BitAcc, BitsAvail,		\
	   StateNames[TabEnt->State], TabEnt->Param);			\
    for (t = 0; t < TabEnt->Width; t++)					\
	DEBUG_SHOW;							\
    putchar('\n');							\
    fflush(stdout);							\
    ClrBits(TabEnt->Width);						\
} while (0)

#define SETVALUE(x) do {							\
    *pa++ = RunLength + (x);						\
    printf("SETVALUE: %d\t%d\n", RunLength + (x), a0);			\
    a0 += x;								\
    RunLength = 0;							\
} while (0)
#else
#define LOOKUP8(wid,tab,eoflab) do {					\
    NeedBits8(wid,eoflab);						\
    TabEnt = tab + GetBits(wid);					\
    ClrBits(TabEnt->Width);						\
} while (0)
#define LOOKUP16(wid,tab,eoflab) do {					\
    NeedBits16(wid,eoflab);						\
    TabEnt = tab + GetBits(wid);					\
    ClrBits(TabEnt->Width);						\
} while (0)

#define SETVALUE(x) do {							\
    *pa++ = RunLength + (x);						\
    a0 += (x);								\
    RunLength = 0;							\
} while (0)
#endif

#define	SYNC_EOL(eoflab) do {						\
    if (EOLcnt == 0) {							\
	for (;;) {							\
	    NeedBits16(11,eoflab);					\
	    if (GetBits(11) == 0)					\
		break;							\
	    ClrBits(1);							\
	}								\
    }									\
    for (;;) {								\
	NeedBits8(8,eoflab);						\
	if (GetBits(8))							\
	    break;							\
	ClrBits(8);							\
    }									\
    while (GetBits(1) == 0)						\
	ClrBits(1);							\
    ClrBits(1);							\
    EOLcnt = 0;					\
} while (0)

#define	CLEANUP_RUNS() do {						\
    if (RunLength)							\
	SETVALUE(0);							\
    if (a0 != lastx) {							\
	badlength(a0, lastx);						\
	while (a0 > lastx && pa > thisrun)				\
	    a0 -= *--pa;						\
	if (a0 < lastx) {						\
	    if (a0 < 0)							\
		a0 = 0;							\
	    if ((pa-thisrun)&1)						\
		SETVALUE(0);						\
	    SETVALUE(lastx - a0);						\
	} else if (a0 > lastx) {					\
	    SETVALUE(lastx);						\
	    SETVALUE(0);							\
	}								\
    }									\
} while (0)

#define EXPAND1D(eoflab) do {						\
    for (;;) {								\
	for (;;) {							\
	    LOOKUP16(12, TIFFFaxWhiteTable, eof1d);			\
	    switch (TabEnt->State) {					\
	    case S_EOL:							\
		EOLcnt = 1;						\
		goto done1d;						\
	    case S_TermW:						\
		SETVALUE(TabEnt->Param);					\
		goto doneWhite1d;					\
	    case S_MakeUpW:						\
	    case S_MakeUp:						\
		a0 += TabEnt->Param;					\
		RunLength += TabEnt->Param;				\
		break;							\
	    default:							\
		unexpected("WhiteTable", a0);				\
		goto done1d;						\
	    }								\
	}								\
    doneWhite1d:							\
	if (a0 >= lastx)						\
	    goto done1d;						\
	for (;;) {							\
	    LOOKUP16(13, TIFFFaxBlackTable, eof1d);			\
	    switch (TabEnt->State) {					\
	    case S_EOL:							\
		EOLcnt = 1;						\
		goto done1d;						\
	    case S_TermB:						\
		SETVALUE(TabEnt->Param);					\
		goto doneBlack1d;					\
	    case S_MakeUpB:						\
	    case S_MakeUp:						\
		a0 += TabEnt->Param;					\
		RunLength += TabEnt->Param;				\
		break;							\
	    default:							\
		unexpected("BlackTable", a0);				\
		goto done1d;						\
	    }								\
	}								\
    doneBlack1d:							\
	if (a0 >= lastx)						\
	    goto done1d;						\
        if( *(pa-1) == 0 && *(pa-2) == 0 )				\
            pa -= 2;                                                    \
    }									\
eof1d:									\
    prematureEOF(a0);							\
    CLEANUP_RUNS();							\
    goto eoflab;							\
done1d:									\
    CLEANUP_RUNS();							\
} while (0)

#define CHECK_b1 do {							\
    if (pa != thisrun) while (b1 <= a0 && b1 < lastx) {			\
	b1 += pb[0] + pb[1];						\
	pb += 2;							\
    }									\
} while (0)

#define EXPAND2D(eoflab) do {						\
    while (a0 < lastx) {						\
	LOOKUP8(7, TIFFFaxMainTable, eof2d);				\
	switch (TabEnt->State) {					\
	case S_Pass:							\
	    CHECK_b1;							\
	    b1 += *pb++;						\
	    RunLength += b1 - a0;					\
	    a0 = b1;							\
	    b1 += *pb++;						\
	    break;							\
	case S_Horiz:							\
	    if ((pa-thisrun)&1) {					\
		for (;;) {				\
		    LOOKUP16(13, TIFFFaxBlackTable, eof2d);		\
		    switch (TabEnt->State) {				\
		    case S_TermB:					\
			SETVALUE(TabEnt->Param);				\
			goto doneWhite2da;				\
		    case S_MakeUpB:					\
		    case S_MakeUp:					\
			a0 += TabEnt->Param;				\
			RunLength += TabEnt->Param;			\
			break;						\
		    default:						\
			goto badBlack2d;				\
		    }							\
		}							\
	    doneWhite2da:;						\
		for (;;) {				\
		    LOOKUP16(12, TIFFFaxWhiteTable, eof2d);		\
		    switch (TabEnt->State) {				\
		    case S_TermW:					\
			SETVALUE(TabEnt->Param);				\
			goto doneBlack2da;				\
		    case S_MakeUpW:					\
		    case S_MakeUp:					\
			a0 += TabEnt->Param;				\
			RunLength += TabEnt->Param;			\
			break;						\
		    default:						\
			goto badWhite2d;				\
		    }							\
		}							\
	    doneBlack2da:;						\
	    } else {							\
		for (;;) {				\
		    LOOKUP16(12, TIFFFaxWhiteTable, eof2d);		\
		    switch (TabEnt->State) {				\
		    case S_TermW:					\
			SETVALUE(TabEnt->Param);				\
			goto doneWhite2db;				\
		    case S_MakeUpW:					\
		    case S_MakeUp:					\
			a0 += TabEnt->Param;				\
			RunLength += TabEnt->Param;			\
			break;						\
		    default:						\
			goto badWhite2d;				\
		    }							\
		}							\
	    doneWhite2db:;						\
		for (;;) {				\
		    LOOKUP16(13, TIFFFaxBlackTable, eof2d);		\
		    switch (TabEnt->State) {				\
		    case S_TermB:					\
			SETVALUE(TabEnt->Param);				\
			goto doneBlack2db;				\
		    case S_MakeUpB:					\
		    case S_MakeUp:					\
			a0 += TabEnt->Param;				\
			RunLength += TabEnt->Param;			\
			break;						\
		    default:						\
			goto badBlack2d;				\
		    }							\
		}							\
	    doneBlack2db:;						\
	    }								\
	    CHECK_b1;							\
	    break;							\
	case S_V0:							\
	    CHECK_b1;							\
	    SETVALUE(b1 - a0);						\
	    b1 += *pb++;						\
	    break;							\
	case S_VR:							\
	    CHECK_b1;							\
	    SETVALUE(b1 - a0 + TabEnt->Param);				\
	    b1 += *pb++;						\
	    break;							\
	case S_VL:							\
	    CHECK_b1;							\
	    SETVALUE(b1 - a0 - TabEnt->Param);				\
	    b1 -= *--pb;						\
	    break;							\
	case S_Ext:							\
	    *pa++ = lastx - a0;						\
	    extension(a0);						\
	    goto eol2d;							\
	case S_EOL:							\
	    *pa++ = lastx - a0;						\
	    NeedBits8(4,eof2d);						\
	    if (GetBits(4))						\
		unexpected("EOL", a0);					\
            ClrBits(4);                                                 \
	    EOLcnt = 1;							\
	    goto eol2d;							\
	default:							\
	badMain2d:							\
	    unexpected("MainTable", a0);				\
	    goto eol2d;							\
	badBlack2d:							\
	    unexpected("BlackTable", a0);				\
	    goto eol2d;							\
	badWhite2d:							\
	    unexpected("WhiteTable", a0);				\
	    goto eol2d;							\
	eof2d:								\
	    prematureEOF(a0);						\
	    CLEANUP_RUNS();						\
	    goto eoflab;						\
	}								\
    }									\
    if (RunLength) {							\
	if (RunLength + a0 < lastx) {					\
	    					\
	    NeedBits8(1,eof2d);						\
	    if (!GetBits(1))						\
		goto badMain2d;						\
	    ClrBits(1);							\
	}								\
	SETVALUE(0);							\
    }									\
eol2d:									\
    CLEANUP_RUNS();							\
} while (0)
#endif 

