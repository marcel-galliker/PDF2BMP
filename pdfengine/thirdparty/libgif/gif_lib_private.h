#ifndef _GIF_LIB_PRIVATE_H
#define _GIF_LIB_PRIVATE_H

#include "gif_lib.h"
#include "gif_hash.h"

#define PROGRAM_NAME "GIFLIB"

#ifdef SYSV
#define VersionStr "Gif library module,\t\tEric S. Raymond\n\
                    (C) Copyright 1997 Eric S. Raymond\n"
#else
#define VersionStr PROGRAM_NAME "    IBMPC " GIF_LIB_VERSION \
                    "    Eric S. Raymond,    " __DATE__ ",   " \
                    __TIME__ "\n" "(C) Copyright 1997 Eric S. Raymond\n"
#endif 

#define LZ_MAX_CODE         4095    
#define LZ_BITS             12

#define FLUSH_OUTPUT        4096    
#define FIRST_CODE          4097    
#define NO_SUCH_CODE        4098    

#define FILE_STATE_WRITE    0x01
#define FILE_STATE_SCREEN   0x02
#define FILE_STATE_IMAGE    0x04
#define FILE_STATE_READ     0x08

#define IS_READABLE(Private)    (Private->FileState & FILE_STATE_READ)
#define IS_WRITEABLE(Private)   (Private->FileState & FILE_STATE_WRITE)

typedef struct GifFilePrivateType {
    GifWord FileState, FileHandle,  
      BitsPerPixel,     
      ClearCode,   
      EOFCode,     
      RunningCode, 
      RunningBits, 
      MaxCode1,    
      LastCode,    
      CrntCode,    
      StackPtr,    
      CrntShiftState;    
    unsigned long CrntShiftDWord;   
    unsigned long PixelCount;   
    FILE *File;    
    InputFunc Read;     
    OutputFunc Write;   
    GifByteType Buf[256];   
    GifByteType Stack[LZ_MAX_CODE]; 
    GifByteType Suffix[LZ_MAX_CODE + 1];    
    GifPrefixType Prefix[LZ_MAX_CODE + 1];
    GifHashTableType *HashTable;
} GifFilePrivateType;

extern int _GifError;

#endif 
