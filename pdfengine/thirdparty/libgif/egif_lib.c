

#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "gif_lib.h"
#include "gif_lib_private.h"

static GifPixelType CodeMask[] = {
    0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff
};

static char GifVersionPrefix[GIF_STAMP_LEN + 1] = GIF87_STAMP;

#define WRITE(_gif,_buf,_len)   \
  (((GifFilePrivateType*)_gif->Private)->Write ?    \
   ((GifFilePrivateType*)_gif->Private)->Write(_gif,_buf,_len) :    \
   fwrite(_buf, 1, _len, ((GifFilePrivateType*)_gif->Private)->File))

static int EGifPutWord(int Word, GifFileType * GifFile);
static int EGifSetupCompress(GifFileType * GifFile);
static int EGifCompressLine(GifFileType * GifFile, GifPixelType * Line,
                            int LineLen);
static int EGifCompressOutput(GifFileType * GifFile, int Code);
static int EGifBufferedOutput(GifFileType * GifFile, GifByteType * Buf,
                              int c);

GifFileType *
EGifOpenFileName(const wchar_t *FileName,
                 int TestExistance) {

    int FileHandle;
    GifFileType *GifFile;

    if (TestExistance)
        FileHandle = _wopen(FileName, O_WRONLY | O_CREAT | O_EXCL | O_BINARY, _S_IREAD | _S_IWRITE);
    else
        FileHandle = _wopen(FileName, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, _S_IREAD | _S_IWRITE);

    if (FileHandle == -1) {
        _GifError = E_GIF_ERR_OPEN_FAILED;
        return NULL;
    }
    GifFile = EGifOpenFileHandle(FileHandle);
    if (GifFile == (GifFileType *) NULL)
        _close(FileHandle);
    return GifFile;
}

GifFileType *
EGifOpenFileHandle(int FileHandle) {

    GifFileType *GifFile;
    GifFilePrivateType *Private;
    FILE *f;

    GifFile = (GifFileType *) malloc(sizeof(GifFileType));
    if (GifFile == NULL) {
        _GifError = E_GIF_ERR_NOT_ENOUGH_MEM;
        return NULL;
    }

    memset(GifFile, '\0', sizeof(GifFileType));

    Private = (GifFilePrivateType *)malloc(sizeof(GifFilePrivateType));
    if (Private == NULL) {
        free(GifFile);
        _GifError = E_GIF_ERR_NOT_ENOUGH_MEM;
        return NULL;
    }
    if ((Private->HashTable = _InitHashTable()) == NULL) {
        free(GifFile);
        free(Private);
        _GifError = E_GIF_ERR_NOT_ENOUGH_MEM;
        return NULL;
    }

#if defined(__MSDOS__) || defined(WINDOWS32) || defined(_OPEN_BINARY)
    setmode(FileHandle, O_BINARY);    
#endif 

    f = _fdopen(FileHandle, "wb");    

#if defined (__MSDOS__) || defined(WINDOWS32)
    setvbuf(f, NULL, _IOFBF, GIF_FILE_BUFFER_SIZE);    
#endif 

    GifFile->Private = (VoidPtr)Private;
    Private->FileHandle = FileHandle;
    Private->File = f;
    Private->FileState = FILE_STATE_WRITE;

    Private->Write = (OutputFunc) 0;    
    GifFile->UserData = (VoidPtr) 0;    

    _GifError = 0;

    return GifFile;
}

GifFileType *
EGifOpen(void *userData,
         OutputFunc writeFunc) {

    GifFileType *GifFile;
    GifFilePrivateType *Private;

    GifFile = (GifFileType *)malloc(sizeof(GifFileType));
    if (GifFile == NULL) {
        _GifError = E_GIF_ERR_NOT_ENOUGH_MEM;
        return NULL;
    }

    memset(GifFile, '\0', sizeof(GifFileType));

    Private = (GifFilePrivateType *)malloc(sizeof(GifFilePrivateType));
    if (Private == NULL) {
        free(GifFile);
        _GifError = E_GIF_ERR_NOT_ENOUGH_MEM;
        return NULL;
    }

    Private->HashTable = _InitHashTable();
    if (Private->HashTable == NULL) {
        free (GifFile);
        free (Private);
        _GifError = E_GIF_ERR_NOT_ENOUGH_MEM;
        return NULL;
    }

    GifFile->Private = (VoidPtr) Private;
    Private->FileHandle = 0;
    Private->File = (FILE *) 0;
    Private->FileState = FILE_STATE_WRITE;

    Private->Write = writeFunc;    
    GifFile->UserData = userData;    

    _GifError = 0;

    return GifFile;
}

void
EGifSetGifVersion(const char *Version) {
    strncpy(GifVersionPrefix + GIF_VERSION_POS, Version, 3);
}

int
EGifPutScreenDesc(GifFileType * GifFile,
                  int Width,
                  int Height,
                  int ColorRes,
                  int BackGround,
                  const ColorMapObject * ColorMap) {

    int i;
    GifByteType Buf[3];
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (Private->FileState & FILE_STATE_SCREEN) {
        
        _GifError = E_GIF_ERR_HAS_SCRN_DSCR;
        return GIF_ERROR;
    }
    if (!IS_WRITEABLE(Private)) {
        
        _GifError = E_GIF_ERR_NOT_WRITEABLE;
        return GIF_ERROR;
    }

#ifndef DEBUG_NO_PREFIX
    if (WRITE(GifFile, (unsigned char *)GifVersionPrefix,
              strlen(GifVersionPrefix)) != strlen(GifVersionPrefix)) {
        _GifError = E_GIF_ERR_WRITE_FAILED;
        return GIF_ERROR;
    }
#endif 

    GifFile->SWidth = Width;
    GifFile->SHeight = Height;
    GifFile->SColorResolution = ColorRes;
    GifFile->SBackGroundColor = BackGround;
    if (ColorMap) {
        GifFile->SColorMap = MakeMapObject(ColorMap->ColorCount,
                                           ColorMap->Colors);
        if (GifFile->SColorMap == NULL) {
            _GifError = E_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }
    } else
        GifFile->SColorMap = NULL;

    
    
    EGifPutWord(Width, GifFile);
    EGifPutWord(Height, GifFile);

    
    
    Buf[0] = (ColorMap ? 0x80 : 0x00) | 
             ((ColorRes - 1) << 4) | 
        (ColorMap ? ColorMap->BitsPerPixel - 1 : 0x07 ); 
    Buf[1] = BackGround;    
    Buf[2] = 0;             
#ifndef DEBUG_NO_PREFIX
    WRITE(GifFile, Buf, 3);
#endif 

    
#ifndef DEBUG_NO_PREFIX
    if (ColorMap != NULL)
        for (i = 0; i < ColorMap->ColorCount; i++) {
            
            Buf[0] = ColorMap->Colors[i].Red;
            Buf[1] = ColorMap->Colors[i].Green;
            Buf[2] = ColorMap->Colors[i].Blue;
            if (WRITE(GifFile, Buf, 3) != 3) {
                _GifError = E_GIF_ERR_WRITE_FAILED;
                return GIF_ERROR;
            }
        }
#endif 

    
    Private->FileState |= FILE_STATE_SCREEN;

    return GIF_OK;
}

int
EGifPutImageDesc(GifFileType * GifFile,
                 int Left,
                 int Top,
                 int Width,
                 int Height,
                 int Interlace,
                 const ColorMapObject * ColorMap) {

    int i;
    GifByteType Buf[3];
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (Private->FileState & FILE_STATE_IMAGE &&
#if defined(__MSDOS__) || defined(WINDOWS32) || defined(__GNUC__)
        Private->PixelCount > 0xffff0000UL) {
#else
        Private->PixelCount > 0xffff0000) {
#endif 
        
        _GifError = E_GIF_ERR_HAS_IMAG_DSCR;
        return GIF_ERROR;
    }
    if (!IS_WRITEABLE(Private)) {
        
        _GifError = E_GIF_ERR_NOT_WRITEABLE;
        return GIF_ERROR;
    }
    GifFile->Image.Left = Left;
    GifFile->Image.Top = Top;
    GifFile->Image.Width = Width;
    GifFile->Image.Height = Height;
    GifFile->Image.Interlace = Interlace;
    if (ColorMap) {
        GifFile->Image.ColorMap = MakeMapObject(ColorMap->ColorCount,
                                                ColorMap->Colors);
        if (GifFile->Image.ColorMap == NULL) {
            _GifError = E_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }
    } else {
        GifFile->Image.ColorMap = NULL;
    }

    
    Buf[0] = ',';    
#ifndef DEBUG_NO_PREFIX
    WRITE(GifFile, Buf, 1);
#endif 
    EGifPutWord(Left, GifFile);
    EGifPutWord(Top, GifFile);
    EGifPutWord(Width, GifFile);
    EGifPutWord(Height, GifFile);
    Buf[0] = (ColorMap ? 0x80 : 0x00) |
       (Interlace ? 0x40 : 0x00) |
       (ColorMap ? ColorMap->BitsPerPixel - 1 : 0);
#ifndef DEBUG_NO_PREFIX
    WRITE(GifFile, Buf, 1);
#endif 

    
#ifndef DEBUG_NO_PREFIX
    if (ColorMap != NULL)
        for (i = 0; i < ColorMap->ColorCount; i++) {
            
            Buf[0] = ColorMap->Colors[i].Red;
            Buf[1] = ColorMap->Colors[i].Green;
            Buf[2] = ColorMap->Colors[i].Blue;
            if (WRITE(GifFile, Buf, 3) != 3) {
                _GifError = E_GIF_ERR_WRITE_FAILED;
                return GIF_ERROR;
            }
        }
#endif 
    if (GifFile->SColorMap == NULL && GifFile->Image.ColorMap == NULL) {
        _GifError = E_GIF_ERR_NO_COLOR_MAP;
        return GIF_ERROR;
    }

    
    Private->FileState |= FILE_STATE_IMAGE;
    Private->PixelCount = (long)Width *(long)Height;

    EGifSetupCompress(GifFile);    

    return GIF_OK;
}

int
EGifPutLine(GifFileType * GifFile,
            GifPixelType * Line,
            int LineLen) {

    int i;
    GifPixelType Mask;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_WRITEABLE(Private)) {
        
        _GifError = E_GIF_ERR_NOT_WRITEABLE;
        return GIF_ERROR;
    }

    if (!LineLen)
        LineLen = GifFile->Image.Width;
    if (Private->PixelCount < (unsigned)LineLen) {
        _GifError = E_GIF_ERR_DATA_TOO_BIG;
        return GIF_ERROR;
    }
    Private->PixelCount -= LineLen;

    
    Mask = CodeMask[Private->BitsPerPixel];
    for (i = 0; i < LineLen; i++)
        Line[i] &= Mask;

    return EGifCompressLine(GifFile, Line, LineLen);
}

int
EGifPutPixel(GifFileType * GifFile,
             GifPixelType Pixel) {

    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_WRITEABLE(Private)) {
        
        _GifError = E_GIF_ERR_NOT_WRITEABLE;
        return GIF_ERROR;
    }

    if (Private->PixelCount == 0) {
        _GifError = E_GIF_ERR_DATA_TOO_BIG;
        return GIF_ERROR;
    }
    --Private->PixelCount;

    
    Pixel &= CodeMask[Private->BitsPerPixel];

    return EGifCompressLine(GifFile, &Pixel, 1);
}

int
EGifPutComment(GifFileType * GifFile,
               const char *Comment) {
  
    unsigned int length = strlen(Comment);
    char *buf;

    length = strlen(Comment);
    if (length <= 255) {
        return EGifPutExtension(GifFile, COMMENT_EXT_FUNC_CODE,
                                length, Comment);
    } else {
        buf = (char *)Comment;
        if (EGifPutExtensionFirst(GifFile, COMMENT_EXT_FUNC_CODE, 255, buf)
                == GIF_ERROR) {
            return GIF_ERROR;
        }
        length -= 255;
        buf = buf + 255;

        
        while (length > 255) {
            if (EGifPutExtensionNext(GifFile, 0, 255, buf) == GIF_ERROR) {
                return GIF_ERROR;
            }
            buf = buf + 255;
            length -= 255;
        }
        
        if (length > 0) {
            if (EGifPutExtensionLast(GifFile, 0, length, buf) == GIF_ERROR) {
                return GIF_ERROR;
            }
        } else {
            if (EGifPutExtensionLast(GifFile, 0, 0, NULL) == GIF_ERROR) {
                return GIF_ERROR;
            }
        }
    }
    return GIF_OK;
}

int
EGifPutExtensionFirst(GifFileType * GifFile,
                      int ExtCode,
                      int ExtLen,
                      const VoidPtr Extension) {

    GifByteType Buf[3];
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_WRITEABLE(Private)) {
        
        _GifError = E_GIF_ERR_NOT_WRITEABLE;
        return GIF_ERROR;
    }

    if (ExtCode == 0) {
        WRITE(GifFile, (GifByteType *)&ExtLen, 1);
    } else {
        Buf[0] = '!';
        Buf[1] = ExtCode;
        Buf[2] = ExtLen;
        WRITE(GifFile, Buf, 3);
    }

    WRITE(GifFile, Extension, ExtLen);

    return GIF_OK;
}

int
EGifPutExtensionNext(GifFileType * GifFile,
                     int ExtCode,
                     int ExtLen,
                     const VoidPtr Extension) {

    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_WRITEABLE(Private)) {
        
        _GifError = E_GIF_ERR_NOT_WRITEABLE;
        return GIF_ERROR;
    }

    Buf = ExtLen;
    WRITE(GifFile, &Buf, 1);
    WRITE(GifFile, Extension, ExtLen);

    return GIF_OK;
}

int
EGifPutExtensionLast(GifFileType * GifFile,
                     int ExtCode,
                     int ExtLen,
                     const VoidPtr Extension) {

    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_WRITEABLE(Private)) {
        
        _GifError = E_GIF_ERR_NOT_WRITEABLE;
        return GIF_ERROR;
    }

    
    if (ExtLen > 0) {
        Buf = ExtLen;
        WRITE(GifFile, &Buf, 1);
        WRITE(GifFile, Extension, ExtLen);
    }

    
    Buf = 0;
    WRITE(GifFile, &Buf, 1);

    return GIF_OK;
}

int
EGifPutExtension(GifFileType * GifFile,
                 int ExtCode,
                 int ExtLen,
                 const VoidPtr Extension) {

    GifByteType Buf[3];
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_WRITEABLE(Private)) {
        
        _GifError = E_GIF_ERR_NOT_WRITEABLE;
        return GIF_ERROR;
    }

    if (ExtCode == 0)
        WRITE(GifFile, (GifByteType *)&ExtLen, 1);
    else {
        Buf[0] = '!';       
        Buf[1] = ExtCode;   
        Buf[2] = ExtLen;    
        WRITE(GifFile, Buf, 3);
    }
    WRITE(GifFile, Extension, ExtLen);
    Buf[0] = 0;
    WRITE(GifFile, Buf, 1);

    return GIF_OK;
}

int
EGifPutCode(GifFileType * GifFile,
            int CodeSize,
            const GifByteType * CodeBlock) {

    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_WRITEABLE(Private)) {
        
        _GifError = E_GIF_ERR_NOT_WRITEABLE;
        return GIF_ERROR;
    }

    
    

    return EGifPutCodeNext(GifFile, CodeBlock);
}

int
EGifPutCodeNext(GifFileType * GifFile,
                const GifByteType * CodeBlock) {

    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (CodeBlock != NULL) {
        if (WRITE(GifFile, CodeBlock, CodeBlock[0] + 1)
               != (unsigned)(CodeBlock[0] + 1)) {
            _GifError = E_GIF_ERR_WRITE_FAILED;
            return GIF_ERROR;
        }
    } else {
        Buf = 0;
        if (WRITE(GifFile, &Buf, 1) != 1) {
            _GifError = E_GIF_ERR_WRITE_FAILED;
            return GIF_ERROR;
        }
        Private->PixelCount = 0;    
    }

    return GIF_OK;
}

int
EGifCloseFile(GifFileType * GifFile) {

    GifByteType Buf;
    GifFilePrivateType *Private;
    FILE *File;

    if (GifFile == NULL)
        return GIF_ERROR;

    Private = (GifFilePrivateType *) GifFile->Private;
    if (!IS_WRITEABLE(Private)) {
        
        _GifError = E_GIF_ERR_NOT_WRITEABLE;
        return GIF_ERROR;
    }

    File = Private->File;

    Buf = ';';
    WRITE(GifFile, &Buf, 1);

    if (GifFile->Image.ColorMap) {
        FreeMapObject(GifFile->Image.ColorMap);
        GifFile->Image.ColorMap = NULL;
    }
    if (GifFile->SColorMap) {
        FreeMapObject(GifFile->SColorMap);
        GifFile->SColorMap = NULL;
    }
    if (Private) {
        if (Private->HashTable) {
            free((char *) Private->HashTable);
        }
	    free((char *) Private);
    }
    free(GifFile);

    if (File && fclose(File) != 0) {
        _GifError = E_GIF_ERR_CLOSE_FAILED;
        return GIF_ERROR;
    }
    return GIF_OK;
}

static int
EGifPutWord(int Word,
            GifFileType * GifFile) {

    unsigned char c[2];

    c[0] = Word & 0xff;
    c[1] = (Word >> 8) & 0xff;
#ifndef DEBUG_NO_PREFIX
    if (WRITE(GifFile, c, 2) == 2)
        return GIF_OK;
    else
        return GIF_ERROR;
#else
    return GIF_OK;
#endif 
}

static int
EGifSetupCompress(GifFileType * GifFile) {

    int BitsPerPixel;
    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    
    if (GifFile->Image.ColorMap)
        BitsPerPixel = GifFile->Image.ColorMap->BitsPerPixel;
    else if (GifFile->SColorMap)
        BitsPerPixel = GifFile->SColorMap->BitsPerPixel;
    else {
        _GifError = E_GIF_ERR_NO_COLOR_MAP;
        return GIF_ERROR;
    }

    Buf = BitsPerPixel = (BitsPerPixel < 2 ? 2 : BitsPerPixel);
    WRITE(GifFile, &Buf, 1);    

    Private->Buf[0] = 0;    
    Private->BitsPerPixel = BitsPerPixel;
    Private->ClearCode = (1 << BitsPerPixel);
    Private->EOFCode = Private->ClearCode + 1;
    Private->RunningCode = Private->EOFCode + 1;
    Private->RunningBits = BitsPerPixel + 1;    
    Private->MaxCode1 = 1 << Private->RunningBits;    
    Private->CrntCode = FIRST_CODE;    
    Private->CrntShiftState = 0;    
    Private->CrntShiftDWord = 0;

   
    _ClearHashTable(Private->HashTable);

    if (EGifCompressOutput(GifFile, Private->ClearCode) == GIF_ERROR) {
        _GifError = E_GIF_ERR_DISK_IS_FULL;
        return GIF_ERROR;
    }
    return GIF_OK;
}

static int
EGifCompressLine(GifFileType * GifFile,
                 GifPixelType * Line,
                 int LineLen) {

    int i = 0, CrntCode, NewCode;
    unsigned long NewKey;
    GifPixelType Pixel;
    GifHashTableType *HashTable;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    HashTable = Private->HashTable;

    if (Private->CrntCode == FIRST_CODE)    
        CrntCode = Line[i++];
    else
        CrntCode = Private->CrntCode;    

    while (i < LineLen) {   
        Pixel = Line[i++];  
        
        NewKey = (((UINT32) CrntCode) << 8) + Pixel;
        if ((NewCode = _ExistsHashTable(HashTable, NewKey)) >= 0) {
            
            CrntCode = NewCode;
        } else {
            
            if (EGifCompressOutput(GifFile, CrntCode) == GIF_ERROR) {
                _GifError = E_GIF_ERR_DISK_IS_FULL;
                return GIF_ERROR;
            }
            CrntCode = Pixel;

            
            if (Private->RunningCode >= LZ_MAX_CODE) {
                
                if (EGifCompressOutput(GifFile, Private->ClearCode)
                        == GIF_ERROR) {
                    _GifError = E_GIF_ERR_DISK_IS_FULL;
                    return GIF_ERROR;
                }
                Private->RunningCode = Private->EOFCode + 1;
                Private->RunningBits = Private->BitsPerPixel + 1;
                Private->MaxCode1 = 1 << Private->RunningBits;
                _ClearHashTable(HashTable);
            } else {
                
                _InsertHashTable(HashTable, NewKey, Private->RunningCode++);
            }
        }

    }

    
    Private->CrntCode = CrntCode;

    if (Private->PixelCount == 0) {
        
        if (EGifCompressOutput(GifFile, CrntCode) == GIF_ERROR) {
            _GifError = E_GIF_ERR_DISK_IS_FULL;
            return GIF_ERROR;
        }
        if (EGifCompressOutput(GifFile, Private->EOFCode) == GIF_ERROR) {
            _GifError = E_GIF_ERR_DISK_IS_FULL;
            return GIF_ERROR;
        }
        if (EGifCompressOutput(GifFile, FLUSH_OUTPUT) == GIF_ERROR) {
            _GifError = E_GIF_ERR_DISK_IS_FULL;
            return GIF_ERROR;
        }
    }

    return GIF_OK;
}

static int
EGifCompressOutput(GifFileType * GifFile,
                   int Code) {

    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;
    int retval = GIF_OK;

    if (Code == FLUSH_OUTPUT) {
        while (Private->CrntShiftState > 0) {
            
            if (EGifBufferedOutput(GifFile, Private->Buf,
                                 Private->CrntShiftDWord & 0xff) == GIF_ERROR)
                retval = GIF_ERROR;
            Private->CrntShiftDWord >>= 8;
            Private->CrntShiftState -= 8;
        }
        Private->CrntShiftState = 0;    
        if (EGifBufferedOutput(GifFile, Private->Buf,
                               FLUSH_OUTPUT) == GIF_ERROR)
            retval = GIF_ERROR;
    } else {
        Private->CrntShiftDWord |= ((long)Code) << Private->CrntShiftState;
        Private->CrntShiftState += Private->RunningBits;
        while (Private->CrntShiftState >= 8) {
            
            if (EGifBufferedOutput(GifFile, Private->Buf,
                                 Private->CrntShiftDWord & 0xff) == GIF_ERROR)
                retval = GIF_ERROR;
            Private->CrntShiftDWord >>= 8;
            Private->CrntShiftState -= 8;
        }
    }

    
    
    if (Private->RunningCode >= Private->MaxCode1 && Code <= 4095) {
       Private->MaxCode1 = 1 << ++Private->RunningBits;
    }

    return retval;
}

static int
EGifBufferedOutput(GifFileType * GifFile,
                   GifByteType * Buf,
                   int c) {

    if (c == FLUSH_OUTPUT) {
        
        if (Buf[0] != 0
            && WRITE(GifFile, Buf, Buf[0] + 1) != (unsigned)(Buf[0] + 1)) {
            _GifError = E_GIF_ERR_WRITE_FAILED;
            return GIF_ERROR;
        }
        
        Buf[0] = 0;
        if (WRITE(GifFile, Buf, 1) != 1) {
            _GifError = E_GIF_ERR_WRITE_FAILED;
            return GIF_ERROR;
        }
    } else {
        if (Buf[0] == 255) {
            
            if (WRITE(GifFile, Buf, Buf[0] + 1) != (unsigned)(Buf[0] + 1)) {
                _GifError = E_GIF_ERR_WRITE_FAILED;
                return GIF_ERROR;
            }
            Buf[0] = 0;
        }
        Buf[++Buf[0]] = c;
    }

    return GIF_OK;
}

int
EGifSpew(GifFileType * GifFileOut) {

    int i, j, gif89 = FALSE;
    int bOff;   
    char SavedStamp[GIF_STAMP_LEN + 1];

    for (i = 0; i < GifFileOut->ImageCount; i++) {
        for (j = 0; j < GifFileOut->SavedImages[i].ExtensionBlockCount; j++) {
            int function =
               GifFileOut->SavedImages[i].ExtensionBlocks[j].Function;

            if (function == COMMENT_EXT_FUNC_CODE
                || function == GRAPHICS_EXT_FUNC_CODE
                || function == PLAINTEXT_EXT_FUNC_CODE
                || function == APPLICATION_EXT_FUNC_CODE)
                gif89 = TRUE;
        }
    }

    strncpy(SavedStamp, GifVersionPrefix, GIF_STAMP_LEN);
    if (gif89) {
        strncpy(GifVersionPrefix, GIF89_STAMP, GIF_STAMP_LEN);
    } else {
        strncpy(GifVersionPrefix, GIF87_STAMP, GIF_STAMP_LEN);
    }
    if (EGifPutScreenDesc(GifFileOut,
                          GifFileOut->SWidth,
                          GifFileOut->SHeight,
                          GifFileOut->SColorResolution,
                          GifFileOut->SBackGroundColor,
                          GifFileOut->SColorMap) == GIF_ERROR) {
        strncpy(GifVersionPrefix, SavedStamp, GIF_STAMP_LEN);
        return (GIF_ERROR);
    }
    strncpy(GifVersionPrefix, SavedStamp, GIF_STAMP_LEN);

    for (i = 0; i < GifFileOut->ImageCount; i++) {
        SavedImage *sp = &GifFileOut->SavedImages[i];
        int SavedHeight = sp->ImageDesc.Height;
        int SavedWidth = sp->ImageDesc.Width;
        ExtensionBlock *ep;

        
        if (sp->RasterBits == NULL)
            continue;

        if (sp->ExtensionBlocks) {
            for (j = 0; j < sp->ExtensionBlockCount; j++) {
                ep = &sp->ExtensionBlocks[j];
                if (j == sp->ExtensionBlockCount - 1 || (ep+1)->Function != 0) {
                    
                    if (EGifPutExtension(GifFileOut,
                                         (ep->Function != 0) ? ep->Function : '\0',
                                         ep->ByteCount,
                                         ep->Bytes) == GIF_ERROR) {
                        return (GIF_ERROR);
                    }
                } else {
                    EGifPutExtensionFirst(GifFileOut, ep->Function, ep->ByteCount, ep->Bytes);
                    for (bOff = j+1; bOff < sp->ExtensionBlockCount; bOff++) {
                        ep = &sp->ExtensionBlocks[bOff];
                        if (ep->Function != 0) {
                            break;
                        }
                        EGifPutExtensionNext(GifFileOut, 0,
                                ep->ByteCount, ep->Bytes);
                    }
                    EGifPutExtensionLast(GifFileOut, 0, 0, NULL);
                    j = bOff-1;
                }
            }
        }

        if (EGifPutImageDesc(GifFileOut,
                             sp->ImageDesc.Left,
                             sp->ImageDesc.Top,
                             SavedWidth,
                             SavedHeight,
                             sp->ImageDesc.Interlace,
                             sp->ImageDesc.ColorMap) == GIF_ERROR)
            return (GIF_ERROR);

        for (j = 0; j < SavedHeight; j++) {
            if (EGifPutLine(GifFileOut,
                            sp->RasterBits + j * SavedWidth,
                            SavedWidth) == GIF_ERROR)
                return (GIF_ERROR);
        }
    }

    if (EGifCloseFile(GifFileOut) == GIF_ERROR)
        return (GIF_ERROR);

    return (GIF_OK);
}
