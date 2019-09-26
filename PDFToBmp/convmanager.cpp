/***************************************************************************************/
/* Routine for Thread managing of Conversion 
/* creator & modifier : 
/* create: 2012.09.10 ~
/* Working: 2012.09.10 ~
/***************************************************************************************/

#include "convmanager.h"
#include <memory.h>

//ConvManager* g_ConvManager = NULL;

DWORD WINAPI CONVERT_THREAD( LPVOID lpParam ) 
{
	Sleep(1000);

	ConvManager* pManager = (ConvManager*)lpParam;
	if (pManager != NULL)
		pManager->run();

	return 0; 
} 

ConvManager::ConvManager()
	: m_engine(L"")
{
	m_convertHandle = NULL;
	m_convResult = CRS_SUCCESSED;
}

bool ConvManager::convert(ConvFileInfo &fileInfo, ConvOptions &options)
{
	//g_ConvManager = this;

	if (m_convertHandle != NULL)
		return false;

	m_convFile = fileInfo;
	m_convOptions =  options;

	m_convResult = CRS_STARTED;
	m_convertHandle = CreateThread( NULL, 0, 
		CONVERT_THREAD, this, 0, NULL);

	if ( m_convertHandle == NULL)
		CloseHandle(m_convertHandle);

	return true;
}

void ConvManager::stop()
{
    if (m_convertHandle != NULL)
        m_engine.setStopFlag();
}

void ConvManager::run()
{
	m_convResult = m_engine.convertPDF(&m_convFile, &m_convOptions);
	m_convertHandle = NULL;
}

int ConvManager::getPDFPageCount(wchar_t *srcPath)
{
	return m_engine.getPDFPageCount(srcPath);
}

bool ConvManager::isRunning()
{
	return (m_convertHandle != NULL);
}

bool ConvManager::WaitForConverting(bool bexit)
{
	if (m_convertHandle == NULL)
		return true;

	if (bexit)
		m_engine.setStopFlag();

	DWORD dwResult = WaitForSingleObject(m_convertHandle, 1000);
	if (dwResult == 0)
		return true;

	return false;
}

int ConvManager::getConvPercent() 
{
	float fPercentTotal = 0.0;

	//if (g_ConvEngine == NULL)
	//	return 0;

	int nThreadCnt = m_engine.getThreadCount();
	if (nThreadCnt <= 0)
		return 0;

	for (int i = 0; i < nThreadCnt; i++)
		fPercentTotal += g_fConvPercent[i];

	return (int)fPercentTotal / nThreadCnt;
}