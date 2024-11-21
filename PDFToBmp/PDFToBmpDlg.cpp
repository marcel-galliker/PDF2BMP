
// PDFToBmpDlg.cpp : implementation file
//
// pdf_image_bmp.c: put into bitmap file here

#include "stdafx.h"
#include <Nb30.h>
#include "PDFToBmp.h"
#include "PDFToBmpDlg.h"
#include "afxdialogex.h"
#include "FolderDlg.h"
#include "windows.h"
#include "Define.h"
#include "../LogWriter/DateTime.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define GET_PROGRESS_POLLING_INTERVAL	100 // 100ms
#define ID_TIMER_CONVERT		1

#define GET_SAVE_POLLING_INTERVAL		500 // 100ms
#define ID_TIMER_SAVE			2

#define REPORT_NAME_COLUMN1 L"IP Address"
#define REPORT_NAME_COLUMN2 L"Message"
#define REPORT_NAME_COLUMN3 L"Time"

#define MAX_MAC_BUFFER		32						/* Maximal name buffers */
typedef struct _ASTAT_{
	ADAPTER_STATUS adapt;
	NAME_BUFFER    NameBuff [MAX_MAC_BUFFER];
} ASTAT, * PASTAT;


// CPDFToBmpDlg dialog
CPDFToBmpDlg::CPDFToBmpDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPDFToBmpDlg::IDD, pParent)
	, m_strSrcPath(_T(""))
	, m_strDestPath(_T("d:/pdf/temp"))
	, m_nResWidth((int)PDF_RESOLUTION_600)
	, m_nResHeight((int)PDF_RESOLUTION_600)
	, m_fSizeWidth(0)
	, m_fSizeHeight(0)
	, m_strPrintedPages(_T("All"))
	, m_nThreadCnt(1)
{
	memset(m_pY, 0, sizeof(float)*5);

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPDFToBmpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EN_SRC, m_strSrcPath);
	DDV_MaxChars(pDX, m_strSrcPath, 255);
	DDX_Text(pDX, IDC_EN_DEST, m_strDestPath);
	DDV_MaxChars(pDX, m_strDestPath, 255);
	DDX_Text(pDX, IDC_EN_RES_WIDTH, m_nResWidth);
	DDX_Text(pDX, IDC_EN_THREAD_COUNT, m_nThreadCnt);
	DDV_MinMaxInt(pDX, m_nResWidth, 0, 5000);
	DDX_Text(pDX, IDC_EN_RES_HEIGHT, m_nResHeight);
	DDV_MinMaxInt(pDX, m_nResHeight, 0, 1200);
	DDX_Text(pDX, IDC_EN_SIZE_WIDTH, m_fSizeWidth);
	DDX_Text(pDX, IDC_EN_SIZE_HEIGHT, m_fSizeHeight);
	DDX_Text(pDX, IDC_EN_Y1, m_pY[0]);
	DDV_MinMaxInt(pDX, (int)m_pY[0], 0, 100);
	DDX_Text(pDX, IDC_EN_Y2, m_pY[1]);
	DDV_MinMaxInt(pDX, (int)m_pY[1], 0, 100);
	DDX_Text(pDX, IDC_EN_Y3, m_pY[2]);
	DDV_MinMaxInt(pDX, (int)m_pY[2], 0, 100);
	DDX_Text(pDX, IDC_EN_Y4, m_pY[3]);
	DDV_MinMaxInt(pDX, (int)m_pY[3], 0, 100);
	DDX_Text(pDX, IDC_EN_Y5, m_pY[4]);
	DDV_MinMaxInt(pDX, (int)m_pY[4], 0, 100);
	DDX_Text(pDX, IDC_EN_PRINT_PAGES, m_strPrintedPages);
	DDV_MaxChars(pDX, m_strPrintedPages, 255);
	DDX_Text(pDX, IDC_EN_THREAD_COUNT, m_nThreadCnt);
	DDV_MinMaxInt(pDX, m_nThreadCnt, 1, 5);
	DDX_Control(pDX, IDC_LIST_CLIENT, m_ListClient);
	DDX_Control(pDX, IDC_LIST_REPORT, m_ListReport);
	DDX_Control(pDX, IDC_COMBO_MODE, m_comboMode);
	DDX_Control(pDX, IDC_COMBO_SPLIT, m_comboSplit);
}

BEGIN_MESSAGE_MAP(CPDFToBmpDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDOK, &CPDFToBmpDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUT_BROW_SRC, &CPDFToBmpDlg::OnBnClickedButBrowSrc)
	ON_BN_CLICKED(IDC_BUT_BROW_DST, &CPDFToBmpDlg::OnBnClickedButBrowDst)

	ON_MESSAGE(WM_ICON_NOTIFY, OnTrayNotification)
	ON_MESSAGE(WM_CLIENT_NOTIFY, OnClientNotification)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_COMMAND(IDM_TRAY_OPEN, &CPDFToBmpDlg::OnTrayOpen)
	ON_COMMAND(IDM_TRAY_EXIT, &CPDFToBmpDlg::OnTrayExit)
	ON_LBN_SELCHANGE(IDC_LIST_CLIENT, &CPDFToBmpDlg::OnLbnSelchangeListClient)
	ON_EN_CHANGE(IDC_EN_THREAD_COUNT, &CPDFToBmpDlg::OnEnChangeEnThreadCnt)
	ON_EN_CHANGE(IDC_EN_RES_WIDTH, &CPDFToBmpDlg::OnEnChangeEnResWidth)
	ON_EN_CHANGE(IDC_EN_RES_HEIGHT, &CPDFToBmpDlg::OnEnChangeEnResHeight)
	ON_EN_CHANGE(IDC_EN_Y1, &CPDFToBmpDlg::OnChangeEnY1)
	ON_EN_CHANGE(IDC_EN_Y2, &CPDFToBmpDlg::OnChangeEnY2)
	ON_EN_CHANGE(IDC_EN_Y3, &CPDFToBmpDlg::OnChangeEnY3)
	ON_EN_CHANGE(IDC_EN_Y4, &CPDFToBmpDlg::OnChangeEnY4)
	ON_EN_CHANGE(IDC_EN_Y5, &CPDFToBmpDlg::OnChangeEnY5)
	ON_CBN_SELCHANGE(IDC_COMBO_MODE, &CPDFToBmpDlg::OnSelchangeComboMode)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLIT, &CPDFToBmpDlg::OnSelchangeComboSplit)
	ON_EN_CHANGE(IDC_EN_SIZE_WIDTH, &CPDFToBmpDlg::OnEnChangeEnSizeWidth)
	ON_EN_CHANGE(IDC_EN_SIZE_HEIGHT, &CPDFToBmpDlg::OnEnChangeEnSizeHeight)
	ON_BN_CLICKED(IDRESETLIST, &CPDFToBmpDlg::OnBnClickedResetlist)
END_MESSAGE_MAP()


// CPDFToBmpDlg message handlers

BOOL CPDFToBmpDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// --- restore values from last time ---
	CFile f;
	CFileException ex;
	TCHAR* pszFileName = _T("LastSet");
	if( f.Open( pszFileName , CFile::modeRead | CFile::shareDenyWrite, &ex)){
		f.Read( &m_options, sizeof( m_options));
		f.Close();
		m_nResWidth = (int)m_options.resolutionX;
		m_nResHeight = (int)m_options.resolutionY;
		m_fSizeWidth = m_options.sizeWidth;
		m_fSizeHeight = m_options.sizeHeight;
		memcpy(m_pY, m_options.pY, sizeof(float)*5);
		m_nThreadCnt = m_options.threadCnt;
	}
	else{
		m_options.convertmode=0;
		m_options.convertsplit=0;
		m_options.resolutionX=600;
		m_options.resolutionY=600;
		m_options.sizeHeight=0;
		m_options.sizeWidth=0;
		m_options.threadCnt=1;
		m_options.pY[0]=0;
		m_options.pY[1]=25;
		m_options.pY[2]=50;
		m_options.pY[3]=75;
		m_options.pY[4]=100;
	}
	m_pY[0] = m_options.pY[0];
	m_pY[1] = m_options.pY[1];
	m_pY[2] = m_options.pY[2];
	m_pY[3] = m_options.pY[3];
	m_pY[4] = m_options.pY[4];


	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// To show tray icon (add by CSC)
	if (!m_TrayIcon.Create(this, WM_ICON_NOTIFY, L"PDF Converter", NULL, IDR_MAINFRAME))
		return FALSE;
	m_TrayIcon.SetIcon(IDR_MAINFRAME);

	m_comboMode.AddString(L"Use Screening");
	m_comboMode.AddString(L"No Screening");	
	m_comboMode.SetCurSel( m_options.convertmode);

	m_comboSplit.AddString(L"No");
	m_comboSplit.AddString(L"Yes");	
	m_comboSplit.SetCurSel( m_options.convertsplit);

	m_fSizeHeight = m_options.sizeHeight;
	m_fSizeWidth = m_options.sizeWidth;
	m_nThreadCnt = m_options.threadCnt;

	memset(&m_clientInfos, 0, sizeof(m_clientInfos));

	// --- license & mac ---
	m_licensed = GetMACaddress();
	if( !m_licensed)
		MessageBox(L"You don't have a license. Please send d:/SendToGT.txt to markus@graphtech.us", L"Bad Boy");

	SetPriorityClass (GetCurrentProcess(), REALTIME_PRIORITY_CLASS);	//HIGH_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS
	m_convManager = new ConvManager();
	m_sokManager = new sokmanager();
	m_sokManager->Init(GetSafeHwnd());

	m_ExitDlg = new CPDFExitDlg();
	//m_loadManager = new LoadManager(this);
	//m_prevSelPage = tr("All");
	//m_curIndex = 0;
	//m_getProgressTimer = -1;
	m_prevStatus = CRS_SUCCESSED;
	m_runningEngine = false;
	//m_uiEnableFlag = true;
	memset(&m_options, 0, sizeof(m_options));

	m_butConvert = (CButton*)GetDlgItem(IDOK);
	m_progConvert = (CProgressCtrl*)GetDlgItem(IDC_PROG_CONVERTING);
	m_progConvert->SetRange(0, 100);
	m_progConvert->SetStep(1);

	m_stcElapsedTime = (CStatic*)GetDlgItem(IDC_STC_ELAPSEDTIME);
	m_stcConvertedPage = (CStatic*)GetDlgItem(IDC_STC_CONVERTPAGES);

	m_ListReport.InsertColumn(0, REPORT_NAME_COLUMN1, LVCFMT_LEFT, 50);
	m_ListReport.InsertColumn(1, REPORT_NAME_COLUMN2, LVCFMT_LEFT, 400);
	m_ListReport.InsertColumn(2, REPORT_NAME_COLUMN3, LVCFMT_LEFT, 150);

	UpdateData(FALSE);

	SetConvertOptions(m_curItem.srcPath);
	m_sokManager->InitParameter( m_options);	// update socket with values

	SetTimer(ID_TIMER_SAVE, GET_SAVE_POLLING_INTERVAL, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPDFToBmpDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPDFToBmpDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPDFToBmpDlg::OnBnClickedOk()
{
	CString strButtonText = L"";
//	char str[100];

	m_butConvert->GetWindowText(strButtonText);
	if (m_convManager->isRunning() || strButtonText == L"Stop")
	{
		m_runningEngine = false;
		m_convManager->stop();
		m_butConvert->SetWindowText(L"Convert");

		KillTimer(1);
		m_progConvert->SetPos(0);
		m_stcConvertedPage->SetWindowText(L"");

		return;
	}

	if (UpdateData(TRUE) == FALSE)
		return;

	if (m_strSrcPath.IsEmpty() || m_strDestPath.IsEmpty())
	{
		MessageBox(L"Please input source file name and output directory.", L"PDF2BMP");
		return;
	}

	if (!testWriteFile())
	{
		MessageBox(L"You are not allowed to write into that output folder.", L"PDF2BMP");
		return;
	}

//	sprintf_s( str, "del %s%c%c.%c %cQ", CStringA( m_strDestPath).GetString(), '/', '*', '*', '/');
//	system( str);
//	system( "del d:\\temp\\*.* /Q" );
	system( "del d:\\pdf\\temp\\*.* /Q" );

	bool bNeedPassword = false;
	memset(&m_curItem, 0, sizeof(ConvFileInfo));

	m_curItem.pageCount = m_convManager->getPDFPageCount((wchar_t*)(LPCWSTR)(LPCTSTR)m_strSrcPath);

	if (!isValidPage(m_strPrintedPages))
	{
		MessageBox(L"Selected page is out of range!", L"PDF2BMP");
		return;
	}

	int i = 0;
	m_curItem.status = FCS_WAITING;

	m_dwStartTick = GetTickCount();
	m_runningEngine = true;
	ConvertItem();

	m_butConvert->SetWindowText(L"Stop");
	SetTimer(ID_TIMER_CONVERT, GET_PROGRESS_POLLING_INTERVAL, NULL);
	//m_getProgressTimer = startTimer(GET_PROGRESS_POLLING_INTERVAL);
}

void CPDFToBmpDlg::OnBnClickedButBrowSrc()
{
	if (UpdateData(TRUE) == FALSE)
		return;

	CFileDialog fOpenDlg(TRUE, L"pdf", L"", OFN_HIDEREADONLY|OFN_FILEMUSTEXIST, 
		L"PDF Files (*.pdf)|*.pdf||", this);

	fOpenDlg.m_pOFN->lpstrTitle = L"File Open";

	CString strFilePath = L"";

	int nFind = m_strSrcPath.ReverseFind('\\');
	if (nFind != -1)
		strFilePath = m_strSrcPath.Left(nFind);
	else
		strFilePath = m_strSrcPath;

	fOpenDlg.m_pOFN->lpstrInitialDir = strFilePath;

	if(fOpenDlg.DoModal()==IDOK)
	{
		m_strSrcPath = fOpenDlg.GetPathName();
	}

	UpdateData(FALSE);
}


void CPDFToBmpDlg::OnBnClickedButBrowDst()
{
	if (UpdateData(TRUE) == FALSE)
		return;

	CFolderDialog folderDlg(L"Output Directory", m_strDestPath, this, BIF_RETURNONLYFSDIRS);
	if (folderDlg.DoModal() == IDOK)
	{
		m_strDestPath = folderDlg.GetFolderPath();
		if (m_strDestPath.GetLength() > 0 && m_strDestPath[m_strDestPath.GetLength() - 1] != '\\')
		{
			m_strDestPath += L"\\";
		}
	}

	UpdateData(FALSE);
}

void CPDFToBmpDlg::ConvertItem()
{
	if (m_runningEngine)
	{
		CString strFileName = L"";

		int nFind = m_strSrcPath.ReverseFind('\\');
		if (nFind != -1)
			strFileName = m_strSrcPath.Right(m_strSrcPath.GetLength() - nFind - 1);
		else
			strFileName = m_strSrcPath;

		nFind = strFileName.ReverseFind('.');
		if (nFind != -1)
		{
			strFileName = strFileName.Left(nFind);
		}
		WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)(LPCTSTR)m_strPrintedPages, m_strPrintedPages.GetLength(), m_curItem.selectPages, 1024, NULL, NULL);

		wcscpy(m_curItem.fileName, (LPCWSTR)(LPCTSTR)strFileName);
		wcscpy(m_curItem.srcPath, (LPCWSTR)(LPCTSTR)m_strSrcPath);
		wcscpy(m_curItem.dstPath, (LPCWSTR)(LPCTSTR)m_strDestPath);

		if (!SetConvertOptions(m_curItem.srcPath))
			return;

		m_convManager->convert(m_curItem, m_options);
	}
}

bool CPDFToBmpDlg::SetConvertOptions(wchar_t* srcPath)
{
//	int err;
//	FILE *fili;
//	struct savePar savePar;

/*	err = fopen_s( &fili, "d:/pdf/savePar.dat", "rb");
	if ( !err){
		fread( &savePar, sizeof (unsigned char), sizeof( struct savePar), fili);
		fclose( fili);
		m_nResWidth = savePar.fSizeWidth;
		m_nResHeight = savePar.fSizeHeight;
		m_nThreadCnt = savePar.nThreadCnt;
		m_fSizeWidth = savePar.nResWidth;
		m_fSizeHeight = savePar.nResHeight;
	}
*/

	m_options.resolutionX = (float)m_nResWidth;
	m_options.resolutionY = (float)m_nResHeight;

	m_options.sizeWidth = m_fSizeWidth;
	m_options.sizeHeight = m_fSizeHeight;

	memcpy(m_options.pY, m_pY, sizeof(float)*5);

	m_options.threadCnt = m_nThreadCnt;
	m_options.convertmode = m_comboMode.GetCurSel();
	m_options.convertsplit = m_comboSplit.GetCurSel();

	return true;
}

void CPDFToBmpDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// --- save values for next time ---
	CFile f;
	CFileException ex;
	TCHAR* pszFileName = _T("LastSet");
	if( f.Open( pszFileName , CFile::modeCreate | CFile::modeWrite, &ex)){
		f.Write( &m_options, sizeof( m_options));
		f.Close();
	}

	if (m_convManager)
	{
		delete m_convManager;
		m_convManager = NULL;
	}

	if (m_sokManager)
	{
		m_sokManager->UnInit();
		delete m_sokManager;
		m_sokManager = NULL;
	}

	if (m_ExitDlg)
	{
		delete m_ExitDlg;
		m_ExitDlg = NULL;
	}

	m_TrayIcon.RemoveIcon();
}


void CPDFToBmpDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == ID_TIMER_CONVERT){

		int status = m_convManager->getConvResult();

		if (m_prevStatus != status || status != CRS_STARTED)
			UpdateConvertResult(status);

		m_prevStatus = status;

		if (status == CRS_STARTED)
		{
			int percent = m_convManager->getConvPercent();
			if (percent <= 0)
				percent = 1;

			if (percent > 100)
				percent = 100;

			DWORD dwElapsed = (GetTickCount() - m_dwStartTick) / 1000;
			CString strTime = L"";
			strTime.Format(L"Elapsed: %.2d:%.2d", dwElapsed / 60, dwElapsed % 60);
			m_stcElapsedTime->SetWindowText(strTime);
			m_progConvert->SetPos(percent);

			strTime.Format(L"%d %%", percent);
			m_stcConvertedPage->SetWindowText(strTime);
		}
	}
	else if (nIDEvent == ID_TIMER_SAVE){
		m_sokManager->InitParameter( m_options);	// update socket with values
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CPDFToBmpDlg::UpdateConvertResult(int status)
{
	if (status == CRS_STARTED)
		return;

	if (status == CRS_STOPED)
	{
		m_curItem.status = FCS_STOPED;
		KillTimer(ID_TIMER_CONVERT);
		m_butConvert->SetWindowText(L"Convert");
		return;
	}

	if (status == CRS_CONVERSION_ERROR) {
		m_curItem.status = FCS_CONV_ERRORED;
	} else if (status == CRS_NOTEXISTFILE_ERROR) {
		m_curItem.status = FCS_FILE_ERRORED;
	} else if (status == CRS_SKIPPED) {
		m_curItem.status = FCS_SKIPPED;
	} else{
		m_curItem.status = FCS_FINISHED;
		
		KillTimer(1);
		m_butConvert->SetWindowText(L"Convert");
		m_runningEngine = false;
		m_progConvert->SetPos(0);
		m_stcConvertedPage->SetWindowText(L"");

		MessageBox(L"Conversion Finished", L"PDF2BMP");
	}
}

bool CPDFToBmpDlg::isValidPage(CString strPages)
{
	if (strPages == L"All")
		return true;

	CString totalpageStr = strPages;
	int totalPage = m_curItem.pageCount;
	int convValue = 0;

	while (totalpageStr != "")
	{
		int nCommaPos = totalpageStr.Find(L",");

		CString pageStr = L"";
		if (nCommaPos != -1)
		{
			pageStr = totalpageStr.Left(nCommaPos);
			totalpageStr = totalpageStr.Right(totalpageStr.GetLength() - nCommaPos - 1);
		}
		else
		{
			pageStr = totalpageStr;
			totalpageStr = L"";
		}

		pageStr = pageStr.Trim();
		int nSlashPos = pageStr.Find(L"-");
		if (nSlashPos != -1)
		{
			CString strFrom = pageStr.Left(nSlashPos);
			CString strTo = pageStr.Right(pageStr.GetLength() - nSlashPos - 1);

			convValue = _wtoi(strFrom);
			if (convValue <= 0 || convValue > totalPage)
				return false;

			convValue = _wtoi(strTo);
			if (convValue <= 0 || convValue > totalPage)
				return false;
		}
		else
		{
			convValue = _wtoi(pageStr);
			if (convValue <= 0 || convValue > totalPage)
				return false;
		}
	}

	return true;
}

bool CPDFToBmpDlg::testWriteFile()
{
	CString filename = L"";
	filename = m_strDestPath + L"_testfile.txt";

	_wmkdir(m_strDestPath);

	CFile cf;
	if (cf.Open(filename, CFile::modeCreate | CFile::modeWrite) == FALSE)
		return false;

	cf.Close();
	if (DeleteFile(filename) == FALSE)
		return false;

	return true;
}

LRESULT CPDFToBmpDlg::OnTrayNotification(WPARAM wParam, LPARAM lParam)
{
	CMenu pMenu, *pSubMenu;

	if (LOWORD(lParam) == WM_LBUTTONDOWN)
	{
		OnTrayOpen();
	} else if(LOWORD(lParam) == WM_RBUTTONDOWN) {
		if (!(pMenu.LoadMenu(IDR_TRAYMENU))) return 0;
		if (!(pSubMenu = pMenu.GetSubMenu(0))) return 0;
		CPoint pos;
		GetCursorPos(&pos);
		pSubMenu->TrackPopupMenu(TPM_RIGHTALIGN,pos.x, pos.y,this);
		pMenu.DestroyMenu();
	}
	return 0;
}

LRESULT CPDFToBmpDlg::OnClientNotification(WPARAM wParam, LPARAM lParam)
{
	CString itemStr = L"", timeStr = L"";
	int index = 0;
	int messageType = (int)wParam;
	wchar_t* paramStr = (wchar_t*)lParam;
	wchar_t* tokStr = wcschr(paramStr, L'\t'), *tokStr1;
	CLIENT_INFO* pClientInfo = NULL;

	int itemCnt = m_ListReport.GetItemCount();
	int clientCnt = m_ListClient.GetCount();

	DateTime currTime = DateTime::now();
	SYSTEMTIME st;
	currTime.toUtcSystemTime(&st);

	timeStr.Format(_T("%.4d-%.2d-%.2d %.2d:%.2d:%.2d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	switch (messageType)
	{
	case CLIENT_MSG_CONNECTED:
		m_ListClient.AddString(paramStr);
		m_ListReport.InsertItem(itemCnt, paramStr);
		m_ListReport.SetItemText(itemCnt, 1, L"Client Connected");
		m_ListReport.SetItemText(itemCnt, 2, timeStr);
		break;
	case CLIENT_MSG_DISCONNECTED:
		for (int i = 0; i < clientCnt; i++)
		{
			m_ListClient.GetText(i, itemStr);

			if (itemStr == paramStr)
			{
				m_ListClient.DeleteString(i);
				break;
			}
		}

		m_ListReport.InsertItem(itemCnt, paramStr);
		m_ListReport.SetItemText(itemCnt, 1, L"Client Disconnected");
		m_ListReport.SetItemText(itemCnt, 2, timeStr);
		PostQuitMessage(0); //??? vtt 2/4/2019
//		CDialog::OnClose();
		break;
	case CLIENT_MSG_RECEIVE:
	case CLIENT_MSG_SEND:
	case CLIENT_MSG_STATE:
		if( !m_licensed){
			MessageBox(L"You don't have a license. Please send d:/SendToGT.txt to markus@graphtech.us", L"Bad Boy");
		}
		if (tokStr == NULL)
			m_ListReport.InsertItem(itemCnt, paramStr, 0);
		else
		{
			wchar_t showStr[MAX_NAME_SIZE];
			int itemColumn = 1;
			memset(showStr, 0, sizeof(showStr));
			wcsncpy(showStr, paramStr, tokStr - paramStr);
			m_ListReport.InsertItem(itemCnt, showStr, 0);

			tokStr1 = tokStr + 1;
			tokStr = wcschr(tokStr1, L'\t');

			while (tokStr != NULL)
			{
				memset(showStr, 0, sizeof(showStr));
				wcsncpy(showStr, tokStr1, tokStr - tokStr1);
				m_ListReport.SetItemText(itemCnt, itemColumn++, showStr);

				tokStr1 = tokStr + 1;
				tokStr = wcschr(tokStr1, L'\t');
			}
		}

		m_ListReport.SetItemText(itemCnt, 2, timeStr);
		break;
	case CLIENT_MSG_SETINFO:
		pClientInfo = (CLIENT_INFO*)lParam;
		index = GetIndexFromAddr(pClientInfo->clientAddr);

		if (index != -1)
		{
			m_clientInfos[index] = *pClientInfo;
			UpdateUIInfos(index, false);
		}
	default:
		break;
	}

	m_ListReport.EnsureVisible(itemCnt, TRUE);
	return 0;
}

void CPDFToBmpDlg::OnClose() 
{
	int end=1;
//	int err;
//	FILE *fili;
//	struct savePar savePar;
	// --- end the program ??? ---
	PostQuitMessage(0); //??? vtt 2/4/2019

/*	err = fopen_s( &fili, "d:/pdf/savePar.dat", "wb");
	if ( !err){
		savePar.fSizeWidth = m_nResWidth;
		savePar.fSizeHeight = m_nResHeight;
		savePar.nThreadCnt = m_nThreadCnt;
		savePar.nResWidth = m_fSizeWidth;
		savePar.nResHeight = m_fSizeHeight;
		fwrite( &savePar, sizeof (unsigned char), sizeof( struct savePar), fili);
		fclose( fili);
	}
*/
	CDialog::OnClose();
/*
	// Get setting values from Registry 
	m_ExitDlg->GetValueFromRegistry();

	// Check if the permanent flag of minimize/exit action is set true 
	if (m_ExitDlg->m_Permanent == TRUE) {
		if (m_ExitDlg->m_SelectedOpt == TRUE) {	//Minimize option
			this->ShowWindow(SW_HIDE);
		} else {	//Exit Option
			PostQuitMessage(0); // vtt 2/4/2019
			CDialog::OnClose();
		}
	} else {
		if (m_ExitDlg->DoModal() == IDOK) {
			m_ExitDlg->AddValueToRegistry();
			if (m_ExitDlg->m_SelectedOpt == TRUE) {	//Minimize option
				this->ShowWindow(SW_HIDE);
			} else if (m_ExitDlg->m_SelectedOpt == FALSE) {	//Exit option
				PostQuitMessage(0); // vtt 2/4/2019
				CDialog::OnClose();
			}
		}
	}
*/
}

void CPDFToBmpDlg::OnTrayOpen()
{
	::ShowWindow(this->m_hWnd,SW_NORMAL);
	::RedrawWindow(this->m_hWnd,NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME | RDW_INVALIDATE | RDW_ERASE);
	::SetForegroundWindow(this->m_hWnd);
	::ShowWindow(this->m_hWnd, SW_RESTORE);
	::SetWindowPos(this->m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	Sleep(50);
	::SetWindowPos(this->m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
	::SetFocus(this->m_hWnd);	
}

void CPDFToBmpDlg::OnTrayExit()
{
	PostQuitMessage(0);
	//DestroyWindow();
}

void CPDFToBmpDlg::UpdateUIInfos(int index, bool bUpdate)
{
	bool bCheckUpdate = bUpdate;
	if (index < 0)
		return;

	if (bCheckUpdate == false)
	{
		CString strAddress = L"";
		int selIndex = -1;
		selIndex = m_ListClient.GetCurSel();

		if (selIndex >= 0)
		{
			m_ListClient.GetText(selIndex, strAddress);
			if (strAddress == m_clientInfos[index].clientAddr)
			{
				bCheckUpdate = true;
			}
		}
	}

	if (bCheckUpdate)
	{
		UpdateData(TRUE);

		m_strDestPath = m_clientInfos[index].convInfo.dstPath;
		m_strSrcPath = m_clientInfos[index].convInfo.srcPath;
		
		m_nResWidth = (int)m_clientInfos[index].convOpts.resolutionX;
		m_nResHeight = (int)m_clientInfos[index].convOpts.resolutionY;
		m_nThreadCnt = m_clientInfos[index].convOpts.threadCnt;

		wchar_t printedPages[MAX_NAME_SIZE];
		memset(printedPages, 0, sizeof(printedPages));
		
		m_strPrintedPages = L"";
		MultiByteToWideChar(CP_ACP, 0, m_clientInfos[index].convInfo.selectPages, strlen(m_clientInfos[index].convInfo.selectPages), printedPages, MAX_NAME_SIZE);
		m_strPrintedPages = printedPages;
		UpdateData(FALSE);
	}
}

int CPDFToBmpDlg::GetIndexFromAddr(wchar_t* clientAddr)
{
	if (clientAddr == NULL)
		return -1;

	int ret = -1;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (wcscmp(m_clientInfos[i].clientAddr, clientAddr) == 0)
		{
			ret = i;
			break;
		}

		if (wcscmp(m_clientInfos[i].clientAddr, L"") == 0 && ret == -1)
		{
			ret = i;
		}
	}

	return ret;
}

void CPDFToBmpDlg::OnLbnSelchangeListClient()
{
	CString strAddress = L"";

	int selIndex = -1;
	selIndex = m_ListClient.GetCurSel();

	if (selIndex >= 0)
	{
		m_ListClient.GetText(selIndex, strAddress);
		UpdateUIInfos(GetIndexFromAddr((LPWSTR)(LPCWSTR)strAddress), true);
	}
}


void CPDFToBmpDlg::OnEnChangeEnThreadCnt()
{
	UpdateData();

	if (m_nThreadCnt <= 0 || m_nThreadCnt > 5)
	{
		m_nThreadCnt = 1;
		UpdateData(FALSE);
	}
	m_options.threadCnt = m_nThreadCnt;
}


void CPDFToBmpDlg::OnEnChangeEnResWidth()
{
	UpdateData();
	if (m_nResWidth <= 0)
	{
		m_nResWidth = (int)PDF_RESOLUTION_600;
		UpdateData(FALSE);
	}
	m_options.resolutionX = (float)m_nResWidth;
}


void CPDFToBmpDlg::OnEnChangeEnResHeight()
{
	UpdateData();
	if (m_nResHeight <= 0)
	{
		m_nResHeight = (int)PDF_RESOLUTION_600;
		UpdateData(FALSE);
	}
	m_options.resolutionY = (float)m_nResHeight;
}


int CPDFToBmpDlg::GetMACaddress()
{
	FILE *fili;
	CString txt;
	char filename[ 500];
	char pcname[ 500];
	char str[ 500], dStr[ 500];
	char mac[ 20];
	char mac2[ 20];
	char lastDate[ 20];
	int i, line, err;
	int actDate, intDate;
	CTime date = CTime::GetCurrentTime();
	struct tm osDate; 
	date.GetLocalTm(&osDate);

	actDate = 1900+osDate.tm_year;
	actDate += osDate.tm_yday;

	line=0;
	mac[0]=0;
	strcpy( str, "GetMAC > d:/pdf/PDF2BMPmac.txt");
	system( str);
	Sleep(100);	// give it time
	err = fopen_s( &fili, "d:/pdf/PDF2BMPmac.txt", "rb");
	if ( !err){
		fread( str, sizeof (unsigned char), sizeof( str), fili);
		mac[0]=mac2[0]=0;
		// --- go to 3rd line ---
		for ( i=0; i< (int)strlen( str); i++){
			if ( str[i] == 0x0a){
				line++;
			}
			if ( line == 3 && mac[0]==0){
				strncpy_s( mac, &str[ i+1], 17);
				mac[17]=0;
//				break;
			}
			if ( line == 4 && mac2[0]==0){
				strncpy_s( mac2, &str[ i+1], 17);
				mac2[17]=0;
				break;
			}
		}
		fclose( fili);
	}
	else{
		MessageBox( L"Please make a D partition (run as Admin)", L"No D partition or no rights to write D. Make it pdf, small letters");
	}
	LPTSTR lpszSystemInfo;
	DWORD cchBuff = 256;
	TCHAR tchBuffer2[1000];  

	lpszSystemInfo = tchBuffer2;

	GetComputerName( lpszSystemInfo, &cchBuff); 
//MessageBox( lpszSystemInfo, L"Bad Boy");

	wchar_to_char( (char*)pcname, (wchar_t*)lpszSystemInfo, 256);
	
	// --- write  ---
	fili = fopen ( "d:/SendToGT.txt", "wb");
	if ( fili != NULL){
		fwrite( pcname, sizeof (unsigned char), strlen( pcname), fili);
		sprintf( str, "%c%c", 0x0d, 0x0a); fwrite( str, sizeof (unsigned char), 2, fili);
		fwrite( mac, sizeof (unsigned char), strlen( mac), fili);
		sprintf( str, "%c%c", 0x0d, 0x0a); fwrite( str, sizeof (unsigned char), 2, fili);
		fclose( fili);
	}

	// --- check if license file there ---
//	sprintf( filename, "d:/License.%s", pcname);
//	sprintf( filename, "License.%s", pcname);
	sprintf( filename, "d:/pdf/License.%s", pcname);
	fili = fopen ( filename, "r+b");
	if ( fili != NULL){
		fread( str, sizeof (unsigned char), sizeof( str), fili);
		fclose( fili);
		decrypt( 0, (unsigned char *) str, 500, 0); 
		for ( i=0; i<500; i++) 
			dStr[ i] = toupper( str[ i]);
		for ( i=0; i<500; i++) 
			pcname[ i] = toupper( pcname[ i]);
		memcpy( lastDate, &dStr[ 80], 5);
		intDate = atoi( lastDate);
/*		if ( intDate != 0 && actDate < intDate){
			MessageBox( L"Setting the PC clock back does not help. contact www.graphtech.us", L"Bad license");
			return false;
		}
		memcpy( lastDate, &dStr[ 0], 5);
		intDate = atoi( lastDate);
		if ( intDate != 0 && actDate > intDate){
			MessageBox( L"Trial time over, license not valid anymore. contact www.graphtech.us", L"Bad license");
			return false;
		}
*/
		sprintf( lastDate, "%05d", actDate);
		memcpy( &dStr[ 80], lastDate, 5);
		memcpy( str, dStr, 500);
		encrypt( 0, (unsigned char *) str, 500, 0); 
		fili = fopen ( filename, "wb");
		if ( fili != NULL){
			err=fwrite( str, sizeof (unsigned char), 1000, fili);
			decrypt( 0, (unsigned char *) str, 500, 0); 
			fclose( fili);
		}
		if ( !strcmp( pcname, &dStr[ 133])){		// check PC name
//			return true;
//			if ( !strncmp( mac, &dStr[ 100], 17)){	// check mac
			if ( !strncmp( mac, &dStr[ 100], 12) || !strncmp( mac2, &dStr[ 100], 12)){	// check mac
				return true;
			}
			else{
				sprintf( str, "Wrong license (MAC [%s][%s] vs %s). contact www.graphtech.us", mac, mac2, &dStr[ 100]);
				txt = str;
				MessageBox( txt, L"Bad license");
				return true;
			}
		}
		else{
			sprintf( str, "Wrong license (PC name %s vs %s). contact www.graphtech.us", pcname, &dStr[ 133]);
			txt = str;
			MessageBox( txt, L"Bad license");
			return false;
		}
	}
	else{
		sprintf( str, "No license file [%s]. contact www.graphtech.us", filename);
		txt = str;
		MessageBox( txt, L"Bad license");
	}

	return false;
}


void CPDFToBmpDlg::wchar_to_char(char *pansi, wchar_t *puni, int len)
{
	while (len>1)
	{
		if (*puni<0x100) *pansi=(UCHAR)*puni;
		else *pansi='?';
		if (*puni==0) break;
		pansi++;
		len--;
		puni++;
	}
	*pansi=0;
}

/*--- encrypt ----------------------------------------------------------------------*/

void CPDFToBmpDlg::encrypt(int pos, unsigned char *data, int len, int key)
{
// pos : file position
// data: the data
// len : len of data to be encrypted
// key : encryption key
	
	UCHAR dat;
	int   i, j, k; // k=pos+key+len

	len--;
	k=pos+key+len;
	while(len>=0) {
		dat = data[len] ^ keytable[k%MAX_ROWS][8] ^ k;
		data[len]=0;
		for (i=0; i<8; i++) {
			if (dat&1) {
				for (j=0; j<8; j++) {
					if (keytable[k%MAX_ROWS][j]==i) {
						data[len] |= (1<<j);
					}
				}
			}
			dat>>=1;
		}
		len--; k--;
	}
} // end encrypt


/*--- decrypt ----------------------------------------------------------------------*/

void CPDFToBmpDlg::decrypt(int pos, unsigned char *data, int len, int key)
{
	UCHAR dat;
	int   i, k; // k=pos+key+len

	len--;
	k=pos+key+len;
	while(len>=0) {
		dat = data[len];
		data[len]=0;
		for (i=0; i<8; i++) {
			if (dat&1) {
				data[len] |= (1<<keytable[k%MAX_ROWS][i]);
			}
			dat>>=1;
		}
		data[len] = data[len] ^ k ^ keytable[k%MAX_ROWS][8];
		len--; k--;
	};
}



void CPDFToBmpDlg::OnChangeEnY1()
{
	UpdateData();
	m_options.pY[0]=m_pY[0];
}


void CPDFToBmpDlg::OnChangeEnY2()
{	
	UpdateData();
	m_options.pY[1]=m_pY[1];
}


void CPDFToBmpDlg::OnChangeEnY3()
{
	UpdateData();
	m_options.pY[2]=m_pY[2];
}


void CPDFToBmpDlg::OnChangeEnY4()
{
	UpdateData();
	m_options.pY[3]=m_pY[3];
}


void CPDFToBmpDlg::OnChangeEnY5()
{
	UpdateData();
	m_options.pY[4]=m_pY[4];
}


void CPDFToBmpDlg::OnSelchangeComboMode()
{
	m_options.convertmode = m_comboMode.GetCurSel();
}

void CPDFToBmpDlg::OnSelchangeComboSplit()
{
	m_options.convertsplit = m_comboSplit.GetCurSel();
}


void CPDFToBmpDlg::OnEnChangeEnSizeWidth()
{
	UpdateData();
	if (m_fSizeWidth <= 0)
	{
		m_fSizeWidth = 0;
		UpdateData(FALSE);
	}
	m_options.sizeWidth = m_fSizeWidth;
}


void CPDFToBmpDlg::OnEnChangeEnSizeHeight()
{
	UpdateData();
	if (m_fSizeHeight <= 0)
	{
		m_fSizeHeight = 0;
		UpdateData(FALSE);
	}
	m_options.sizeHeight = m_fSizeHeight;
}


void CPDFToBmpDlg::OnBnClickedResetlist()
{
	m_ListReport.DeleteAllItems();
}
