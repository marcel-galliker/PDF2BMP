#ifndef CONVENGINE_H
#define CONVENGINE_H

#include <stdio.h>
#include "../pdfengine/common/convbase.h"
#include <vector>

#define MAX_THREAD_COUNT	5

typedef struct _ST_CONV_PAGE
{
	int threadId;
	int startpageNumber;
	int endpageNumber;

	ConvFileInfo *pdfFile;
	ConvOptions *convOpts;

	void *classPtr;
} ST_CONV_PAGE, *LPST_CONV_PAGE;

class ConvEngine
{
public:
    ConvEngine(wchar_t* path);
    ~ConvEngine();

	int convertPDF(ConvFileInfo *pdfFile, ConvOptions *convOpts);
    int getPDFPageCount(wchar_t *srcPath);

	void setStopFlag();
	void convertPageFromThread(LPST_CONV_PAGE convParam);

	int getThreadCount() { return m_nCurThreadCnt;}

private:

	void beginConvert(ConvFileInfo *pdfFile, ConvOptions *convOpts, int threadId);
	bool endConvert(ConvFileInfo *pdfFile, int threadId);
	bool convertPage(ConvFileInfo *pdfFile, int pageNo, ConvOptions *convOpts, int threadId, unsigned char* pLinearBuf, unsigned char* pNewPixelBuf, int* pErrorBuf);
	bool PDF2Image(ConvFileInfo *pdfFile, int pageno, ConvOptions *convOpts, int threadId, unsigned char* pLinearBuf, unsigned char* pNewPixelBuf, int* pErrorBuf);

	void loadCMapDlls();
	int getPageList(ConvFileInfo *pdfFile);
	bool openPDF(int threadId, wchar_t *file_path);
	void closePDF(int threadId);
	bool extractPageData(int pageno, 
						float fResolutionX, 
						float fResolutionY,
						float sizeWidth,
						float sizeHeight,
						int threadId);
	void releasePageData(int threadId);

private:
	std::vector<unsigned int> g_page_list;

	FILE * m_fp[MAX_THREAD_COUNT];
	wchar_t m_strRunPath[1024];

	void *		m_pPDFCtx[MAX_THREAD_COUNT];			// base_context *
	void *		m_pPDFDoc[MAX_THREAD_COUNT];

	INT_RECT	m_sPageBBox[MAX_THREAD_COUNT];

	void *		m_pPDFPage[MAX_THREAD_COUNT];			// base_page *
	void *		m_pPDFDevice[MAX_THREAD_COUNT];		// base_device *
	void *		m_pPDFImage[MAX_THREAD_COUNT];		// base_pixmap *
	void *		m_pPDFDisplayList[MAX_THREAD_COUNT];	// base_display_list *

	int			m_nBaseHeight;
	int			m_nMaxFontSize;		// resolution : 72.0

	unsigned char *		m_pPtrBmpTotalBuffer[MAX_THREAD_COUNT];
	unsigned char *		m_pPtrBmpLineBuffer[MAX_THREAD_COUNT];

	int			m_nBmpTotalBufSize[MAX_THREAD_COUNT];
	int			m_nBmpLineBufsize[MAX_THREAD_COUNT];

	HANDLE		m_pThreadHandle[MAX_THREAD_COUNT];
	int			m_result[MAX_THREAD_COUNT];

	int			m_nCurThreadCnt;
};

#endif // CONVENGINE_H
