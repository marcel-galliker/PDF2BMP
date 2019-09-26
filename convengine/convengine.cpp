/***************************************************************************************/
/* Main Routine of Convengine Project 
/* creator & modifier : 
/* create: 2012.09.10 ~
/* Working: 2012.09.10 ~
/***************************************************************************************/

#include <direct.h>
#include <string.h>
#include <algorithm>
#include <Windows.h>
#include "convengine.h"

extern "C"
{
#include "pdf-internal.h"
#include "pdfengine-internal.h"
};

#define PREFIX_BMP_NAME		L"BMP"

bool g_bStopThread = false;

float g_fConvPercent[MAX_THREAD_COUNT] = {0.0};
float g_fConvPercentMin[MAX_THREAD_COUNT] = {0.0};
float g_fConvPercentMax[MAX_THREAD_COUNT] = {0.0};

#define DEFAULT_BMP_TOTAL_SIZE	0xE4E1C0
#define DEFAULT_BMP_LINE_SIZE	0x600

//ConvEngine* g_ConvEngine = NULL;

DWORD WINAPI CONVERT_PAGE_THREAD( LPVOID lpParam ) 
{
	LPST_CONV_PAGE paramPtr = (LPST_CONV_PAGE)lpParam;
	if (paramPtr->classPtr != NULL)
	{
		((ConvEngine*)(paramPtr->classPtr))->convertPageFromThread(paramPtr);
	}

	return 0; 
} 

static bool compareTextInfo(const TEXT_INFO &info1, const TEXT_INFO &info2)
{
	if (info1.box.left < info2.box.left)
		return true;
	else if ((info1.box.left == info2.box.left) && (info1.box.top < info2.box.top))
		return true;
	return false;
}


typedef pdf_cmap_table* (*LoadCMapFunction)(int*);

ConvEngine::ConvEngine(wchar_t* path)
{
	//g_ConvEngine = this;

	m_nBaseHeight = 0;
	m_nMaxFontSize = 0;

	wcscpy(m_strRunPath, path);

	loadCMapDlls(); // call only once

	for (int i = 0; i < MAX_THREAD_COUNT; i++)
	{
		m_fp[i] = NULL;

		m_sPageBBox[i].left = 0, m_sPageBBox[i].top = 0;
		m_sPageBBox[i].right = 0, m_sPageBBox[i].bottom = 0;

		m_pPDFCtx[i] = NULL;//base_new_context(NULL, NULL, base_STORE_DEFAULT);

		m_pPDFDoc[i] = NULL;

		m_pPDFDevice[i] = NULL;
		m_pPDFImage[i] = NULL;
		m_pPDFDisplayList[i] = NULL;
		m_pPDFPage[i] = NULL;

		m_pPtrBmpTotalBuffer[i] = NULL;//(unsigned char*)malloc(sizeof(unsigned char) * DEFAULT_BMP_TOTAL_SIZE);
		m_pPtrBmpLineBuffer[i] = NULL;//(unsigned char*)malloc(sizeof(unsigned char) * DEFAULT_BMP_LINE_SIZE);

		m_nBmpTotalBufSize[i] = 0;//DEFAULT_BMP_TOTAL_SIZE;
		m_nBmpLineBufsize[i] = 0;//DEFAULT_BMP_LINE_SIZE;
	}
}

ConvEngine::~ConvEngine()
{
	for (int i = 0; i < MAX_THREAD_COUNT; i++)
	{
		if (NULL != m_fp[i])
		{
			fclose(m_fp[i]);
			m_fp[i] = NULL;
		}

		if (NULL != m_pPtrBmpTotalBuffer[i])
		{
			free(m_pPtrBmpTotalBuffer[i]);
			m_pPtrBmpTotalBuffer[i] = NULL;
		}

		if (NULL != m_pPtrBmpLineBuffer[i])
		{
			free(m_pPtrBmpLineBuffer[i]);
			m_pPtrBmpLineBuffer[i] = NULL;
		}

		releasePageData(i);
	}
}

/***************************************************************************************/
/* function name:	setStopFlag
/* description:		set the flag to stop the conversion thread
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void ConvEngine::setStopFlag()
{ 
	g_bStopThread = true; 
}

/***************************************************************************************/
/* function name:	convertPDF
/* description:		main PDF conversion function, which performs PDF conversion according to
/*												the conversion options
/* param 1:			information of the conversion source file
/* param 2:			conversion options information
/* return:			CONVERT_RESULT_STATUS enum type
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

int ConvEngine::convertPDF(ConvFileInfo *pdfFile, ConvOptions *convOpts)
{
	int page_count = 0;
	int result = CRS_SUCCESSED;

	g_bStopThread = false;

	g_page_list.clear();

	page_count = getPageList(pdfFile);
	if (page_count <= 0)
		return 0;

	m_nCurThreadCnt = convOpts->threadCnt;
	if (m_nCurThreadCnt > MAX_THREAD_COUNT)
		m_nCurThreadCnt = MAX_THREAD_COUNT;

	if (m_nCurThreadCnt <= 0)
		m_nCurThreadCnt = 1;

	if (page_count < m_nCurThreadCnt)
		m_nCurThreadCnt = page_count;

	if (m_nCurThreadCnt <= 0)
		return 0;

	int nPageStep = g_page_list.size() / m_nCurThreadCnt;
	for (int iThread = 0; iThread < m_nCurThreadCnt; iThread++)
	{
		ST_CONV_PAGE* thread_param = new ST_CONV_PAGE;

		thread_param->threadId = iThread;
		thread_param->startpageNumber = nPageStep * iThread;
		thread_param->endpageNumber = (iThread == m_nCurThreadCnt - 1) ? g_page_list.size(): nPageStep * (iThread + 1);
		thread_param->pdfFile = pdfFile;
		thread_param->convOpts = convOpts;
		thread_param->classPtr = this;

		m_result[iThread] = CRS_SUCCESSED;
		g_fConvPercent[iThread] = 0;
		g_fConvPercentMin[iThread] = 0;
		g_fConvPercentMax[iThread] = 100;

		m_pThreadHandle[iThread] = CreateThread(NULL, 0, CONVERT_PAGE_THREAD, thread_param, 0, NULL);
	}

	WaitForMultipleObjects(m_nCurThreadCnt, m_pThreadHandle, TRUE, INFINITE);

	for (int iThread = 0; iThread < m_nCurThreadCnt; iThread++)
	{
		g_fConvPercent[iThread] = 0;//100.0;
	}

	if (g_bStopThread)
		result = CRS_STOPED;
	else
	{
		for (int iThread = 0; iThread < m_nCurThreadCnt; iThread++)
		{
			if (m_result[iThread] != CRS_SUCCESSED)
			{
				result = m_result[iThread];
				break;
			}
		}
	}

	return result;
}

void ConvEngine::convertPageFromThread(LPST_CONV_PAGE convParam)
{
	if (g_bStopThread)
	{
		m_result[convParam->threadId] = CRS_STOPED;
		return;
	}

	if (m_pPDFCtx[convParam->threadId] == NULL)
		m_pPDFCtx[convParam->threadId] = base_new_context(NULL, NULL, base_STORE_DEFAULT);

	if (m_pPDFCtx[convParam->threadId] == NULL)
	{
		m_result[convParam->threadId] = CRS_CONVERSION_ERROR;
		return;
	}

	if (!openPDF(convParam->threadId, convParam->pdfFile->srcPath))
	{
		closePDF(convParam->threadId);
		m_result[convParam->threadId] = CRS_NOTEXISTFILE_ERROR;
		return;
	}
	
	m_nBaseHeight = 0;
	unsigned char nLinearBuf[256];
	int nErrorBuf[256 * 4];
	unsigned char nNewPixelBuf[256];

	float X[5]={0,25,50,75,100};
	float Xdiff_float[5];
	float Ydiff_float[5];
	int i = 0;

	if (convParam->convOpts->convertmode == 0)
	{
		for (i = 0; i < 5; i++)
		{
			if (i != 0)
			{
				Xdiff_float[i] = X[i] - X[i - 1];
				Ydiff_float[i] = convParam->convOpts->pY[i] - convParam->convOpts->pY[i - 1];
			}
		}

		for (i = 0; i <= 255; i++)
		{
			float result = 0;
			float pFloat = (255 - i) * 100.0 / 255;

			if (pFloat < X[1])
			{
				result = (convParam->convOpts->pY[0] + (pFloat - X[0]) * Ydiff_float[1] / Xdiff_float[1]) * 255.0 / 100;
			}
			else if (pFloat < X[2])
			{
				result = (convParam->convOpts->pY[1] + (pFloat - X[1]) * Ydiff_float[2] / Xdiff_float[2]) * 255.0 / 100;
			}
			else if (pFloat < X[3])
			{
				result = (convParam->convOpts->pY[2] + (pFloat - X[2]) * Ydiff_float[3] / Xdiff_float[3]) * 255.0 / 100;
			}
			else if (pFloat < X[4])
			{
				result = (convParam->convOpts->pY[3] + (pFloat - X[3]) * Ydiff_float[4] / Xdiff_float[4]) * 255.0 / 100;
			}
			else
			{
				result = convParam->convOpts->pY[4] * 255 / 100;
			}

			nLinearBuf[i] = (unsigned char)(255 - result);

			nNewPixelBuf[i] = 255;
			if ((i >> 7) == 0)
				nNewPixelBuf[i] = 0;

			int error = i - nNewPixelBuf[i];
			nErrorBuf[i * 4] = error / 16;
			nErrorBuf[i * 4 + 1] = error * 3 / 16;
			nErrorBuf[i * 4 + 2] = error * 5 / 16;
			nErrorBuf[i * 4 + 3] = error * 7 / 16;
		}
	}

	base_try(((base_context *)m_pPDFCtx[convParam->threadId]))
	{
		for (int i = convParam->startpageNumber; i < convParam->endpageNumber; i++)
		{
			if (g_bStopThread)
			{
				m_result[convParam->threadId] = CRS_STOPED;
				break;
			}

			g_fConvPercentMin[convParam->threadId] = 90 * (i - convParam->startpageNumber) / (convParam->endpageNumber - convParam->startpageNumber);
			g_fConvPercentMax[convParam->threadId] = 90 * (i - convParam->startpageNumber + 1) / (convParam->endpageNumber - convParam->startpageNumber);

			if ( !convertPage(convParam->pdfFile, g_page_list[i], convParam->convOpts, convParam->threadId, nLinearBuf, nNewPixelBuf, nErrorBuf) )
				base_throw((base_context *)m_pPDFCtx[convParam->threadId], "convert error at %d page", g_page_list[i]);

			g_fConvPercentMin[convParam->threadId] = g_fConvPercentMax[convParam->threadId];
		}
	}
	base_catch(((base_context *)m_pPDFCtx[convParam->threadId]))
	{
		//pdfapp_error(app, "cannot open document");
		m_result[convParam->threadId] = CRS_CONVERSION_ERROR;
	}

	if (!g_bStopThread)
	{
		g_fConvPercentMin[convParam->threadId] = 90.0;
		g_fConvPercentMax[convParam->threadId] = 100.0;
	}

	closePDF(convParam->threadId);

	delete convParam;
}

/***************************************************************************************/
/* function name:	getPDFPageCount
/* description:		get the page count of the PDF file
/* param 1:			path of the PDF file
/* param 2:			password string for the PDF document opening when it is password-protected
/* param 3:			determine whether the PDF document is password-protected
/* return:			page count of PDF document
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

int ConvEngine::getPDFPageCount(wchar_t *srcPath)
{
	int pageCount = 0;

	m_pPDFCtx[0] = base_new_context(NULL, NULL, base_STORE_DEFAULT);
	if (m_pPDFCtx[0] == NULL)
	{
		return -1;
	}

	if (!openPDF(0, srcPath))
	{
		closePDF(0);
		return -1;
	}

	base_try(((base_context *)m_pPDFCtx[0]))
	{
		pageCount = base_count_pages((base_document *)m_pPDFDoc[0]);
	}
	base_catch(((base_context *)m_pPDFCtx[0]))
	{
		pageCount = -1;
	}

	closePDF(0);

	return pageCount;
}

bool ConvEngine::convertPage(ConvFileInfo *pdfFile, int pageNo, ConvOptions *convOpts, int threadId, unsigned char* pLinearBuf, unsigned char* pNewPixelBuf, int* pErrorBuf)
{
	try
	{
		return PDF2Image(pdfFile, pageNo, convOpts, threadId, pLinearBuf, pNewPixelBuf, pErrorBuf);
	}
	catch (...)
	{
		return false;
	}

	return false;
}

#define STEP_GENERATETABLE_PERCENT	0.8

bool ConvEngine::PDF2Image(ConvFileInfo *pdfFile, int pageno, ConvOptions *convOpts, int threadId, unsigned char* pLinearBuf, unsigned char* pNewPixelBuf, int* pErrorBuf)
{
	int ret;

	float fConvPercentMin = g_fConvPercentMin[threadId];
	float fConvPercentMax = g_fConvPercentMax[threadId];

	wchar_t name[MAX_NAME_SIZE];

	float fResolutionX = PDF_RESOLUTION_600, fResolutionY = PDF_RESOLUTION_600;
	float sizeWidth = 0, sizeHeight = 0;

	if (convOpts->resolutionX > 0)
		fResolutionX = convOpts->resolutionX;

	if (convOpts->resolutionY > 0)
		fResolutionY = convOpts->resolutionY;

	if (convOpts->sizeWidth > 0){
		sizeWidth = convOpts->sizeWidth * fResolutionX;
		int sizi = sizeWidth;
		sizi = (sizi+31)&(~31);
		sizeWidth = sizi;
	}

	if (convOpts->sizeHeight > 0)
		sizeHeight = convOpts->sizeHeight * fResolutionY;

	if (!extractPageData(pageno, 
						fResolutionX, 
						fResolutionY,
						sizeWidth,
						sizeHeight,
						threadId))
	{
		releasePageData(threadId);

		g_fConvPercent[threadId] = g_fConvPercentMax[threadId] = fConvPercentMax;
		return false;
	}
		
	g_fConvPercent[threadId] = fConvPercentMin + (fConvPercentMax - fConvPercentMin) * STEP_GENERATETABLE_PERCENT;

	//swprintf(name, L"%s\\%s-%09d.bmp", pdfFile->dstPath, pdfFile->fileName, pdfFile->startNumber + (pageno - pdfFile->startPageNumber));
	swprintf(name, L"%s\\%s-%09d.bmp", pdfFile->dstPath, PREFIX_BMP_NAME, pdfFile->startNumber + (pageno - pdfFile->startPageNumber));

	ret=base_write_bmp((base_context *)m_pPDFCtx[threadId], (base_pixmap *)m_pPDFImage[threadId], name, &m_pPtrBmpTotalBuffer[threadId], &m_nBmpTotalBufSize[threadId], &m_pPtrBmpLineBuffer[threadId], &m_nBmpLineBufsize[threadId], pLinearBuf, pNewPixelBuf, pErrorBuf, convOpts->convertmode, convOpts->convertsplit);
	if ( ret != 0){
		if ( ret == 1)
			m_result[threadId] = CRS_NOTEXISTFILE_ERROR;
		else
			m_result[threadId] = CRS_CONVERSION_ERROR;
	}
	releasePageData(threadId);

	g_fConvPercent[threadId] = g_fConvPercentMax[threadId] = fConvPercentMax;
	return true;
}

void ConvEngine::loadCMapDlls()
{
	wchar_t strCMapPath[1024];
	wcscpy(strCMapPath, m_strRunPath);
	wcscat(strCMapPath, L"/cmap/");
	
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	BOOL blNext = TRUE;
	hFind = FindFirstFile(L"*.dll", &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) 
		return;

	while (hFind != INVALID_HANDLE_VALUE && blNext == TRUE)
	{
		wchar_t* firstStr = wcstok(FindFileData.cFileName, L"_");
		if (firstStr != NULL)
		{
			wchar_t fontfilename[1024];
			wcscpy(fontfilename, strCMapPath);
			wcscat(fontfilename, FindFileData.cFileName);

			HINSTANCE hInst = LoadLibrary(fontfilename);
			if (hInst != NULL)
			{
				pdf_cmap_table *cmap_table = NULL;
				LoadCMapFunction loadFonts = (LoadCMapFunction)GetProcAddress(hInst, "loadFonts");
				int fontcount = 0;

				if (loadFonts)
					cmap_table = loadFonts(&fontcount);

				for (int j = 0; j < fontcount; j++)
				{
					cmap_table->cmap->storable.free = pdf_free_cmap_imp;
					g_customCMapTable[g_customCMapTableSize+j] = cmap_table[j];
				}

				g_customCMapTableSize += fontcount;
			}
		}

		blNext = FindNextFile(hFind, &FindFileData);
	}

	FindClose(hFind);
}

bool CompareObj( unsigned int first, unsigned int second )
{
	return first < second;
}

int ConvEngine::getPageList(ConvFileInfo *pdfFile)
{
	int page, spage, epage;
	char *spec, *dash;
	char *range = pdfFile->selectPages;

	spec = base_strsep(&range, ",");
	while (spec)
	{
		if (strlen(spec) == strlen("All") && strcmp(spec, "All") == 0)
		{
			spage = 1;
			epage = pdfFile->pageCount;
		}
		else
		{
			dash = strchr(spec, '-');

			if (dash == spec)
				spage = epage = pdfFile->pageCount;
			else
				spage = epage = atoi(spec);

			if (dash)
			{
				if (strlen(dash) > 1)
					epage = atoi(dash + 1);
				else
					epage = pdfFile->pageCount;
			}
		}

		if (spage < 1 || spage > pdfFile->pageCount)
			return 0;
		if (epage < 1 || epage > pdfFile->pageCount)
			return 0;

		if (spage < epage)
			for (page = spage; page <= epage; page++)
			{
				bool bExist = false;
				for (int i = 0; i < (int)g_page_list.size(); i++)
				{
					if (g_page_list[i] == page)
					{
						bExist = true;
						break;
					}
				}

				if (bExist == false)
				{
					g_page_list.push_back( page );
				}
			}
		else
			for (page = spage; page >= epage; page--)
			{
				bool bExist = false;
				for (int i = 0; i < (int)g_page_list.size(); i++)
				{
					if (g_page_list[i] == page)
					{
						bExist = true;
						break;
					}
				}

				if (bExist == false)
				{
					g_page_list.push_back( page );
				}
			}

			spec = base_strsep(&range, ",");
	}

	sort(g_page_list.begin(), g_page_list.end(), CompareObj);

	return g_page_list.size();
}

bool ConvEngine::openPDF(int threadId, wchar_t *file_path)
{
	bool bResult = false;

	if (NULL == m_pPDFCtx[threadId])
		return false;

	base_try(((base_context *)m_pPDFCtx[threadId]))
	{
		base_try(((base_context *)m_pPDFCtx[threadId]))
		{
			m_pPDFDoc[threadId] = base_open_document((base_context *)m_pPDFCtx[threadId], file_path);
		}
		base_catch(((base_context *)m_pPDFCtx[threadId]))
		{
			base_throw((base_context *)m_pPDFCtx[threadId], "cannot open document: %s", file_path);
		}

		if (base_needs_password((base_document *)m_pPDFDoc[threadId]))
		{
			if (!base_authenticate_password((base_document *)m_pPDFDoc[threadId]))
				base_throw((base_context *)m_pPDFCtx[threadId], "cannot authenticate password: %s", file_path);
		}
		bResult = true;
	}
	base_catch(((base_context *)m_pPDFCtx[threadId]))
	{
		bResult = false;
	}

	return bResult;
}

void ConvEngine::closePDF(int threadId)
{
	if (m_pPDFDoc[threadId])
	{
		base_close_document((base_document *)m_pPDFDoc[threadId]);
		m_pPDFDoc[threadId] = NULL;
	}

	if (m_pPDFCtx[threadId])
	{
		base_free_context((base_context *)m_pPDFCtx[threadId]);
		m_pPDFCtx[threadId] = NULL;
	}
}

bool ConvEngine::extractPageData(int pageno, 
								 float fResolutionX, 
								 float fResolutionY,
								 float sizeWidth,
								 float sizeHeight,
								 int threadId)
{
	bool bResult = false;

	base_rect page_bbox;
	base_matrix ctm;
	base_bbox bbox;

	base_try(((base_context *)m_pPDFCtx[threadId]))
	{
		m_pPDFPage[threadId] = base_load_page((base_document *)m_pPDFDoc[threadId], pageno - 1);
		page_bbox = base_bound_page((base_document *)m_pPDFDoc[threadId], (base_page *)m_pPDFPage[threadId]);

		if (sizeWidth > 0 && page_bbox.x1 - page_bbox.x0 != 0)
			fResolutionX = sizeWidth * 72 / (page_bbox.x1 - page_bbox.x0);

		if (sizeHeight > 0 && page_bbox.y1 - page_bbox.y0 != 0)
			fResolutionY = sizeHeight * 72 / (page_bbox.y1 - page_bbox.y0);

		ctm = base_scale(fResolutionX/72.0f, fResolutionY/72.0f);
		ctm = base_concat(ctm, base_rotate(0));

		bbox = base_round_rect(base_transform_rect(ctm, page_bbox));

		if (m_sPageBBox[threadId].left != bbox.x0
			|| m_sPageBBox[threadId].top != bbox.y0
			|| m_sPageBBox[threadId].right != bbox.x1 
			||m_sPageBBox[threadId].bottom != bbox.y1)
		{
			if (m_pPDFImage[threadId] != NULL)
			{
				base_drop_pixmap((base_context *)m_pPDFCtx[threadId], (base_pixmap *)m_pPDFImage[threadId]);
				m_pPDFImage[threadId] = NULL;
			}
		}

		m_sPageBBox[threadId].left = bbox.x0, m_sPageBBox[threadId].top = bbox.y0;
		m_sPageBBox[threadId].right = bbox.x1, m_sPageBBox[threadId].bottom = bbox.y1;
		if (m_pPDFImage[threadId] == NULL)
			m_pPDFImage[threadId] = base_new_pixmap_with_bbox((base_context *)m_pPDFCtx[threadId], base_device_gray/*base_device_rgb*/, bbox);

		base_clear_pixmap_with_value((base_context *)m_pPDFCtx[threadId], (base_pixmap *)m_pPDFImage[threadId], 255);

/*		m_pPDFDisplayList = base_new_display_list((base_context *)m_pPDFCtx);
		m_pPDFDevice = base_new_list_device((base_context *)m_pPDFCtx, (base_display_list *)m_pPDFDisplayList);
		base_run_page((base_document *)m_pPDFDoc, (base_page *)m_pPDFPage, (base_device *)m_pPDFDevice, base_identity, NULL);
		base_free_device((base_device *)m_pPDFDevice);
		m_pPDFDevice = NULL;
*/	

		m_pPDFDevice[threadId] = base_new_draw_device((base_context *)m_pPDFCtx[threadId], (base_pixmap *)m_pPDFImage[threadId]);
		((base_device *)(m_pPDFDevice[threadId]))->flags |= base_DEVFLAG_ONLYIMAGE;

		base_run_page((base_document *)m_pPDFDoc[threadId], (base_page *)m_pPDFPage[threadId], (base_device *)m_pPDFDevice[threadId], ctm, NULL);

		if (g_bStopThread)
			base_throw((base_context *)m_pPDFCtx[threadId], "extractPageData stoped.");

		bResult = true;
	}
	base_catch(((base_context *)m_pPDFCtx[threadId]))
	{
		bResult = false;
	}

	return bResult;
}

void ConvEngine::releasePageData(int threadId)
{
	if (m_pPDFDevice[threadId])
	{
		base_free_device((base_device *)m_pPDFDevice[threadId]);
		m_pPDFDevice[threadId] = NULL;
	}
	if (m_pPDFImage[threadId])
	{
		base_drop_pixmap((base_context *)m_pPDFCtx[threadId], (base_pixmap *)m_pPDFImage[threadId]);
		m_pPDFImage[threadId] = NULL;
	}
	if (m_pPDFDisplayList[threadId])
	{
		base_free_display_list((base_context *)m_pPDFCtx[threadId], (base_display_list *)m_pPDFDisplayList[threadId]);
		m_pPDFDisplayList[threadId] = NULL;
	}
	if (m_pPDFPage[threadId])
	{
		base_free_page((base_document *)m_pPDFDoc[threadId], (base_page *)m_pPDFPage[threadId]);
		m_pPDFPage[threadId] = NULL;
	}
}