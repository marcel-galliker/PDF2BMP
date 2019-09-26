#ifndef CONVMANAGER_H
#define CONVMANAGER_H

#include <Windows.h>
#include "../pdfengine/common/convbase.h"
#include "../convengine/convengine.h"

class ConvManager
{
public:
	ConvManager();

	bool convert(ConvFileInfo &convFiles, ConvOptions &options);
    void stop();
    int getPDFPageCount(wchar_t *srcPath);
	int getConvResult() { return m_convResult; }
	bool isRunning();

    void run();
	bool WaitForConverting(bool bexit);

	int getConvPercent();

private:
	ConvFileInfo	m_convFile;
	ConvOptions		m_convOptions;
	ConvEngine		m_engine;
	int				m_convResult;

	HANDLE			m_convertHandle;
};

#endif // CONVMANAGER_H
