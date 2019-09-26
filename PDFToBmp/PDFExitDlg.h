#pragma once


// CPDFExitDlg dialog

class CPDFExitDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPDFExitDlg)

public:
	CPDFExitDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPDFExitDlg();
	int m_Permanent;
	int m_SelectedOpt;

	void GetValueFromRegistry();
	void AddValueToRegistry();

// Dialog Data
	enum { IDD = IDD_CONFIRM_EXIT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();

	afx_msg void OnBnClickedRadMinimize();
	afx_msg void OnBnClickedRadExit();
	afx_msg void OnBnClickedCheck1();
};
