#include "pdf-internal.h"
#include <stdio.h>
#include <process.h>


#pragma pack (1)

typedef struct RGBQUAD_s {
	unsigned char    rgbBlue;
	unsigned char    rgbGreen;
	unsigned char    rgbRed;
	unsigned char    rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFOHEADER{
	unsigned long	biSize;
	unsigned long	biWidth;
	unsigned long	biHeight;
	unsigned short	biPlanes;
	unsigned short	biBitCount;
	unsigned long	biCompression;
	unsigned long	biSizeImage;
	unsigned long	biXPelsPerMeter;
	unsigned long	biYPelsPerMeter;
	unsigned long	biClrUsed;
	unsigned long	biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER    bmiHeader;
	RGBQUAD             bmiColors[2];
} BITMAPINFO;

typedef struct tagBITMAPFILEHEADER {
	unsigned short	bfType;
	unsigned long	bfSize;
	unsigned short	bfReserved1;
	unsigned short	bfReserved2;
	unsigned long	bfOffBits;
} BITMAPFILEHEADER;

/***************************************************************************************/
/* function name:	base_write_bmp
/* description:		save pixmap as bmp file
/* param 1:			pointer to the context
/* param 2:			pixmap to save
/* param 3:			filepath where to save
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

int
base_write_bmp(base_context *ctx, base_pixmap *pixmap, wchar_t *filename, unsigned char** pBmpTotalBuffer, int* nTotalBufSize, unsigned char** pBmpLineBuffer, int* nLineBufSize, unsigned char* pLinearBuf, unsigned char* pNewPixelBuf, int* pErrorBuf, int convmode, int convsplit)
{
	BITMAPINFO bmi = {0};
	BITMAPFILEHEADER  bmpFileHeader = {0};
	FILE * outfile;
	char fname[ 256];
	char arg[ 256];
	int i, j, len, ret;
	unsigned char *p;
	unsigned char *curColor;
	unsigned char *ptrCurBuf;
	unsigned char *ptrLineBuf;

//	unsigned char *pPtrNextXPt;
	unsigned char *pPtrNextYPt;

	unsigned char *pPtrDownYPt;
	unsigned char *pPtrDownY1Pt;
	unsigned char *pPtrDownY2Pt;

	int nCurWriteSize = 0;
	int nNewBufSize = 0;

	int nOutImgWidth = pixmap->w;
	int nOutImgHeight = pixmap->h;
//	int nIndexX, nIndexY;

//	unsigned char xbitFlag, ybitFlag;

	int nBitmapPaddingWidth = 0;
	unsigned int BITMAP_THRESHOLD = 0;
	unsigned int BITMAP_THRESHOLD_MIN = 255;
	unsigned int BITMAP_THRESHOLD_MAX = 0;
	long nlevel; 
	unsigned char level;

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = nOutImgWidth;
	bmi.bmiHeader.biHeight = nOutImgHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 1;
	bmi.bmiHeader.biCompression = 0;
	bmi.bmiColors[0].rgbBlue = 0;
	bmi.bmiColors[0].rgbGreen = 0;
	bmi.bmiColors[0].rgbRed = 0;
	bmi.bmiColors[0].rgbReserved = 0;

	bmi.bmiColors[1].rgbBlue = 255;
	bmi.bmiColors[1].rgbGreen = 255;
	bmi.bmiColors[1].rgbRed = 255;
	bmi.bmiColors[1].rgbReserved = 0;

	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;

	bmi.bmiHeader.biSizeImage = ((nOutImgWidth * bmi.bmiHeader.biBitCount + 31) & ~ 31) / 8 * nOutImgHeight;

	outfile = _wfopen(filename, L"wb");
	if (outfile == NULL)
		base_throw(ctx, "File Open Error : %s", filename);

	bmpFileHeader.bfReserved1 = 0;
	bmpFileHeader.bfReserved2 = 0;
	bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmi.bmiHeader.biSizeImage + sizeof(RGBQUAD) * 2;
	bmpFileHeader.bfType = 'MB';

	bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 2;

	nNewBufSize = sizeof(unsigned char) * (((nOutImgWidth * bmi.bmiHeader.biBitCount + 31) & ~ 31) / 8 + 1) * nOutImgHeight;
	if (*pBmpTotalBuffer == NULL || *nTotalBufSize < nNewBufSize)
	{
		if (*pBmpTotalBuffer != NULL)
			free(*pBmpTotalBuffer);

		*pBmpTotalBuffer = (unsigned char*)malloc(nNewBufSize);
		*nTotalBufSize = nNewBufSize;
	}

	nNewBufSize = sizeof(unsigned char) * ((nOutImgWidth * bmi.bmiHeader.biBitCount + 31) & ~ 31) / 8 + 1;
	if (*pBmpLineBuffer == NULL || *nLineBufSize < nNewBufSize)
	{
		if (*pBmpLineBuffer != NULL)
			free(*pBmpLineBuffer);

		*pBmpLineBuffer = (unsigned char*)malloc(nNewBufSize);
		*nLineBufSize = nNewBufSize;
	}

	ptrLineBuf = *pBmpLineBuffer;
	ptrCurBuf = *pBmpTotalBuffer;
	memcpy(ptrCurBuf, &bmpFileHeader, sizeof(BITMAPFILEHEADER));
	nCurWriteSize += sizeof(BITMAPFILEHEADER);
	ptrCurBuf += sizeof(BITMAPFILEHEADER);

	memcpy(ptrCurBuf, &bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
	nCurWriteSize += sizeof(BITMAPINFOHEADER);
	ptrCurBuf += sizeof(BITMAPINFOHEADER);

	memcpy(ptrCurBuf, &bmi.bmiColors, sizeof(RGBQUAD) * 2);
	nCurWriteSize += sizeof(RGBQUAD) * 2;
	ptrCurBuf += sizeof(RGBQUAD) * 2;

	nBitmapPaddingWidth = sizeof(unsigned char) * ((((nOutImgWidth * bmi.bmiHeader.biBitCount + 31) & ~ 31) >> 3) - (nOutImgWidth >> 3));

	if (convmode == 0){
		for(i=0;i<pixmap->h-1;i++)
		{
			for(p = (unsigned char*)&pixmap->samples[i * pixmap->w * 2],j=0; j<pixmap->w-1; p+= 2,j++)
			{
				*p = pLinearBuf[*p];
			}
		}
	}

	for(i=pixmap->h ; i > 0 ; i--){
		if (i == 1)
			pPtrDownYPt=(unsigned char*)&pixmap->samples[(i-1) * pixmap->w * 2];
		else
			pPtrDownYPt=(unsigned char*)&pixmap->samples[(i-2) * pixmap->w * 2];

		for (p = (unsigned char*)&pixmap->samples[(i-1) * pixmap->w * 2], j = 0; j < pixmap->w; j++, p+= 2,pPtrDownYPt+=2)
		{
			if (convmode == 0 && ((i > 2) && (j < pixmap->w - 1)))
			{
				level=*p;
				pPtrNextYPt=p+2;
				pPtrDownY1Pt=pPtrDownYPt-2;
				pPtrDownY2Pt=pPtrDownYPt+2;

				nlevel=*pPtrNextYPt + pErrorBuf[((*p) << 2) + 3];
				level=255;
				if (nlevel > 0)
				{
					if ((nlevel >> 8) == 0)
						level = nlevel;
				}
				else
					level = 0;
						
				//level=(unsigned char)MIN(255,MAX(0,(int)nlevel));
				*pPtrNextYPt=level;
			
				nlevel=*pPtrDownYPt + pErrorBuf[((*p) << 2) + 2];
				level=255;
				if (nlevel > 0)
				{
					if ((nlevel >> 8) == 0)
						level = nlevel;
				}
				else
					level = 0;

				//level=(unsigned char)MIN(255,MAX(0,(int)nlevel));
				*pPtrDownYPt=level;
				
				nlevel=*pPtrDownY1Pt + pErrorBuf[((*p) << 2) + 1];
				level=255;
				if (nlevel > 0)
				{
					if ((nlevel >> 8) == 0)
						level = nlevel;
				}
				else
					level = 0;

				//level=(unsigned char)MIN(255,MAX(0,(int)nlevel));
				*pPtrDownY1Pt=level;
		
				nlevel=*pPtrDownY2Pt + pErrorBuf[((*p) << 2)];
				level=255;
				if (nlevel > 0)
				{
					if ((nlevel >> 8) == 0)
						level = nlevel;
				}
				else
					level = 0;

				//level=(unsigned char)MIN(255,MAX(0,(int)nlevel));
				*pPtrDownY2Pt=level;

				*p = pNewPixelBuf[*p];
									
			}
			curColor = &ptrCurBuf[j >> 3];
			*curColor = *curColor << 1;
			if (((*p) >> 7) != 0)
				*curColor |= 1;
		}
		ptrCurBuf += pixmap->w >> 3;
		nCurWriteSize += pixmap->w >> 3;
					
		if (nBitmapPaddingWidth != 0)
		{
			memset(ptrCurBuf, 0, nBitmapPaddingWidth);
			ptrCurBuf += nBitmapPaddingWidth;
			nCurWriteSize += nBitmapPaddingWidth;
		}
	}
	
	fwrite(*pBmpTotalBuffer, nCurWriteSize, 1, outfile);
	fclose(outfile);
	if ( convsplit){
		char *p_fname=fname;
		wchar_t *p_filename=filename;
		len = wcslen( filename);
		while ( len > 0){
			if (*p_filename<0x100) *p_fname=(char)*p_filename;
			else *p_fname='?';
			if (*p_filename==0) 
				break;
			p_fname++;
			p_filename++;
			len--;
		}
		*p_fname=0;
//		sprintf( arg, "-file %s", fname);
//		ret = _execl( "d:/pdf/split_bitmap.exe","d:/pdf/split_bitmap.exe", arg, NULL);
//		CreateProcess ( NULL , arg, NULL, NULL, FALSE, NULL, NULL, NULL, NULL, NULL);
		sprintf( arg, "d:/pdf/split_bitmap.exe -file %s", fname);
		ret = system( arg);
		
	}
	return 0;
}
