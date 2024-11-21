//#include <stdio.h>	
#include "sokmanager.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

sokmanager* g_sokmanager = NULL;

#define		TRACE_LEN	250 
char trString [2][TRACE_LEN+1];
void sokmanager::TrPrintf (int level, char *format, ...)
{
	int	len;
	char *str;
	//	LARGE_INTEGER t;
	//	int	time;


	if (level>9) {
		str=trString[1]; level-=10; 
	}
	else str=trString[0];

	if (level <= Trace) {

		len = _vsnprintf(str, TRACE_LEN, format, (va_list)&format + _INTSIZEOF(format));
		if (len<0 || (len>TRACE_LEN-(int)strlen(MyName)-10)) {
			memcpy(&str[TRACE_LEN-strlen(MyName)-10], "...\n", 5);
			str[TRACE_LEN-strlen(MyName)-10+4]=0;
		}
		OutputDebugStringA( str);
	}
} /* end TrPrintf */

VOID watch_main_thread()
{
	if (g_sokmanager)
		g_sokmanager->WatchClient();
}


VOID watch_client_thread(int no)
{
	if (g_sokmanager)
		g_sokmanager->client_thread(no);
}

VOID start_convert_thread(sokmanager::Sconvert_thread_par *par)
{
	if (g_sokmanager)
		g_sokmanager->convert_thread(par);
}


VOID sokmanager::convert_thread (Sconvert_thread_par *par)
{
	int ret=0;
	wchar_t clientMsg[MAX_NAME_SIZE];
	struct SocketStruct sendMessage;	
	struct SocketStruct receiveMessage;
	memcpy(&receiveMessage, par->pReceiveMessage, sizeof(receiveMessage));

	g_sokmanager->NoPagesConv=receiveMessage.parameter;

	sendMessage.parameter	= receiveMessage.parameter;
	sendMessage.number		= receiveMessage.number;
	sendMessage.startNumber = receiveMessage.startNumber;

	if (receiveMessage.parameter > 0 && receiveMessage.number > 0)
	{
		if (receiveMessage.parameter + receiveMessage.number - 1 > par->m_curItem->pageCount)
		{
			sprintf(par->m_curItem->selectPages, "%d-%d", receiveMessage.parameter, par->m_curItem->pageCount);
			par->m_curItem->startNumber = receiveMessage.startNumber;
			par->m_curItem->startPageNumber = receiveMessage.parameter;

			SendMessage(g_sokmanager->m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SETINFO, (LPARAM)par->clientInfo);

			g_sokmanager->dwStartTime = GetTickCount();
			swprintf(clientMsg, L"%s\tConverting Started 1 (resX=%.1f, resY=%.1f width=%.2f  height=%.2f tCnt=%d)\t", par->clientAddr, par->m_options->resolutionX, par->m_options->resolutionY, par->m_options->sizeWidth, par->m_options->sizeHeight, par->m_options->threadCnt);
			SendMessage(g_sokmanager->m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_STATE, (LPARAM)clientMsg);

			/*
			m_options.pY[0] = 0;
			m_options.pY[1] = 25;
			m_options.pY[2] = 50;
			m_options.pY[3] = 75;
			m_options.pY[4] = 100;

			m_options.convertmode = 0;
			*/
			// --- values come from foreground ---
			par->m_options->pY[0] = m_convOptions.pY[0];
			par->m_options->pY[1] = m_convOptions.pY[1];
			par->m_options->pY[2] = m_convOptions.pY[2];
			par->m_options->pY[3] = m_convOptions.pY[3];
			par->m_options->pY[4] = m_convOptions.pY[4];
			par->m_options->convertmode = m_convOptions.convertmode;
			par->m_options->convertsplit = m_convOptions.convertsplit;

			par->m_options->threadCnt = m_convOptions.threadCnt; //MAX_THREAD_COUNT;
			g_sokmanager->ConvMgr[par->clientNo].convert(*par->m_curItem, *par->m_options);
			g_sokmanager->running = TRUE;
			while (TRUE)
			{
				if (!g_sokmanager->running) return;
				if (g_sokmanager->ConvMgr[par->clientNo].WaitForConverting(g_sokmanager->bExitFlag))
					break;
			}
			g_sokmanager->ConvMgr[par->clientNo].stop();

			DWORD dwElapsed = (GetTickCount() - g_sokmanager->dwStartTime) / 1000;
			int seconds = dwElapsed;
			//			strTime.Format(L"Elapsed: %.2d:%.2d", dwElapsed / 60, dwElapsed % 60);
			wsprintf(clientMsg, L"%s\tConverting Finished 1 time: %.2d:%.2d  %d p/h max.\t", par->clientAddr, dwElapsed / 60, dwElapsed % 60, 3600*(g_sokmanager->NoPagesConv/seconds));
			SendMessage(g_sokmanager->m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_STATE, (LPARAM)clientMsg);

			ret = 1;
			strcpy( (char*)sendMessage.name, "End of the file");
		}
		else
		{
			sprintf(par->m_curItem->selectPages, "%d-%d", receiveMessage.parameter, receiveMessage.number + receiveMessage.parameter - 1);
			par->m_curItem->startNumber = receiveMessage.startNumber;
			par->m_curItem->startPageNumber = receiveMessage.parameter;

			if ( m_convOptions.sizeWidth > 0)
				par->m_options->sizeWidth = m_convOptions.sizeWidth;
			if ( m_convOptions.sizeHeight > 0)
				par->m_options->sizeHeight = m_convOptions.sizeHeight;

			par->clientInfo->convInfo = *par->m_curItem;
			par->clientInfo->convOpts = *par->m_options;
			SendMessage(g_sokmanager->m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SETINFO, (LPARAM)par->clientInfo);

			g_sokmanager->dwStartTime = GetTickCount();
			swprintf(clientMsg, L"%s\tConverting Started 2 (resX=%.1f, resY=%.1f width=%.2f  height=%.2f tCnt=%d)\t", par->clientAddr, par->m_options->resolutionX, par->m_options->resolutionY, par->m_options->sizeWidth, par->m_options->sizeHeight, par->m_options->threadCnt);
			SendMessage(g_sokmanager->m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_STATE, (LPARAM)clientMsg);

			/*						m_options.pY[0] = 0;
			m_options.pY[1] = 25;
			m_options.pY[2] = 50;
			m_options.pY[3] = 75;
			m_options.pY[4] = 100;
			m_options.convertmode = 0;
			*/
			// --- values come from foreground ---
			par->m_options->pY[0] = m_convOptions.pY[0];
			par->m_options->pY[1] = m_convOptions.pY[1];
			par->m_options->pY[2] = m_convOptions.pY[2];
			par->m_options->pY[3] = m_convOptions.pY[3];
			par->m_options->pY[4] = m_convOptions.pY[4];
			par->m_options->convertmode = m_convOptions.convertmode;
			par->m_options->convertsplit = m_convOptions.convertsplit;

			par->m_options->threadCnt = m_convOptions.threadCnt; //MAX_THREAD_COUNT;
			//						m_options.threadCnt = MAX_THREAD_COUNT;
			g_sokmanager->ConvMgr[par->clientNo].convert(*par->m_curItem, *par->m_options);
			g_sokmanager->running = TRUE;
			while (TRUE)
			{
				if (!g_sokmanager->running) return;
				if (g_sokmanager->ConvMgr[par->clientNo].WaitForConverting(g_sokmanager->bExitFlag))
					break;
			}

			DWORD dwElapsed = (GetTickCount() - g_sokmanager->dwStartTime) / 1000;
			int seconds = dwElapsed;
			//			strTime.Format(L"Elapsed: %.2d:%.2d", dwElapsed / 60, dwElapsed % 60);
			wsprintf(clientMsg, L"%s\tConverting Finished 2 time: %.2d:%.2d  %d p/h max.\t", par->clientAddr, dwElapsed / 60, dwElapsed % 60, 3600*(g_sokmanager->NoPagesConv/seconds));

			SendMessage(g_sokmanager->m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_STATE, (LPARAM)clientMsg);
			strcpy( (char*)sendMessage.name, "Completed");
		}
	}
	else
	{
		par->clientInfo->convInfo = *par->m_curItem;
		par->clientInfo->convOpts = *par->m_options;
		SendMessage(g_sokmanager->m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SETINFO, (LPARAM)par->clientInfo);

		ret = 2;
		strcpy( (char*)sendMessage.name, "Page Settings are not correct");
	}
	sendMessage.type=receiveMessage.type;
	sendMessage.flag=ret;

	wsprintf(clientMsg, L"%s\tReply Command type=%d, flag=%d, number=%d\t", par->clientAddr, sendMessage.type, sendMessage.flag, sendMessage.number);
	SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SEND, (LPARAM)clientMsg);
	int len = sizeof(sendMessage);
	if ( rt_sok_msg_send(&ChannelAccept[par->clientNo], &sendMessage, sizeof( sendMessage)) == 0){
		printf("\nSend socket error. Server there ?\n\n");
		ret=3;
	}
}

VOID sokmanager::client_thread (int no)
{

	SOCKET *socketConn = &ChannelAccept[no];
	int		err=0;
	int		len;
	struct	SocketStruct			receiveMessage;		/* receieve from socket			*/
	struct	SocketStruct			sendMessage;	/* send to socket (simulation)	*/
	int		ret;

	char *pPtr;
	wchar_t *pNamePtr;
	wchar_t clientAddr[MAX_NAME_SIZE];
	wchar_t clientMsg[MAX_NAME_SIZE];
	wchar_t clientErrorMsg[MAX_NAME_SIZE];

	TrPrintf (0, "---- client_thread ----\n");
	
	struct _stat st;
	int retCode;

	CLIENT_INFO clientInfo;

	ConvFileInfo	m_curItem;
	ConvOptions		m_options;
	memset(&m_options, 0, sizeof(m_options));
	memset(&m_curItem, 0, sizeof(m_curItem));
	memset(&clientInfo, 0, sizeof(clientInfo));

	char *connAddr = socket_getPeerAddress(*socketConn);
	memset(clientAddr, 0, MAX_NAME_SIZE);
	MultiByteToWideChar(CP_ACP, 0, connAddr, strlen(connAddr), clientAddr, MAX_NAME_SIZE);
	wcscpy(clientInfo.clientAddr, clientAddr);

	clientInfo.convInfo = m_curItem;
	clientInfo.convOpts = m_options;
	SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SETINFO, (LPARAM)&clientInfo);

	while(!err) {
		TrPrintf (0, "waiting for message\n");
		len=rt_sok_msg_receive (socketConn, &receiveMessage, sizeof(receiveMessage));
		TrPrintf (0, "err=%d, len=%d, id=%d  flag=%d\n", err, len, receiveMessage.type, receiveMessage.flag);
		//strcpy( (char*)sendMessage.name, "");
		memset(&sendMessage, 0, sizeof(sendMessage));

		ret=0;

		if (len<=0)
		{
			err = 1;
			running=FALSE;
			m_logWriter->log(LOG_NOTICE, L"%s Client Disconnected", clientAddr);

			if (bExitFlag == false)
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_DISCONNECTED, (LPARAM)clientAddr);
			break;
		}
		else {
			switch (receiveMessage.type) {

			case REM_RESET:
				TrPrintf (0, "Reset \n");
				m_logWriter->log(LOG_NOTICE, L"%s Client: Reset Command Received", clientAddr);

				memset(&m_options, 0, sizeof(m_options));
				memset(&m_curItem, 0, sizeof(m_curItem));
				wsprintf(clientMsg, L"%s\tReset Command Received\t", clientAddr);
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_RECEIVE, (LPARAM)clientMsg);
			//	system( "del d:\\temp\\*.* /Q" );
			//	system( "del d:\\pdf\\temp\\*.* /Q" );

				clientInfo.convInfo = m_curItem;
				clientInfo.convOpts = m_options;
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SETINFO, (LPARAM)&clientInfo);
				break;

			case REM_LOAD_PDF:
				TrPrintf (0, "Load PDF (%s) \n", receiveMessage.name);
				sendMessage.number = 0;

				//memset(&m_curItem, 0, sizeof(m_curItem));
				memset(m_curItem.fileName, 0, sizeof(m_curItem.fileName));
				memset(m_curItem.srcPath, 0, sizeof(m_curItem.srcPath));
				memset(m_curItem.selectPages, 0, sizeof(m_curItem.selectPages));
				m_curItem.pageCount = 0;

				MultiByteToWideChar(CP_ACP, 0, (char*)receiveMessage.name, strlen((char*)receiveMessage.name), m_curItem.srcPath, MAX_NAME_SIZE);

				pPtr = strrchr((char*)receiveMessage.name, '\\');
				if (pPtr == NULL)
					pPtr = strrchr((char*)receiveMessage.name, '/');

				if (pPtr != NULL)
				{
					pPtr++;
					MultiByteToWideChar(CP_ACP, 0, pPtr, strlen(pPtr), m_curItem.fileName, MAX_NAME_SIZE);
				}
				else
					MultiByteToWideChar(CP_ACP, 0, (char*)receiveMessage.name, strlen((char*)receiveMessage.name), m_curItem.fileName, MAX_NAME_SIZE);

				pNamePtr = wcsrchr(m_curItem.fileName, L'.');
				if (pNamePtr != NULL)
				{
					*pNamePtr = 0;
				}

				m_logWriter->log(LOG_NOTICE, L"%s Client: Load PDF(%s) Command Received", clientAddr, m_curItem.srcPath);

				wsprintf(clientMsg, L"%s\tLoad PDF(%s) Command Received\t", clientAddr, m_curItem.srcPath);
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_RECEIVE, (LPARAM)clientMsg);

				// --- _stat opens the file ---
				retCode = _stat((char*)receiveMessage.name, &st);
				if (retCode < 0 || ((st.st_mode & _S_IFDIR) != 0))
				{
					ret = 1;
					strcpy( (char*)sendMessage.name, "File Not Found");
					memset(m_curItem.fileName, 0, sizeof(m_curItem.fileName));
					memset(m_curItem.srcPath, 0, sizeof(m_curItem.srcPath));
				}
				else
				{
					m_curItem.pageCount = ConvMgr[no].getPDFPageCount(m_curItem.srcPath);
					if (m_curItem.pageCount <= 0)
					{
						ret = 2;
						strcpy( (char*)sendMessage.name, "Cannot Open Document");
						memset(m_curItem.fileName, 0, sizeof(m_curItem.fileName));
						memset(m_curItem.srcPath, 0, sizeof(m_curItem.srcPath));
					}
					else
					{
						ret = 0;
						sendMessage.number = m_curItem.pageCount;
					}
				}

				clientInfo.convInfo = m_curItem;
				clientInfo.convOpts = m_options;
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SETINFO, (LPARAM)&clientInfo);

				break;
			case REM_SET_TARGET:
				TrPrintf (0, "Target directory (%s) \n", receiveMessage.name);

				memset(m_curItem.dstPath, 0, sizeof(m_curItem.dstPath));
				MultiByteToWideChar(CP_ACP, 0, (char*)receiveMessage.name, strlen((char*)receiveMessage.name), m_curItem.dstPath, MAX_NAME_SIZE);
				m_logWriter->log(LOG_NOTICE, L"%s Client: Target directory(%s) Command Received", clientAddr, m_curItem.dstPath);

				wsprintf(clientMsg, L"%s\tTarget Directory(%s) Command Received\t", clientAddr, m_curItem.dstPath);
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_RECEIVE, (LPARAM)clientMsg);

				retCode = _stat((char*)receiveMessage.name, &st);
				if (retCode < 0 || ((st.st_mode & _S_IFDIR) == 0))
				{
					memset(m_curItem.dstPath, 0, sizeof(m_curItem.dstPath));
					unsigned char* pLastPtr = &receiveMessage.name[strlen((char*)receiveMessage.name) - 1];
					while (pLastPtr > receiveMessage.name)
					{
						if (*pLastPtr != '\\' && *pLastPtr != '/')
							break;

						*pLastPtr = 0;
						pLastPtr--;
					}
					
					retCode = _stat((char*)receiveMessage.name, &st);
					if (retCode < 0 || ((st.st_mode & _S_IFDIR) == 0))
					{
						ret = 1;
						strcpy( (char*)sendMessage.name, "Path does not exist");
					}
					else
					{
						MultiByteToWideChar(CP_ACP, 0, (char*)receiveMessage.name, strlen((char*)receiveMessage.name), m_curItem.dstPath, MAX_NAME_SIZE);
					}
				}

				clientInfo.convInfo = m_curItem;
				clientInfo.convOpts = m_options;
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SETINFO, (LPARAM)&clientInfo);

				break;
			case REM_SET_RES_WIDTH:
				TrPrintf (0, "Resolution width (%d) \n", receiveMessage.parameter);
				m_logWriter->log(LOG_NOTICE, L"%s Client: Resolution width(%d) Command Received", clientAddr, receiveMessage.parameter);
				wsprintf(clientMsg, L"%s\tResolution width(%d) Command Received\t", clientAddr, receiveMessage.parameter);
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_RECEIVE, (LPARAM)clientMsg);

//				m_options.resolutionX = receiveMessage.parameter;

				clientInfo.convInfo = m_curItem;
				clientInfo.convOpts = m_options;
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SETINFO, (LPARAM)&clientInfo);

				break;
			case REM_SET_RES_HEIGHT:
				TrPrintf (0, "Resolution height (%d) \n", receiveMessage.parameter);
				m_logWriter->log(LOG_NOTICE, L"%s Client: Resolution height(%d) Command Received", clientAddr, receiveMessage.parameter);
				wsprintf(clientMsg, L"%s\tResolution height(%d) Command Received\t", clientAddr, receiveMessage.parameter);
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_RECEIVE, (LPARAM)clientMsg);
//				m_options.resolutionY = receiveMessage.parameter;

				clientInfo.convInfo = m_curItem;
				clientInfo.convOpts = m_options;
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SETINFO, (LPARAM)&clientInfo);

				break;
			case REM_SET_PAGE_WIDTH:
				TrPrintf (0, "Page width (%d) \n", receiveMessage.parameter);
				m_logWriter->log(LOG_NOTICE, L"%s Client: Page width(%d) Command Received", clientAddr, receiveMessage.parameter);
				if ( m_convOptions.sizeWidth > 0)
					wsprintf(clientMsg, L"%s\tPage width(%d) Command Received but taking local input because not 0\t", clientAddr, receiveMessage.parameter);
				else
					wsprintf(clientMsg, L"%s\tPage width(%d) Command Received\t", clientAddr, receiveMessage.parameter);
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_RECEIVE, (LPARAM)clientMsg);

				m_options.sizeWidth = (float) receiveMessage.parameter/1000;

				clientInfo.convInfo = m_curItem;
				clientInfo.convOpts = m_options;
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SETINFO, (LPARAM)&clientInfo);

				break;
			case REM_SET_PAGE_HEIGHT:
				TrPrintf (0, "Page height (%d) \n", receiveMessage.parameter);
				m_logWriter->log(LOG_NOTICE, L"%s Client: Page height(%d) Command Received", clientAddr, receiveMessage.parameter);
				if ( m_convOptions.sizeHeight > 0)
					wsprintf(clientMsg, L"%s\tPage height(%d) Command Received but taking local input because not 0\t", clientAddr, receiveMessage.parameter);
				else
					wsprintf(clientMsg, L"%s\tPage height(%d) Command Received\t", clientAddr, receiveMessage.parameter);
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_RECEIVE, (LPARAM)clientMsg);

				m_options.sizeHeight = (float) receiveMessage.parameter/1000;

				clientInfo.convInfo = m_curItem;
				clientInfo.convOpts = m_options;
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SETINFO, (LPARAM)&clientInfo);

				break;
			case REM_CONVERT_N_PAGES:
				TrPrintf (0, "Convert (%d) pages starting at page (%d) \n--> target (bmp%d.bmp) \n", receiveMessage.parameter, receiveMessage.number, receiveMessage.startNumber);
				m_logWriter->log(LOG_NOTICE, L"%s Client: Convert (%d) pages starting at page (%d) \n--> target (bmp%d.bmp) \n Command Received", clientAddr, receiveMessage.number, receiveMessage.parameter, receiveMessage.startNumber);
				wsprintf(clientMsg, L"%s\tConvert (%d) pages starting at page (%d) \n--> target (bmp%d.bmp) Command Received\t", clientAddr, receiveMessage.number, receiveMessage.parameter, receiveMessage.startNumber);
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_RECEIVE, (LPARAM)clientMsg);

				if (wcslen(m_curItem.srcPath) == 0)
				{
					ret = 2;
					strcpy( (char*)sendMessage.name, "PDF File is not loaded correctly");
					break;
				}

				if (wcslen(m_curItem.dstPath) == 0)
				{
					ret = 2;
					strcpy( (char*)sendMessage.name, "Target Path is not correct");
					break;
				}

				Sconvert_thread_par par;
				par.clientNo		= no;
				par.clientAddr		= clientAddr;
				par.pReceiveMessage = &receiveMessage;
				par.clientInfo		= &clientInfo;
				par.m_curItem		= &m_curItem;
				par.m_options		= &m_options;

				ConvertThreadHandle = CreateThread ( 
					NULL,									/* no security attributes */
					0,										/* default stack size */
					(LPTHREAD_START_ROUTINE) &start_convert_thread,/* function to call */
					&par,						/* parameter for function */
					0,										/* 0=thread runs immediately after being called */
					NULL									/* returns thread identifier */
				);

				break;
			case REM_CLOSE_PDF:
				running = FALSE;
				TrPrintf (0, "Closed pdf\n", receiveMessage.type);
				m_logWriter->log(LOG_NOTICE, L"%s Client: Close pdf Command Received", clientAddr);

				wsprintf(clientMsg, L"%s\tClose pdf Command Received\t", clientAddr);
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_RECEIVE, (LPARAM)clientMsg);

				clientInfo.convInfo = m_curItem;
				clientInfo.convOpts = m_options;
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SETINFO, (LPARAM)&clientInfo);

				break;
			default:
				TrPrintf (0, "received: unexpected type %d\n", receiveMessage.type);
				m_logWriter->log(LOG_NOTICE, L"%s Client: unexpected type Command Received", clientAddr);
				wsprintf(clientMsg, L"%s\tunexpected type %d\t", clientAddr, receiveMessage.type);
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_RECEIVE, (LPARAM)clientMsg);
				break;
			} /* end switch */

			// send REPLY
			sendMessage.type=receiveMessage.type;
			sendMessage.flag=ret;
			TrPrintf (0, "REPLY to %s Client, type=%d  flag=%d\n", clientAddr, sendMessage.type, sendMessage.flag);
			m_logWriter->log(LOG_NOTICE, L"REPLY to %s Client, type=%d  flag=%d\n", clientAddr, sendMessage.type, sendMessage.flag);
			
			if (ret != 0)
			{
				memset(clientErrorMsg, 0, sizeof(clientErrorMsg));
				MultiByteToWideChar(CP_ACP, 0, (char*)sendMessage.name, strlen((char*)sendMessage.name), clientErrorMsg, sizeof(clientErrorMsg));
				wsprintf(clientMsg, L"%s\tError: %s\t", clientAddr, clientErrorMsg);
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_STATE, (LPARAM)clientMsg);
			}
			wsprintf(clientMsg, L"%s\tReply Command type=%d, flag=%d, number=%d\t", clientAddr, sendMessage.type, sendMessage.flag, sendMessage.number);
			SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_SEND, (LPARAM)clientMsg);

			len = sizeof( sendMessage);
			if ( rt_sok_msg_send( socketConn, &sendMessage, sizeof( sendMessage)) == 0){
				printf("\nSend socket error. Server there ?\n\n");
				err=3;
			}
		}
	}
	ConvMgr[no].stop();
	TrPrintf (0, "client thread: LOOP LEFT  err=%d\n", err);
	NoConn--;
	closesocket (*socketConn);
	*socketConn=INVALID_SOCKET;
} /* end client_thread */

sokmanager::sokmanager()
{
	//Trace = 0;
	bExitFlag = false;
	g_sokmanager = this;
	First = 0;
	strcpy(MyName, "clentpdf2bmp");
	NonBlk = 0;

	m_logWriter = new LogWriter();
	m_logWriter->initLog();
}

sokmanager::~sokmanager()
{
	if(m_logWriter)
	{
		delete m_logWriter;
		m_logWriter = NULL;
	}
}

void sokmanager::Init(HWND hParentWnd)
{
	int i;

	m_hParentWnd = hParentWnd;

	strcpy( Host, "localHost");
	strcpy( Port, "10065");
	NoConn				=0;
	SocketListen		=INVALID_SOCKET;
	SocketCall			=INVALID_SOCKET;
	for (i=0; i<MAX_CLIENTS; i++) {
		ThreadHandle  [i]=NULL;
		ChannelAccept [i]=INVALID_SOCKET;
	}

	WatchClientHandle = CreateThread ( 
		NULL,									/* no security attributes */
		0,										/* default stack size */
		(LPTHREAD_START_ROUTINE) &watch_main_thread,/* function to call */
		NULL,			/* parameter for function */
		0,										/* 0=thread runs immediately after being called */
		NULL									/* returns thread identifier */
		);
}

void sokmanager::UnInit()
{
	bExitFlag = true;

	for (int i=0; i<MAX_CLIENTS; i++)
	{
		if (ChannelAccept [i] != INVALID_SOCKET) {
			closesocket(ChannelAccept [i]);
		}

		if (ThreadHandle [i] != NULL && ThreadHandle [i] != INVALID_HANDLE_VALUE)
			WaitForSingleObject(ThreadHandle [i], 1000);
	}

	if (SocketListen != INVALID_SOCKET)
		closesocket(SocketListen);

	if (WatchClientHandle != NULL && WatchClientHandle != INVALID_HANDLE_VALUE)
		WaitForSingleObject(WatchClientHandle, 1000);
}

void sokmanager::WatchClient()
{
	int	i, ret;
	wchar_t clientAddr[MAX_NAME_SIZE];
	wchar_t clientMsg[MAX_NAME_SIZE];

	rt_sok_init (FALSE, Host, Port, &SocketListen);
	if ( NonBlk)
		rt_sok_set_nb (&SocketListen);
	listen (SocketListen, MAX_CLIENTS);
	while (bExitFlag == false) {
		TrPrintf( 0, "Waiting for clients\n");

		if ( NoConn>=MAX_CLIENTS) {
			printf("Max. number of clients %d reached\n", MAX_CLIENTS);
			wsprintf(clientMsg, L"%s\tMax. number of clients %d reached\t", clientAddr, MAX_CLIENTS);
			SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_DISCONNECTED, (LPARAM)clientMsg);
			Sleep (1000);
			continue;
		}
		/*--- Get the next available connection ---*/
		ActualConn=-1;
		for (i=0; i<MAX_CLIENTS; i++)
			if (ChannelAccept [i]==INVALID_SOCKET) {
				ActualConn=i;
				break;
			}
			if (ActualConn==-1) {
				printf( "2Too many clients connected, >%d", MAX_CLIENTS);
				continue;
			}
			/*--- wait for client connection ---*/
			ChannelAccept [ActualConn]=accept (SocketListen, NULL, NULL);
			if (ChannelAccept [ActualConn]==INVALID_SOCKET) {
				if (ret=WSAGetLastError() ==10093) { 
					// socket has already been closed
					printf( "socket has already been closed");
					return;
				}
			}
			else {
				BOOL bNoDelay = TRUE;
				char *connAddr = socket_getPeerAddress(ChannelAccept[ActualConn]);

				setsockopt (ChannelAccept [ActualConn], IPPROTO_TCP, TCP_NODELAY, (LPSTR) &bNoDelay, sizeof (BOOL));

				TrPrintf( 0, "create a receiver thread for that client");

				//to get ip address of client (add by CSC)
				memset(clientAddr, 0, MAX_NAME_SIZE);
				MultiByteToWideChar(CP_ACP, 0, connAddr, strlen(connAddr), clientAddr, MAX_NAME_SIZE);

				m_logWriter->log(LOG_NOTICE, L"A Client Connected from IP address [%s].", clientAddr);
				SendMessage(m_hParentWnd, WM_CLIENT_NOTIFY, (WPARAM)CLIENT_MSG_CONNECTED, (LPARAM)clientAddr);

				/*--- create a receiver thread for that client ---*/
				ThreadHandle [ActualConn]=CreateThread ( 
					NULL,									/* no security attributes */
					0,										/* default stack size */
					(LPTHREAD_START_ROUTINE) &watch_client_thread,/* function to call */
					(void*)ActualConn,						/* parameter for function */
					0,										/* 0=thread runs immediately after being called */
					NULL									/* returns thread identifier */
					);
				if (ThreadHandle [ActualConn]==NULL)
					printf("Could not start Thread");
				SetThreadPriority (ThreadHandle [ActualConn], THREAD_PRIORITY_BELOW_NORMAL);
				NoConn++;
			}
	}
}

char* sokmanager::socket_getPeerAddress(
	SOCKET sock)
{
	struct sockaddr_in  info;
	int info_size = sizeof(info);
	//if (sock == INVALID_SOCKET) return SOCKET_ERROR;

	getpeername(sock, (struct sockaddr *)&info, &info_size);

	return inet_ntoa(info.sin_addr);
}

void sokmanager::InitParameter( ConvOptions &options)
{
	m_convOptions =  options;	
}

