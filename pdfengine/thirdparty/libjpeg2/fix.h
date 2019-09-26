
#ifndef __FIX_H
#define __FIX_H

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define int64 __int64
#else
#define int64 long long
#endif

static INLINE int fix_mul(int a, int b) {
    int64 temp = (int64) a * (int64) b ;
    temp += temp & 4096;
    return (int) (temp >> 13) ;
}

#endif 
