#ifndef __DEFINE_H__
#define __DEFINE_H__

#include "../pdfengine/common/convbase.h"

#define WM_CLIENT_NOTIFY WM_USER + 1

enum
{
	CLIENT_MSG_CONNECTED = 0,
	CLIENT_MSG_RECEIVE,
	CLIENT_MSG_SEND,
	CLIENT_MSG_STATE,
	CLIENT_MSG_DISCONNECTED,
	CLIENT_MSG_SETINFO,
	CLIENT_MSG_TYPE_COUNT,
};

typedef struct _ST_CLIENT_INFO
{
	wchar_t clientAddr[MAX_NAME_SIZE];
	ConvFileInfo convInfo;
	ConvOptions convOpts;
} CLIENT_INFO;

#endif