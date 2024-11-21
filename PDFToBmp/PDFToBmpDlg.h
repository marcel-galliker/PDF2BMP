
// PDFToBmpDlg.h : header file
//
#include "convmanager.h"
#include "TrayIcon.h"
#include "PDFExitDlg.h"
#include "../sock/sokmanager.h"
#include "afxwin.h"
#include "afxcmn.h"

#define WM_ICON_NOTIFY WM_USER

	#define MAX_ROWS 13

static int keytable[MAX_ROWS][9] = 
{	
	/* 00 */ {6,5,2,3,0,7,1,4, 0xff},
	/* 01 */ {1,0,3,7,6,5,2,4, 0xdf},
	/* 02 */ {7,4,2,5,6,0,3,1, 0xc1},
	/* 03 */ {4,2,5,1,3,6,0,7, 0xff},
	/* 04 */ {3,4,0,5,1,6,2,7, 0xe0},
	/* 05 */ {0,3,7,1,4,2,6,5, 0xff},
	/* 06 */ {1,3,0,5,4,6,2,7, 0x20},
	/* 07 */ {1,7,2,0,3,4,6,5, 0xd4},
	/* 08 */ {6,4,3,5,0,2,1,7, 0xb6},
	/* 09 */ {0,1,4,6,7,2,3,5, 0xed},
	/* 10 */ {1,2,6,5,3,0,7,4, 0x90},
	/* 11 */ {6,7,4,0,3,5,1,2, 0xaf},
	/* 12 */ {1,2,3,5,0,6,4,7, 0x10} 
};


#pragma once

// CPDFToBmpDlg dialog
class CPDFToBmpDlg : public CDialogEx
{
// Construction
public:
	CPDFToBmpDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_PDFTOBMP_DIALOG };

	LRESULT OnTrayNotification(WPARAM wParam, LPARAM lParam);
	LRESULT OnClientNotification(WPARAM wParam, LPARAM lParam);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
public:
	ConvManager     *m_convManager;
	sokmanager		*m_sokManager;

	CTrayIcon		m_TrayIcon;	//To show tray icon (add by CSC)
	CPDFExitDlg		*m_ExitDlg;

	int				m_licensed;
	char			m_mac[ 100];
	int				m_maclen;

	bool			m_runningEngine;
	ConvFileInfo	m_curItem;

	ConvOptions		m_options;
	int				m_prevStatus;

	DWORD			m_dwStartTick;

	CLIENT_INFO		m_clientInfos[MAX_CLIENTS];

private:
	void	ConvertItem();
	bool	SetConvertOptions(wchar_t* srcPath);
	void	UpdateConvertResult(int status);
	bool	isValidPage(CString strPages);
	bool	testWriteFile();

	void	UpdateUIInfos(int index, bool bUpdate);
	int		GetIndexFromAddr(wchar_t* clientAddr);
	int		GetMACaddress( );
	void	wchar_to_char(char *pansi, wchar_t *puni, int len);
	void	encrypt(int pos, unsigned char *data, int len, int key);
	void	decrypt(int pos, unsigned char *data, int len, int key);

protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButBrowSrc();
	afx_msg void OnBnClickedButBrowDst();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnTrayOpen();
	afx_msg void OnTrayExit();
	afx_msg void OnClose();

public:
	CButton* m_butConvert;
	CString m_strSrcPath;
	CString m_strDestPath;
	CString m_strPrintedPages;
	CProgressCtrl* m_progConvert;
	CStatic* m_stcElapsedTime;
	CStatic* m_stcConvertedPage;

	int m_nResWidth;
	int m_nResHeight;

	float m_fSizeWidth;
	float m_fSizeHeight;
	
	float m_pY[5];

	struct savePar{
		int nResWidth;
		int nResHeight;
		int nThreadCnt;
		float fSizeWidth;
		float fSizeHeight;
	};

	int m_nThreadCnt;
	CListBox m_ListClient;
	CListCtrl m_ListReport;
	afx_msg void OnLbnSelchangeListClient();
	afx_msg void OnEnChangeEnThreadCnt();
	afx_msg void OnEnChangeEnResWidth();
	afx_msg void OnEnChangeEnResHeight();
	CComboBox m_comboMode;
	CComboBox m_comboSplit;
	afx_msg void OnChangeEnY1();
	afx_msg void OnChangeEnY2();
	afx_msg void OnChangeEnY3();
	afx_msg void OnChangeEnY4();
	afx_msg void OnChangeEnY5();
	afx_msg void OnSelchangeComboMode();
	afx_msg void OnSelchangeComboSplit();
	afx_msg void OnEnChangeEnSizeWidth();
	afx_msg void OnEnChangeEnSizeHeight();
	afx_msg void OnBnClickedResetlist();
};
