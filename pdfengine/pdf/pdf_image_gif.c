#include "pdf-internal.h"

#include "gif_lib.h"

static void QuitGifError(GifFileType *GifFile)
{
	PrintGifError();
	if (GifFile != NULL) EGifCloseFile(GifFile);
	exit(EXIT_FAILURE);
}

static void SaveGif(wchar_t *filename,
					GifByteType *OutputBuffer,
					ColorMapObject *OutputColorMap,
					int ExpColorMapSize, int Width, int Height)
{
	int i;
	GifFileType *GifFile;
	GifByteType *Ptr = OutputBuffer;

	
	if ((GifFile = EGifOpenFileName(filename, 0)) == NULL)
		QuitGifError(GifFile);

	if (EGifPutScreenDesc(GifFile, Width, Height, ExpColorMapSize, 0, OutputColorMap) == GIF_ERROR ||
		EGifPutImageDesc(GifFile, 0, 0, Width, Height, FALSE, NULL) == GIF_ERROR)
		QuitGifError(GifFile);

	for (i = 0; i < Height; i++) {
		if (EGifPutLine(GifFile, Ptr, Width) == GIF_ERROR)
			QuitGifError(GifFile);

		Ptr += Width;
	}

	if (EGifCloseFile(GifFile) == GIF_ERROR)
		QuitGifError(GifFile);
}

/***************************************************************************************/
/* function name:	base_write_gif
/* description:		save pixmap as gif file
/* param 1:			pointer to the context
/* param 2:			pixmap to save
/* param 3:			filepath where to save
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_write_gif(base_context *ctx, base_pixmap *pixmap, wchar_t *filename)
{
	GifByteType *RedBuffer = NULL, *GreenBuffer = NULL, *BlueBuffer = NULL, *OutputBuffer = NULL;
	GifByteType *RedP, *GreenP, *BlueP, *BufP;

	ColorMapObject *OutputColorMap = NULL;
	int ExpNumOfColors = 8, ColorMapSize = 256;

	unsigned int count;

	unsigned int size = pixmap->w * pixmap->h * sizeof(GifByteType);

	if ((RedBuffer = (GifByteType *) malloc(size)) == NULL ||
		(GreenBuffer = (GifByteType *) malloc(size)) == NULL ||
		(BlueBuffer = (GifByteType *) malloc(size)) == NULL)
		base_throw(ctx, "Failed to allocate memory required, aborted.");

	RedP = RedBuffer;
	GreenP = GreenBuffer;
	BlueP = BlueBuffer;
	BufP = pixmap->samples;

	for (count = 0; count < size; count++) {
		*RedP++ = *BufP++;
		*GreenP++ = *BufP++;
		*BlueP++ = *BufP++;
                BufP++;
	}

	if ((OutputColorMap = MakeMapObject(ColorMapSize, NULL)) == NULL ||
		(OutputBuffer = (GifByteType *) malloc(pixmap->w * pixmap->h * sizeof(GifByteType))) == NULL)
		base_throw(ctx, "Failed to allocate memory required, aborted.");

	if (QuantizeBuffer(pixmap->w, pixmap->h, &ColorMapSize,
		RedBuffer, GreenBuffer, BlueBuffer,
		OutputBuffer, OutputColorMap->Colors) == GIF_ERROR)
		base_throw(ctx, "Failed quantize.");

	free(RedBuffer);
	free(GreenBuffer);
	free(BlueBuffer);

	SaveGif(filename, OutputBuffer, OutputColorMap, ExpNumOfColors, pixmap->w, pixmap->h);

	free(OutputBuffer);
}

