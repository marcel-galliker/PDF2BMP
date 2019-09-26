#pragma once

#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>

const TCHAR strLogSig[][256] = {  
	_T("LOG_NOTICE"), 
	_T("LOG_WARN"),
	_T("LOG_ERROR"),
	_T("LOG_INFO"),
	_T("LOG_DETAIL"),
	_T("LOG_DEBUG")   
};

enum PDF2BMP_LOGTYPE {
	LOG_NOTICE,
	LOG_WARN,
	LOG_ERROR,
	LOG_INFO,
	LOG_DETAIL,
	LOG_DEBUG,
	PDF2BMP_LOGTYPE_CNT
}; 

// static const int LOG_NOTICE = 0;
// static const int LOG_WARN = 1;
// static const int LOG_ERROR = 2;
// 
// static const int LOG_INFO = 3;
// static const int LOG_DETAIL = 4;
// static const int LOG_DEBUG = 5;

class DateTime;

class LogWriter
{
public:
	LogWriter();
	virtual ~LogWriter();

	void initLog();
	void log(int nlogtype, const TCHAR *fmt, ...);

private:
	CRITICAL_SECTION m_lock;
	FILE* m_file;
};
