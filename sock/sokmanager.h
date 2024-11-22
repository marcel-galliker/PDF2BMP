#ifndef __SOKMANAGER_H__
#define __SOKMANAGER_H__

extern "C"
{
	#include "rt_sok_api_min.h"
};

#include "../PDFToBmp/convmanager.h"
#include "../LogWriter/LogWriter.h"
#include "../PDFToBmp/Define.h"

// ---------- DEFINES -----------------------------------------------------------------------------
#define MAX_NAME_HOST		30
#define MAX_NAME_PORT		10
#define MAX_CLIENTS			1
#define MAX_RECORD_LEN		250
#define RECS				10
#define MIC_REM_NAME_LEN	500

// ---------- STRUCTURES --------------------------------------------------------------------------
enum REMPacketType {
	REM_RESET,				//0: resets all buffers. Return 0
	REM_LOAD_PDF,			//1: load the pdf. name contains path and filename of source 
	// for example: D:/data/example.pdf
	REM_SET_TARGET,			//2: name has path of target directory. Example H:/hotfolder
	REM_CLOSE_PDF,			//3
	REM_SET_RES_WIDTH,		//4 set in parameter, default is 600 dpi
	REM_SET_RES_HEIGHT,		//5 set in parameter, default is 600 dpi
	REM_SET_PAGE_WIDTH,		//6 inch set in parameter, default is 0=as is
	REM_SET_PAGE_HEIGHT,	//7 inch set in parameter, default is 0=as is
	REM_SET_THREAD_CNT,		//8 number of threads
	REM_CONVERT_N_PAGES,	//9 starting page in parameter, number in number
	REM_CONVERT_PAGE,		//10 starting page in parameter
	REMPacketType_MAX
}; 

/*--- receive structure () ---*/
struct SocketStruct {
	int					type;		// see REMPacketType
	int					flag;		// 
	int					parameter;	// 
	int					number;	// 
	int					startNumber;  //
	unsigned char		name[MIC_REM_NAME_LEN +1];	// 
};

struct ClientThreadParam {
	wchar_t				clientAddr[MAX_NAME_SIZE];
	SOCKET*				clientSocket;
};

void TrPrintf (int level, char *format, ...);
extern int	 Trace;
extern char	 MyName[20];


class sokmanager
{
public:
	sokmanager();
	~sokmanager();

private:
	int		Client;
	int		First;
	char	Host[ MAX_NAME_HOST+1];					// Hostname						
	char	Port[ MAX_NAME_PORT+1];					// Communication Port	
	int		NoConn,ActualConn;
	int		NonBlk;
	ConvOptions		m_convOptions;

	bool	bExitFlag;
	bool	running;
	DWORD	dwStartTime;
	int		NoPagesConv;

	HANDLE	WatchClientHandle;
	HANDLE	ConvertThreadHandle;
	HANDLE	ThreadHandle[ MAX_CLIENTS];
	ConvManager		ConvMgr[ MAX_CLIENTS];

	SOCKET	SocketListen;							// main socket 							
	SOCKET	SocketCall;								// main socket for simulation			
	SOCKET	SocketSetupServer;						// main socket for setup server	

	LogWriter *m_logWriter;
	HWND	m_hParentWnd;

public:
	void	Init(HWND hParentWnd);
	void	UnInit();
	void	WatchClient();
	void	InitParameter( ConvOptions &options);

	VOID	client_thread (int no);
	char*	socket_getPeerAddress(SOCKET sock);
	SOCKET	ChannelAccept[ MAX_CLIENTS];			// channel per client

	typedef struct Sconvert_thread_par
	{
		int					clientNo;
		wchar_t				*clientAddr;
		struct SocketStruct *pReceiveMessage;

		CLIENT_INFO		*clientInfo;

	} Sconvert_thread_par;

	VOID sokmanager::convert_thread (Sconvert_thread_par *par);
};
#endif