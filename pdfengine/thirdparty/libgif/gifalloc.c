

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gif_lib.h"

#define MAX(x, y)    (((x) > (y)) ? (x) : (y))

int
BitSize(int n) {
    
    register int i;

    for (i = 1; i <= 8; i++)
        if ((1 << i) >= n)
            break;
    return (i);
}

ColorMapObject *
MakeMapObject(int ColorCount,
              const GifColorType * ColorMap) {
    
    ColorMapObject *Object;

    
    if (ColorCount != (1 << BitSize(ColorCount))) {
        return ((ColorMapObject *) NULL);
    }
    
    Object = (ColorMapObject *)malloc(sizeof(ColorMapObject));
    if (Object == (ColorMapObject *) NULL) {
        return ((ColorMapObject *) NULL);
    }

    Object->Colors = (GifColorType *)calloc(ColorCount, sizeof(GifColorType));
    if (Object->Colors == (GifColorType *) NULL) {
        return ((ColorMapObject *) NULL);
    }

    Object->ColorCount = ColorCount;
    Object->BitsPerPixel = BitSize(ColorCount);

    if (ColorMap) {
        memcpy((char *)Object->Colors,
               (char *)ColorMap, ColorCount * sizeof(GifColorType));
    }

    return (Object);
}

void
FreeMapObject(ColorMapObject * Object) {

    if (Object != NULL) {
        free(Object->Colors);
        free(Object);
        
    }
}

#ifdef DEBUG
void
DumpColorMap(ColorMapObject * Object,
             FILE * fp) {

    if (Object) {
        int i, j, Len = Object->ColorCount;

        for (i = 0; i < Len; i += 4) {
            for (j = 0; j < 4 && j < Len; j++) {
                fprintf(fp, "%3d: %02x %02x %02x   ", i + j,
                        Object->Colors[i + j].Red,
                        Object->Colors[i + j].Green,
                        Object->Colors[i + j].Blue);
            }
            fprintf(fp, "\n");
        }
    }
}
#endif 

ColorMapObject *
UnionColorMap(const ColorMapObject * ColorIn1,
              const ColorMapObject * ColorIn2,
              GifPixelType ColorTransIn2[]) {

    int i, j, CrntSlot, RoundUpTo, NewBitSize;
    ColorMapObject *ColorUnion;

    
    ColorUnion = MakeMapObject(MAX(ColorIn1->ColorCount,
                               ColorIn2->ColorCount) * 2, NULL);

    if (ColorUnion == NULL)
        return (NULL);

    
    
    for (i = 0; i < ColorIn1->ColorCount; i++)
        ColorUnion->Colors[i] = ColorIn1->Colors[i];
    CrntSlot = ColorIn1->ColorCount;

    
    while (ColorIn1->Colors[CrntSlot - 1].Red == 0
           && ColorIn1->Colors[CrntSlot - 1].Green == 0
           && ColorIn1->Colors[CrntSlot - 1].Blue == 0)
        CrntSlot--;

    
    for (i = 0; i < ColorIn2->ColorCount && CrntSlot <= 256; i++) {
        
        
        for (j = 0; j < ColorIn1->ColorCount; j++)
            if (memcmp (&ColorIn1->Colors[j], &ColorIn2->Colors[i], 
                        sizeof(GifColorType)) == 0)
                break;

        if (j < ColorIn1->ColorCount)
            ColorTransIn2[i] = j;    
        else {
            
            ColorUnion->Colors[CrntSlot] = ColorIn2->Colors[i];
            ColorTransIn2[i] = CrntSlot++;
        }
    }

    if (CrntSlot > 256) {
        FreeMapObject(ColorUnion);
        return ((ColorMapObject *) NULL);
    }

    NewBitSize = BitSize(CrntSlot);
    RoundUpTo = (1 << NewBitSize);

    if (RoundUpTo != ColorUnion->ColorCount) {
        register GifColorType *Map = ColorUnion->Colors;

        
        for (j = CrntSlot; j < RoundUpTo; j++)
            Map[j].Red = Map[j].Green = Map[j].Blue = 0;

        
        if (RoundUpTo < ColorUnion->ColorCount)
            ColorUnion->Colors = (GifColorType *)realloc(Map,
                                 sizeof(GifColorType) * RoundUpTo);
    }

    ColorUnion->ColorCount = RoundUpTo;
    ColorUnion->BitsPerPixel = NewBitSize;

    return (ColorUnion);
}

void
ApplyTranslation(SavedImage * Image,
                 GifPixelType Translation[]) {

    register int i;
    register int RasterSize = Image->ImageDesc.Height * Image->ImageDesc.Width;

    for (i = 0; i < RasterSize; i++)
        Image->RasterBits[i] = Translation[Image->RasterBits[i]];
}

void
MakeExtension(SavedImage * New,
              int Function) {

    New->Function = Function;
    
}

int
AddExtensionBlock(SavedImage * New,
                  int Len,
                  unsigned char ExtData[]) {

    ExtensionBlock *ep;

    if (New->ExtensionBlocks == NULL)
        New->ExtensionBlocks=(ExtensionBlock *)malloc(sizeof(ExtensionBlock));
    else
        New->ExtensionBlocks = (ExtensionBlock *)realloc(New->ExtensionBlocks,
                                      sizeof(ExtensionBlock) *
                                      (New->ExtensionBlockCount + 1));

    if (New->ExtensionBlocks == NULL)
        return (GIF_ERROR);

    ep = &New->ExtensionBlocks[New->ExtensionBlockCount++];

    ep->ByteCount=Len;
    ep->Bytes = (char *)malloc(ep->ByteCount);
    if (ep->Bytes == NULL)
        return (GIF_ERROR);

    if (ExtData) {
        memcpy(ep->Bytes, ExtData, Len);
        ep->Function = New->Function;
    }

    return (GIF_OK);
}

void
FreeExtension(SavedImage * Image)
{
    ExtensionBlock *ep;

    if ((Image == NULL) || (Image->ExtensionBlocks == NULL)) {
        return;
    }
    for (ep = Image->ExtensionBlocks;
         ep < (Image->ExtensionBlocks + Image->ExtensionBlockCount); ep++)
        (void)free((char *)ep->Bytes);
    free((char *)Image->ExtensionBlocks);
    Image->ExtensionBlocks = NULL;
}

void
FreeLastSavedImage(GifFileType *GifFile) {

    SavedImage *sp;
    
    if ((GifFile == NULL) || (GifFile->SavedImages == NULL))
        return;

    
    GifFile->ImageCount--;
    sp = &GifFile->SavedImages[GifFile->ImageCount];

    
    if (sp->ImageDesc.ColorMap) {
        FreeMapObject(sp->ImageDesc.ColorMap);
        sp->ImageDesc.ColorMap = NULL;
    }

    
    if (sp->RasterBits)
        free((char *)sp->RasterBits);

    
    if (sp->ExtensionBlocks)
        FreeExtension(sp);

    
}

SavedImage *
MakeSavedImage(GifFileType * GifFile,
               const SavedImage * CopyFrom) {

    SavedImage *sp;

    if (GifFile->SavedImages == NULL)
        GifFile->SavedImages = (SavedImage *)malloc(sizeof(SavedImage));
    else
        GifFile->SavedImages = (SavedImage *)realloc(GifFile->SavedImages,
                               sizeof(SavedImage) * (GifFile->ImageCount + 1));

    if (GifFile->SavedImages == NULL)
        return ((SavedImage *)NULL);
    else {
        sp = &GifFile->SavedImages[GifFile->ImageCount++];
        memset((char *)sp, '\0', sizeof(SavedImage));

        if (CopyFrom) {
            memcpy((char *)sp, CopyFrom, sizeof(SavedImage));

            

            
            if (sp->ImageDesc.ColorMap) {
                sp->ImageDesc.ColorMap = MakeMapObject(
                                         CopyFrom->ImageDesc.ColorMap->ColorCount,
                                         CopyFrom->ImageDesc.ColorMap->Colors);
                if (sp->ImageDesc.ColorMap == NULL) {
                    FreeLastSavedImage(GifFile);
                    return (SavedImage *)(NULL);
                }
            }

            
            sp->RasterBits = (unsigned char *)malloc(sizeof(GifPixelType) *
                                                   CopyFrom->ImageDesc.Height *
                                                   CopyFrom->ImageDesc.Width);
            if (sp->RasterBits == NULL) {
                FreeLastSavedImage(GifFile);
                return (SavedImage *)(NULL);
            }
            memcpy(sp->RasterBits, CopyFrom->RasterBits,
                   sizeof(GifPixelType) * CopyFrom->ImageDesc.Height *
                   CopyFrom->ImageDesc.Width);

            
            if (sp->ExtensionBlocks) {
                sp->ExtensionBlocks = (ExtensionBlock *)malloc(
                                      sizeof(ExtensionBlock) *
                                      CopyFrom->ExtensionBlockCount);
                if (sp->ExtensionBlocks == NULL) {
                    FreeLastSavedImage(GifFile);
                    return (SavedImage *)(NULL);
                }
                memcpy(sp->ExtensionBlocks, CopyFrom->ExtensionBlocks,
                       sizeof(ExtensionBlock) * CopyFrom->ExtensionBlockCount);

                
            }
        }

        return (sp);
    }
}

void
FreeSavedImages(GifFileType * GifFile) {

    SavedImage *sp;

    if ((GifFile == NULL) || (GifFile->SavedImages == NULL)) {
        return;
    }
    for (sp = GifFile->SavedImages;
         sp < GifFile->SavedImages + GifFile->ImageCount; sp++) {
        if (sp->ImageDesc.ColorMap) {
            FreeMapObject(sp->ImageDesc.ColorMap);
            sp->ImageDesc.ColorMap = NULL;
        }

        if (sp->RasterBits)
            free((char *)sp->RasterBits);

        if (sp->ExtensionBlocks)
            FreeExtension(sp);
    }
    free((char *)GifFile->SavedImages);
    GifFile->SavedImages=NULL;
}
