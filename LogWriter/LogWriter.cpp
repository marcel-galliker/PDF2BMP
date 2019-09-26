#include "stdafx.h"
#include "LogWriter.h"
#include "DateTime.h"

LogWriter::LogWriter()
{
	m_file = NULL;
	InitializeCriticalSection(&m_lock);
}

LogWriter::~LogWriter()
{
	if (m_file)
	{
		fclose(m_file);
		m_file = NULL;
	}
	DeleteCriticalSection(&m_lock);
}

void LogWriter::initLog()
{
	if (m_file) return;

// 	SHCreateDirectoryEx(NULL, inPath, NULL);

	TCHAR path[512] = {0}, inPath[512] = {0}, fnameFmt[256] = {0};

	DateTime  now = DateTime::now();
	SYSTEMTIME  st;
	now.toUtcSystemTime(&st);
	if (st.wDay < 0 || st.wDay > 31) return;
	
	GetModuleFileNameW(0, inPath, sizeof(inPath));

	TCHAR *fnd = wcsrchr(inPath, _T('\\'));

	memset(fnd, 0, sizeof(fnd));

	lstrcat(fnameFmt, _T("%s\\PDF2BMP_%d-"));

	if (st.wMonth < 10) {
		lstrcat(fnameFmt, _T("0"));
	}
	lstrcat(fnameFmt, _T("%d-"));

	if (st.wDay < 10) {
		lstrcat(fnameFmt, _T("0"));
	}
	lstrcat(fnameFmt, _T("%d.log"));
	
	_stprintf(path, fnameFmt, inPath, st.wYear, st.wMonth, st.wDay);

	m_file = NULL;
	m_file = _tfopen(path, _T("r"));
	if (m_file)
	{
		fclose(m_file);
		m_file = _tfopen(path, _T("a"));
	}
	else
	{
		m_file = _tfopen(path, _T("w"));
	}

	if (m_file)
	{
		TCHAR line_str[256], time_str[256];
		memset(line_str, 0x0, sizeof(line_str));
		memset(time_str, 0x0, sizeof(time_str));
		_stprintf(time_str, _T("%d-%d-%d %d-%d-%d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

		_ftprintf(m_file, _T("%s"), line_str);
		fflush(m_file);
	}
}

void LogWriter::log(int nlogtype, const TCHAR *fmt, ...)
{
	if (!m_file) return;
	if( nlogtype > LOG_DEBUG ) return;

	EnterCriticalSection(&m_lock);

	TCHAR msg[256], time[256], res[256];
	memset(msg, 0x0, sizeof(msg));
	memset(time, 0x0, sizeof(time));
	memset(res, 0x0, sizeof(res));

	va_list	arglist;
	va_start (arglist, fmt);
	_vsntprintf(msg, 256, fmt, arglist);
	va_end (arglist);

	DateTime currTime = DateTime::now();
	SYSTEMTIME st;
	currTime.toUtcSystemTime(&st);

	_stprintf(time, _T("%.4d-%.2d-%.2d %.2d:%.2d:%.2d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	_stprintf(res, _T("[%s]  %s   %s\n"), time, strLogSig[nlogtype], msg);

	_ftprintf(m_file, _T("%s"), res);
	fflush(m_file);

	LeaveCriticalSection(&m_lock);
}