

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gif_lib.h"
#include "gif_lib_private.h"

#define COMMENT_EXT_FUNC_CODE 0xfe  

#define READ(_gif,_buf,_len)                                     \
  (((GifFilePrivateType*)_gif->Private)->Read ?                   \
    ((GifFilePrivateType*)_gif->Private)->Read(_gif,_buf,_len) : \
    fread(_buf,1,_len,((GifFilePrivateType*)_gif->Private)->File))

static int DGifGetWord(GifFileType *GifFile, GifWord *Word);
static int DGifSetupDecompress(GifFileType *GifFile);
static int DGifDecompressLine(GifFileType *GifFile, GifPixelType *Line,
                              int LineLen);
static int DGifGetPrefixChar(GifPrefixType *Prefix, int Code, int ClearCode);
static int DGifDecompressInput(GifFileType *GifFile, int *Code);
static int DGifBufferedInput(GifFileType *GifFile, GifByteType *Buf,
                             GifByteType *NextByte);
#ifndef _GBA_NO_FILEIO

GifFileType *
DGifOpenFileName(const char *FileName) {
    int FileHandle;
    GifFileType *GifFile;

    if ((FileHandle = _open(FileName, O_RDONLY
#if defined(__MSDOS__) || defined(WINDOWS32) || defined(_OPEN_BINARY)
                           | O_BINARY
#endif 
         )) == -1) {
        _GifError = D_GIF_ERR_OPEN_FAILED;
        return NULL;
    }

    GifFile = DGifOpenFileHandle(FileHandle);
    return GifFile;
}

GifFileType *
DGifOpenFileHandle(int FileHandle) {

    unsigned char Buf[GIF_STAMP_LEN + 1];
    GifFileType *GifFile;
    GifFilePrivateType *Private;
    FILE *f;

    GifFile = (GifFileType *)malloc(sizeof(GifFileType));
    if (GifFile == NULL) {
        _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
        _close(FileHandle);
        return NULL;
    }

    memset(GifFile, '\0', sizeof(GifFileType));

    Private = (GifFilePrivateType *)malloc(sizeof(GifFilePrivateType));
    if (Private == NULL) {
        _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
        _close(FileHandle);
        free((char *)GifFile);
        return NULL;
    }
#if defined(__MSDOS__) || defined(WINDOWS32) || defined(_OPEN_BINARY)
    setmode(FileHandle, O_BINARY);    
#endif 

    f = _fdopen(FileHandle, "rb");    

#if defined(__MSDOS__) || defined(WINDOWS32)
    setvbuf(f, NULL, _IOFBF, GIF_FILE_BUFFER_SIZE);    
#endif 

    GifFile->Private = (VoidPtr)Private;
    Private->FileHandle = FileHandle;
    Private->File = f;
    Private->FileState = FILE_STATE_READ;
    Private->Read = 0;    
    GifFile->UserData = 0;    

    
    if (READ(GifFile, Buf, GIF_STAMP_LEN) != GIF_STAMP_LEN) {
        _GifError = D_GIF_ERR_READ_FAILED;
        fclose(f);
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    
    Buf[GIF_STAMP_LEN] = 0;
    if (strncmp(GIF_STAMP, Buf, GIF_VERSION_POS) != 0) {
        _GifError = D_GIF_ERR_NOT_GIF_FILE;
        fclose(f);
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    if (DGifGetScreenDesc(GifFile) == GIF_ERROR) {
        fclose(f);
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    _GifError = 0;

    return GifFile;
}

#endif 

GifFileType *
DGifOpen(void *userData,
         InputFunc readFunc) {

    unsigned char Buf[GIF_STAMP_LEN + 1];
    GifFileType *GifFile;
    GifFilePrivateType *Private;

    GifFile = (GifFileType *)malloc(sizeof(GifFileType));
    if (GifFile == NULL) {
        _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
        return NULL;
    }

    memset(GifFile, '\0', sizeof(GifFileType));

    Private = (GifFilePrivateType *)malloc(sizeof(GifFilePrivateType));
    if (!Private) {
        _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
        free((char *)GifFile);
        return NULL;
    }

    GifFile->Private = (VoidPtr)Private;
    Private->FileHandle = 0;
    Private->File = 0;
    Private->FileState = FILE_STATE_READ;

    Private->Read = readFunc;    
    GifFile->UserData = userData;    

    
    if (READ(GifFile, Buf, GIF_STAMP_LEN) != GIF_STAMP_LEN) {
        _GifError = D_GIF_ERR_READ_FAILED;
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    
    Buf[GIF_STAMP_LEN] = 0;
    if (strncmp(GIF_STAMP, Buf, GIF_VERSION_POS) != 0) {
        _GifError = D_GIF_ERR_NOT_GIF_FILE;
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    if (DGifGetScreenDesc(GifFile) == GIF_ERROR) {
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    _GifError = 0;

    return GifFile;
}

int
DGifGetScreenDesc(GifFileType * GifFile) {

    int i, BitsPerPixel;
    GifByteType Buf[3];
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_READABLE(Private)) {
        
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    
    if (DGifGetWord(GifFile, &GifFile->SWidth) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->SHeight) == GIF_ERROR)
        return GIF_ERROR;

    if (READ(GifFile, Buf, 3) != 3) {
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }
    GifFile->SColorResolution = (((Buf[0] & 0x70) + 1) >> 4) + 1;
    BitsPerPixel = (Buf[0] & 0x07) + 1;
    GifFile->SBackGroundColor = Buf[1];
    if (Buf[0] & 0x80) {    

        GifFile->SColorMap = MakeMapObject(1 << BitsPerPixel, NULL);
        if (GifFile->SColorMap == NULL) {
            _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }

        
        for (i = 0; i < GifFile->SColorMap->ColorCount; i++) {
            if (READ(GifFile, Buf, 3) != 3) {
                FreeMapObject(GifFile->SColorMap);
                GifFile->SColorMap = NULL;
                _GifError = D_GIF_ERR_READ_FAILED;
                return GIF_ERROR;
            }
            GifFile->SColorMap->Colors[i].Red = Buf[0];
            GifFile->SColorMap->Colors[i].Green = Buf[1];
            GifFile->SColorMap->Colors[i].Blue = Buf[2];
        }
    } else {
        GifFile->SColorMap = NULL;
    }

    return GIF_OK;
}

int
DGifGetRecordType(GifFileType * GifFile,
                  GifRecordType * Type) {

    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_READABLE(Private)) {
        
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    if (READ(GifFile, &Buf, 1) != 1) {
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }

    switch (Buf) {
      case ',':
          *Type = IMAGE_DESC_RECORD_TYPE;
          break;
      case '!':
          *Type = EXTENSION_RECORD_TYPE;
          break;
      case ';':
          *Type = TERMINATE_RECORD_TYPE;
          break;
      default:
          *Type = UNDEFINED_RECORD_TYPE;
          _GifError = D_GIF_ERR_WRONG_RECORD;
          return GIF_ERROR;
    }

    return GIF_OK;
}

int
DGifGetImageDesc(GifFileType * GifFile) {

    int i, BitsPerPixel;
    GifByteType Buf[3];
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;
    SavedImage *sp;

    if (!IS_READABLE(Private)) {
        
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    if (DGifGetWord(GifFile, &GifFile->Image.Left) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->Image.Top) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->Image.Width) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->Image.Height) == GIF_ERROR)
        return GIF_ERROR;
    if (READ(GifFile, Buf, 1) != 1) {
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }
    BitsPerPixel = (Buf[0] & 0x07) + 1;
    GifFile->Image.Interlace = (Buf[0] & 0x40);
    if (Buf[0] & 0x80) {    

        
        if (GifFile->Image.ColorMap && GifFile->SavedImages == NULL)
            FreeMapObject(GifFile->Image.ColorMap);

        GifFile->Image.ColorMap = MakeMapObject(1 << BitsPerPixel, NULL);
        if (GifFile->Image.ColorMap == NULL) {
            _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }

        
        for (i = 0; i < GifFile->Image.ColorMap->ColorCount; i++) {
            if (READ(GifFile, Buf, 3) != 3) {
                FreeMapObject(GifFile->Image.ColorMap);
                _GifError = D_GIF_ERR_READ_FAILED;
                GifFile->Image.ColorMap = NULL;
                return GIF_ERROR;
            }
            GifFile->Image.ColorMap->Colors[i].Red = Buf[0];
            GifFile->Image.ColorMap->Colors[i].Green = Buf[1];
            GifFile->Image.ColorMap->Colors[i].Blue = Buf[2];
        }
    } else if (GifFile->Image.ColorMap) {
        FreeMapObject(GifFile->Image.ColorMap);
        GifFile->Image.ColorMap = NULL;
    }

    if (GifFile->SavedImages) {
        if ((GifFile->SavedImages = (SavedImage *)realloc(GifFile->SavedImages,
                                      sizeof(SavedImage) *
                                      (GifFile->ImageCount + 1))) == NULL) {
            _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }
    } else {
        if ((GifFile->SavedImages =
             (SavedImage *) malloc(sizeof(SavedImage))) == NULL) {
            _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }
    }

    sp = &GifFile->SavedImages[GifFile->ImageCount];
    memcpy(&sp->ImageDesc, &GifFile->Image, sizeof(GifImageDesc));
    if (GifFile->Image.ColorMap != NULL) {
        sp->ImageDesc.ColorMap = MakeMapObject(
                                 GifFile->Image.ColorMap->ColorCount,
                                 GifFile->Image.ColorMap->Colors);
        if (sp->ImageDesc.ColorMap == NULL) {
            _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }
    }
    sp->RasterBits = (unsigned char *)NULL;
    sp->ExtensionBlockCount = 0;
    sp->ExtensionBlocks = (ExtensionBlock *) NULL;

    GifFile->ImageCount++;

    Private->PixelCount = (long)GifFile->Image.Width *
       (long)GifFile->Image.Height;

    DGifSetupDecompress(GifFile);  

    return GIF_OK;
}

int
DGifGetLine(GifFileType * GifFile,
            GifPixelType * Line,
            int LineLen) {

    GifByteType *Dummy;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
        
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    if (!LineLen)
        LineLen = GifFile->Image.Width;

#if defined(__MSDOS__) || defined(WINDOWS32) || defined(__GNUC__)
    if ((Private->PixelCount -= LineLen) > 0xffff0000UL) {
#else
    if ((Private->PixelCount -= LineLen) > 0xffff0000) {
#endif 
        _GifError = D_GIF_ERR_DATA_TOO_BIG;
        return GIF_ERROR;
    }

    if (DGifDecompressLine(GifFile, Line, LineLen) == GIF_OK) {
        if (Private->PixelCount == 0) {
            
            do
                if (DGifGetCodeNext(GifFile, &Dummy) == GIF_ERROR)
                    return GIF_ERROR;
            while (Dummy != NULL) ;
        }
        return GIF_OK;
    } else
        return GIF_ERROR;
}

int
DGifGetPixel(GifFileType * GifFile,
             GifPixelType Pixel) {
    
    GifByteType *Dummy;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
        
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }
#if defined(__MSDOS__) || defined(WINDOWS32) || defined(__GNUC__)
    if (--Private->PixelCount > 0xffff0000UL)
#else
    if (--Private->PixelCount > 0xffff0000)
#endif 
    {
        _GifError = D_GIF_ERR_DATA_TOO_BIG;
        return GIF_ERROR;
    }

    if (DGifDecompressLine(GifFile, &Pixel, 1) == GIF_OK) {
        if (Private->PixelCount == 0) {
            
            do
                if (DGifGetCodeNext(GifFile, &Dummy) == GIF_ERROR)
                    return GIF_ERROR;
            while (Dummy != NULL) ;
        }
        return GIF_OK;
    } else
        return GIF_ERROR;
}

int
DGifGetExtension(GifFileType * GifFile,
                 int *ExtCode,
                 GifByteType ** Extension) {

    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_READABLE(Private)) {
        
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    if (READ(GifFile, &Buf, 1) != 1) {
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }
    *ExtCode = Buf;

    return DGifGetExtensionNext(GifFile, Extension);
}

int
DGifGetExtensionNext(GifFileType * GifFile,
                     GifByteType ** Extension) {
    
    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (READ(GifFile, &Buf, 1) != 1) {
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }
    if (Buf > 0) {
        *Extension = Private->Buf;    
        (*Extension)[0] = Buf;  
        if (READ(GifFile, &((*Extension)[1]), Buf) != Buf) {
            _GifError = D_GIF_ERR_READ_FAILED;
            return GIF_ERROR;
        }
    } else
        *Extension = NULL;

    return GIF_OK;
}

int
DGifCloseFile(GifFileType * GifFile) {
    
    GifFilePrivateType *Private;
    FILE *File;

    if (GifFile == NULL)
        return GIF_ERROR;

    Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
        
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    File = Private->File;

    if (GifFile->Image.ColorMap) {
        FreeMapObject(GifFile->Image.ColorMap);
        GifFile->Image.ColorMap = NULL;
    }

    if (GifFile->SColorMap) {
        FreeMapObject(GifFile->SColorMap);
        GifFile->SColorMap = NULL;
    }

    if (Private) {
        free((char *)Private);
        Private = NULL;
    }

    if (GifFile->SavedImages) {
        FreeSavedImages(GifFile);
        GifFile->SavedImages = NULL;
    }

    free(GifFile);

    if (File && (fclose(File) != 0)) {
        _GifError = D_GIF_ERR_CLOSE_FAILED;
        return GIF_ERROR;
    }
    return GIF_OK;
}

static int
DGifGetWord(GifFileType * GifFile,
            GifWord *Word) {

    unsigned char c[2];

    if (READ(GifFile, c, 2) != 2) {
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }

    *Word = (((unsigned int)c[1]) << 8) + c[0];
    return GIF_OK;
}

int
DGifGetCode(GifFileType * GifFile,
            int *CodeSize,
            GifByteType ** CodeBlock) {

    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_READABLE(Private)) {
        
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    *CodeSize = Private->BitsPerPixel;

    return DGifGetCodeNext(GifFile, CodeBlock);
}

int
DGifGetCodeNext(GifFileType * GifFile,
                GifByteType ** CodeBlock) {

    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (READ(GifFile, &Buf, 1) != 1) {
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }

    if (Buf > 0) {
        *CodeBlock = Private->Buf;    
        (*CodeBlock)[0] = Buf;  
        if (READ(GifFile, &((*CodeBlock)[1]), Buf) != Buf) {
            _GifError = D_GIF_ERR_READ_FAILED;
            return GIF_ERROR;
        }
    } else {
        *CodeBlock = NULL;
        Private->Buf[0] = 0;    
        Private->PixelCount = 0;    
    }

    return GIF_OK;
}

static int
DGifSetupDecompress(GifFileType * GifFile) {

    int i, BitsPerPixel;
    GifByteType CodeSize;
    GifPrefixType *Prefix;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    READ(GifFile, &CodeSize, 1);    
    BitsPerPixel = CodeSize;

    Private->Buf[0] = 0;    
    Private->BitsPerPixel = BitsPerPixel;
    Private->ClearCode = (1 << BitsPerPixel);
    Private->EOFCode = Private->ClearCode + 1;
    Private->RunningCode = Private->EOFCode + 1;
    Private->RunningBits = BitsPerPixel + 1;    
    Private->MaxCode1 = 1 << Private->RunningBits;    
    Private->StackPtr = 0;    
    Private->LastCode = NO_SUCH_CODE;
    Private->CrntShiftState = 0;    
    Private->CrntShiftDWord = 0;

    Prefix = Private->Prefix;
    for (i = 0; i <= LZ_MAX_CODE; i++)
        Prefix[i] = NO_SUCH_CODE;

    return GIF_OK;
}

static int
DGifDecompressLine(GifFileType * GifFile,
                   GifPixelType * Line,
                   int LineLen) {

    int i = 0;
    int j, CrntCode, EOFCode, ClearCode, CrntPrefix, LastCode, StackPtr;
    GifByteType *Stack, *Suffix;
    GifPrefixType *Prefix;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    StackPtr = Private->StackPtr;
    Prefix = Private->Prefix;
    Suffix = Private->Suffix;
    Stack = Private->Stack;
    EOFCode = Private->EOFCode;
    ClearCode = Private->ClearCode;
    LastCode = Private->LastCode;

    if (StackPtr > LZ_MAX_CODE) {
        return GIF_ERROR;
    }

    if (StackPtr != 0) {
        
        while (StackPtr != 0 && i < LineLen)
            Line[i++] = Stack[--StackPtr];
    }

    while (i < LineLen) {    
        if (DGifDecompressInput(GifFile, &CrntCode) == GIF_ERROR)
            return GIF_ERROR;

        if (CrntCode == EOFCode) {
            
            if (i != LineLen - 1 || Private->PixelCount != 0) {
                _GifError = D_GIF_ERR_EOF_TOO_SOON;
                return GIF_ERROR;
            }
            i++;
        } else if (CrntCode == ClearCode) {
            
            for (j = 0; j <= LZ_MAX_CODE; j++)
                Prefix[j] = NO_SUCH_CODE;
            Private->RunningCode = Private->EOFCode + 1;
            Private->RunningBits = Private->BitsPerPixel + 1;
            Private->MaxCode1 = 1 << Private->RunningBits;
            LastCode = Private->LastCode = NO_SUCH_CODE;
        } else {
            
            if (CrntCode < ClearCode) {
                
                Line[i++] = CrntCode;
            } else {
                
                if (Prefix[CrntCode] == NO_SUCH_CODE) {
                    
                    if (CrntCode == Private->RunningCode - 2) {
                        CrntPrefix = LastCode;
                        Suffix[Private->RunningCode - 2] =
                           Stack[StackPtr++] = DGifGetPrefixChar(Prefix,
                                                                 LastCode,
                                                                 ClearCode);
                    } else {
                        _GifError = D_GIF_ERR_IMAGE_DEFECT;
                        return GIF_ERROR;
                    }
                } else
                    CrntPrefix = CrntCode;

                
                j = 0;
                while (j++ <= LZ_MAX_CODE &&
                       CrntPrefix > ClearCode && CrntPrefix <= LZ_MAX_CODE) {
                    Stack[StackPtr++] = Suffix[CrntPrefix];
                    CrntPrefix = Prefix[CrntPrefix];
                }
                if (j >= LZ_MAX_CODE || CrntPrefix > LZ_MAX_CODE) {
                    _GifError = D_GIF_ERR_IMAGE_DEFECT;
                    return GIF_ERROR;
                }
                
                Stack[StackPtr++] = CrntPrefix;

                
                while (StackPtr != 0 && i < LineLen)
                    Line[i++] = Stack[--StackPtr];
            }
            if (LastCode != NO_SUCH_CODE) {
                Prefix[Private->RunningCode - 2] = LastCode;

                if (CrntCode == Private->RunningCode - 2) {
                    
                    Suffix[Private->RunningCode - 2] =
                       DGifGetPrefixChar(Prefix, LastCode, ClearCode);
                } else {
                    Suffix[Private->RunningCode - 2] =
                       DGifGetPrefixChar(Prefix, CrntCode, ClearCode);
                }
            }
            LastCode = CrntCode;
        }
    }

    Private->LastCode = LastCode;
    Private->StackPtr = StackPtr;

    return GIF_OK;
}

static int
DGifGetPrefixChar(GifPrefixType *Prefix,
                  int Code,
                  int ClearCode) {

    int i = 0;

    while (Code > ClearCode && i++ <= LZ_MAX_CODE) {
        if (Code > LZ_MAX_CODE) {
            return NO_SUCH_CODE;
        }
        Code = Prefix[Code];
    }
    return Code;
}

int
DGifGetLZCodes(GifFileType * GifFile,
               int *Code) {
    
    GifByteType *CodeBlock;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_READABLE(Private)) {
        
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    if (DGifDecompressInput(GifFile, Code) == GIF_ERROR)
        return GIF_ERROR;

    if (*Code == Private->EOFCode) {
        
        do {
            if (DGifGetCodeNext(GifFile, &CodeBlock) == GIF_ERROR)
                return GIF_ERROR;
        } while (CodeBlock != NULL) ;

        *Code = -1;
    } else if (*Code == Private->ClearCode) {
        
        Private->RunningCode = Private->EOFCode + 1;
        Private->RunningBits = Private->BitsPerPixel + 1;
        Private->MaxCode1 = 1 << Private->RunningBits;
    }

    return GIF_OK;
}

static int
DGifDecompressInput(GifFileType * GifFile,
                    int *Code) {
    
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    GifByteType NextByte;
    static unsigned short CodeMasks[] = {
        0x0000, 0x0001, 0x0003, 0x0007,
        0x000f, 0x001f, 0x003f, 0x007f,
        0x00ff, 0x01ff, 0x03ff, 0x07ff,
        0x0fff
    };
    
    if (Private->RunningBits > LZ_BITS) {
        _GifError = D_GIF_ERR_IMAGE_DEFECT;
        return GIF_ERROR;
    }
    
    while (Private->CrntShiftState < Private->RunningBits) {
        
        if (DGifBufferedInput(GifFile, Private->Buf, &NextByte) == GIF_ERROR) {
            return GIF_ERROR;
        }
        Private->CrntShiftDWord |=
           ((unsigned long)NextByte) << Private->CrntShiftState;
        Private->CrntShiftState += 8;
    }
    *Code = Private->CrntShiftDWord & CodeMasks[Private->RunningBits];

    Private->CrntShiftDWord >>= Private->RunningBits;
    Private->CrntShiftState -= Private->RunningBits;

    
    if (Private->RunningCode < LZ_MAX_CODE + 2 &&
            ++Private->RunningCode > Private->MaxCode1 &&
            Private->RunningBits < LZ_BITS) {
        Private->MaxCode1 <<= 1;
        Private->RunningBits++;
    }
    return GIF_OK;
}

static int
DGifBufferedInput(GifFileType * GifFile,
                  GifByteType * Buf,
                  GifByteType * NextByte) {

    if (Buf[0] == 0) {
        
        if (READ(GifFile, Buf, 1) != 1) {
            _GifError = D_GIF_ERR_READ_FAILED;
            return GIF_ERROR;
        }
        
        if (Buf[0] == 0) {
            _GifError = D_GIF_ERR_IMAGE_DEFECT;
            return GIF_ERROR;
        }
        if (READ(GifFile, &Buf[1], Buf[0]) != Buf[0]) {
            _GifError = D_GIF_ERR_READ_FAILED;
            return GIF_ERROR;
        }
        *NextByte = Buf[1];
        Buf[1] = 2;    
        Buf[0]--;
    } else {
        *NextByte = Buf[Buf[1]++];
        Buf[0]--;
    }

    return GIF_OK;
}
#ifndef _GBA_NO_FILEIO

int
DGifSlurp(GifFileType * GifFile) {

    int ImageSize;
    GifRecordType RecordType;
    SavedImage *sp;
    GifByteType *ExtData;
    SavedImage temp_save;

    temp_save.ExtensionBlocks = NULL;
    temp_save.ExtensionBlockCount = 0;

    do {
        if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR)
            return (GIF_ERROR);

        switch (RecordType) {
          case IMAGE_DESC_RECORD_TYPE:
              if (DGifGetImageDesc(GifFile) == GIF_ERROR)
                  return (GIF_ERROR);

              sp = &GifFile->SavedImages[GifFile->ImageCount - 1];
              ImageSize = sp->ImageDesc.Width * sp->ImageDesc.Height;

              sp->RasterBits = (unsigned char *)malloc(ImageSize *
                                                       sizeof(GifPixelType));
              if (sp->RasterBits == NULL) {
                  return GIF_ERROR;
              }
              if (DGifGetLine(GifFile, sp->RasterBits, ImageSize) ==
                  GIF_ERROR)
                  return (GIF_ERROR);
              if (temp_save.ExtensionBlocks) {
                  sp->ExtensionBlocks = temp_save.ExtensionBlocks;
                  sp->ExtensionBlockCount = temp_save.ExtensionBlockCount;

                  temp_save.ExtensionBlocks = NULL;
                  temp_save.ExtensionBlockCount = 0;

                  
                  sp->Function = sp->ExtensionBlocks[0].Function;
              }
              break;

          case EXTENSION_RECORD_TYPE:
              if (DGifGetExtension(GifFile, &temp_save.Function, &ExtData) ==
                  GIF_ERROR)
                  return (GIF_ERROR);
              while (ExtData != NULL) {

                  
                  if (AddExtensionBlock(&temp_save, ExtData[0], &ExtData[1])
                      == GIF_ERROR)
                      return (GIF_ERROR);

                  if (DGifGetExtensionNext(GifFile, &ExtData) == GIF_ERROR)
                      return (GIF_ERROR);
                  temp_save.Function = 0;
              }
              break;

          case TERMINATE_RECORD_TYPE:
              break;

          default:    
              break;
        }
    } while (RecordType != TERMINATE_RECORD_TYPE);

    
    if (temp_save.ExtensionBlocks)
        FreeExtension(&temp_save);

    return (GIF_OK);
}
#endif 
