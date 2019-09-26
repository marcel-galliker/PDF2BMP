

#ifndef _GIF_LIB_H_
#define _GIF_LIB_H_ 1

#ifdef __cplusplus
extern "C" {
#endif 

#define GIF_LIB_VERSION " Version 4.1, "

#define GIF_ERROR   0
#define GIF_OK      1

#ifndef TRUE
#define TRUE        1
#endif 
#ifndef FALSE
#define FALSE       0
#endif 

#ifndef NULL
#define NULL        0
#endif 

#define GIF_STAMP "GIFVER"          
#define GIF_STAMP_LEN sizeof(GIF_STAMP) - 1
#define GIF_VERSION_POS 3           
#define GIF87_STAMP "GIF87a"        
#define GIF89_STAMP "GIF89a"        

#define GIF_FILE_BUFFER_SIZE 16384  

typedef unsigned int UINT32;
typedef int GifBooleanType;
typedef unsigned char GifPixelType;
typedef unsigned char *GifRowType;
typedef unsigned char GifByteType;
#ifdef _GBA_OPTMEM
    typedef unsigned short GifPrefixType;
    typedef short GifWord;
#else
    typedef unsigned int GifPrefixType;
    typedef int GifWord;
#endif

#define GIF_MESSAGE(Msg) fprintf(stderr, "\n%s: %s\n", PROGRAM_NAME, Msg)
#define GIF_EXIT(Msg)    { GIF_MESSAGE(Msg); exit(-3); }

#ifdef SYSV
#define VoidPtr char *
#else
#define VoidPtr void *
#endif 

typedef struct GifColorType {
    GifByteType Red, Green, Blue;
} GifColorType;

typedef struct ColorMapObject {
    int ColorCount;
    int BitsPerPixel;
    GifColorType *Colors;    
} ColorMapObject;

typedef struct GifImageDesc {
    GifWord Left, Top, Width, Height,   
      Interlace;                    
    ColorMapObject *ColorMap;       
} GifImageDesc;

typedef struct GifFileType {
    GifWord SWidth, SHeight,        
      SColorResolution,         
      SBackGroundColor;         
    ColorMapObject *SColorMap;  
    int ImageCount;             
    GifImageDesc Image;         
    struct SavedImage *SavedImages; 
    VoidPtr UserData;           
    VoidPtr Private;            
} GifFileType;

typedef enum {
    UNDEFINED_RECORD_TYPE,
    SCREEN_DESC_RECORD_TYPE,
    IMAGE_DESC_RECORD_TYPE, 
    EXTENSION_RECORD_TYPE,  
    TERMINATE_RECORD_TYPE   
} GifRecordType;

typedef enum {
    GIF_DUMP_SGI_WINDOW = 1000,
    GIF_DUMP_X_WINDOW = 1001
} GifScreenDumpType;

typedef int (*InputFunc) (GifFileType *, GifByteType *, int);

typedef int (*OutputFunc) (GifFileType *, const GifByteType *, int);

#define COMMENT_EXT_FUNC_CODE     0xfe    
#define GRAPHICS_EXT_FUNC_CODE    0xf9    
#define PLAINTEXT_EXT_FUNC_CODE   0x01    
#define APPLICATION_EXT_FUNC_CODE 0xff    

GifFileType *EGifOpenFileName(const wchar_t *GifFileName,
                              int GifTestExistance);
GifFileType *EGifOpenFileHandle(int GifFileHandle);
GifFileType *EGifOpen(void *userPtr, OutputFunc writeFunc);

int EGifSpew(GifFileType * GifFile);
void EGifSetGifVersion(const char *Version);
int EGifPutScreenDesc(GifFileType * GifFile,
                      int GifWidth, int GifHeight, int GifColorRes,
                      int GifBackGround,
                      const ColorMapObject * GifColorMap);
int EGifPutImageDesc(GifFileType * GifFile, int GifLeft, int GifTop,
                     int Width, int GifHeight, int GifInterlace,
                     const ColorMapObject * GifColorMap);
int EGifPutLine(GifFileType * GifFile, GifPixelType * GifLine,
                int GifLineLen);
int EGifPutPixel(GifFileType * GifFile, GifPixelType GifPixel);
int EGifPutComment(GifFileType * GifFile, const char *GifComment);
int EGifPutExtensionFirst(GifFileType * GifFile, int GifExtCode,
                          int GifExtLen, const VoidPtr GifExtension);
int EGifPutExtensionNext(GifFileType * GifFile, int GifExtCode,
                         int GifExtLen, const VoidPtr GifExtension);
int EGifPutExtensionLast(GifFileType * GifFile, int GifExtCode,
                         int GifExtLen, const VoidPtr GifExtension);
int EGifPutExtension(GifFileType * GifFile, int GifExtCode, int GifExtLen,
                     const VoidPtr GifExtension);
int EGifPutCode(GifFileType * GifFile, int GifCodeSize,
                const GifByteType * GifCodeBlock);
int EGifPutCodeNext(GifFileType * GifFile,
                    const GifByteType * GifCodeBlock);
int EGifCloseFile(GifFileType * GifFile);

#define E_GIF_ERR_OPEN_FAILED    1    
#define E_GIF_ERR_WRITE_FAILED   2
#define E_GIF_ERR_HAS_SCRN_DSCR  3
#define E_GIF_ERR_HAS_IMAG_DSCR  4
#define E_GIF_ERR_NO_COLOR_MAP   5
#define E_GIF_ERR_DATA_TOO_BIG   6
#define E_GIF_ERR_NOT_ENOUGH_MEM 7
#define E_GIF_ERR_DISK_IS_FULL   8
#define E_GIF_ERR_CLOSE_FAILED   9
#define E_GIF_ERR_NOT_WRITEABLE  10

#ifndef _GBA_NO_FILEIO
GifFileType *DGifOpenFileName(const char *GifFileName);
GifFileType *DGifOpenFileHandle(int GifFileHandle);
int DGifSlurp(GifFileType * GifFile);
#endif 
GifFileType *DGifOpen(void *userPtr, InputFunc readFunc);    
int DGifGetScreenDesc(GifFileType * GifFile);
int DGifGetRecordType(GifFileType * GifFile, GifRecordType * GifType);
int DGifGetImageDesc(GifFileType * GifFile);
int DGifGetLine(GifFileType * GifFile, GifPixelType * GifLine, int GifLineLen);
int DGifGetPixel(GifFileType * GifFile, GifPixelType GifPixel);
int DGifGetComment(GifFileType * GifFile, char *GifComment);
int DGifGetExtension(GifFileType * GifFile, int *GifExtCode,
                     GifByteType ** GifExtension);
int DGifGetExtensionNext(GifFileType * GifFile, GifByteType ** GifExtension);
int DGifGetCode(GifFileType * GifFile, int *GifCodeSize,
                GifByteType ** GifCodeBlock);
int DGifGetCodeNext(GifFileType * GifFile, GifByteType ** GifCodeBlock);
int DGifGetLZCodes(GifFileType * GifFile, int *GifCode);
int DGifCloseFile(GifFileType * GifFile);

#define D_GIF_ERR_OPEN_FAILED    101    
#define D_GIF_ERR_READ_FAILED    102
#define D_GIF_ERR_NOT_GIF_FILE   103
#define D_GIF_ERR_NO_SCRN_DSCR   104
#define D_GIF_ERR_NO_IMAG_DSCR   105
#define D_GIF_ERR_NO_COLOR_MAP   106
#define D_GIF_ERR_WRONG_RECORD   107
#define D_GIF_ERR_DATA_TOO_BIG   108
#define D_GIF_ERR_NOT_ENOUGH_MEM 109
#define D_GIF_ERR_CLOSE_FAILED   110
#define D_GIF_ERR_NOT_READABLE   111
#define D_GIF_ERR_IMAGE_DEFECT   112
#define D_GIF_ERR_EOF_TOO_SOON   113

int QuantizeBuffer(unsigned int Width, unsigned int Height,
                   int *ColorMapSize, GifByteType * RedInput,
                   GifByteType * GreenInput, GifByteType * BlueInput,
                   GifByteType * OutputBuffer,
                   GifColorType * OutputColorMap);

#ifndef _GBA_NO_FILEIO
extern void PrintGifError(void);
#endif 
extern int GifLastError(void);

extern ColorMapObject *MakeMapObject(int ColorCount,
                                     const GifColorType * ColorMap);
extern void FreeMapObject(ColorMapObject * Object);
extern ColorMapObject *UnionColorMap(const ColorMapObject * ColorIn1,
                                     const ColorMapObject * ColorIn2,
                                     GifPixelType ColorTransIn2[]);
extern int BitSize(int n);

typedef struct {
    int ByteCount;
    char *Bytes;    
    int Function;   
} ExtensionBlock;

typedef struct SavedImage {
    GifImageDesc ImageDesc;
    unsigned char *RasterBits;  
    int Function;   
    int ExtensionBlockCount;
    ExtensionBlock *ExtensionBlocks;    
} SavedImage;

extern void ApplyTranslation(SavedImage * Image, GifPixelType Translation[]);
extern void MakeExtension(SavedImage * New, int Function);
extern int AddExtensionBlock(SavedImage * New, int Len,
                             unsigned char ExtData[]);
extern void FreeExtension(SavedImage * Image);
extern SavedImage *MakeSavedImage(GifFileType * GifFile,
                                  const SavedImage * CopyFrom);
extern void FreeSavedImages(GifFileType * GifFile);

#ifdef __cplusplus
}
#endif 
#endif 
