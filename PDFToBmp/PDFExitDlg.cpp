// PDFExitDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PDFToBmp.h"
#include "PDFExitDlg.h"
#include "afxdialogex.h"


// CPDFExitDlg dialog

#define REGBUF 30

IMPLEMENT_DYNAMIC(CPDFExitDlg, CDialogEx)

CPDFExitDlg::CPDFExitDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPDFExitDlg::IDD, pParent)
{

}

CPDFExitDlg::~CPDFExitDlg()
{
}

void CPDFExitDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}



BEGIN_MESSAGE_MAP(CPDFExitDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CPDFExitDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_RAD_MINIMIZE, &CPDFExitDlg::OnBnClickedRadMinimize)
	ON_BN_CLICKED(IDC_RAD_EXIT, &CPDFExitDlg::OnBnClickedRadExit)
	ON_BN_CLICKED(IDC_CHECK1, &CPDFExitDlg::OnBnClickedCheck1)
END_MESSAGE_MAP()


// CPDFExitDlg message handlers

void CPDFExitDlg::OnBnClickedOk()
{

	CDialogEx::OnOK();
}

BOOL CPDFExitDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	
	//m_SelectedOpt = TRUE;
	if (m_SelectedOpt == TRUE) {
		((CButton*)GetDlgItem(IDC_RAD_MINIMIZE))->SetCheck(TRUE);
	} else {
		((CButton*)GetDlgItem(IDC_RAD_EXIT))->SetCheck(TRUE);
	}
	

	((CButton*)GetDlgItem(IDC_PERMENANT))->SetCheck(m_Permanent);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPDFExitDlg::OnBnClickedRadMinimize()
{
	m_SelectedOpt = TRUE;
}

void CPDFExitDlg::OnBnClickedRadExit()
{
	m_SelectedOpt = FALSE;
}

void CPDFExitDlg::AddValueToRegistry()
{
	HKEY hk; 
	char reg_permanent[REGBUF], reg_minimize[REGBUF];

	// Add your source name as a subkey under the Application 
	// key in the EventLog registry key. 

	if (RegCreateKey(HKEY_LOCAL_MACHINE, L"SAM\\PDF2BMP", &hk)) {
//		MessageBox(L"registry failed.", L"PDF2BMP");
		return;
	}

	// Set the name of the message file. 
	memset(reg_permanent, '\0', REGBUF);
	memset(reg_minimize, '\0', REGBUF);

	strncpy(reg_permanent, itoa(m_Permanent, reg_permanent, 10), REGBUF);
	strncpy(reg_minimize, itoa(m_SelectedOpt, reg_minimize, 10), REGBUF);

	if (RegSetValueEx(hk,			// subkey handle 
		L"permanent",				// value name 
		0,							// must be zero 
		REG_SZ,						// value type 
		(LPBYTE) &reg_permanent,	// pointer to value data 
		strlen(reg_permanent)))		// length of value data 
		return;

	if (RegSetValueEx(hk,
		L"minimize",
		0,
		REG_SZ,
		(LPBYTE) &reg_minimize,
		strlen(reg_minimize)))
		return;

	RegCloseKey(hk);
}

void CPDFExitDlg::GetValueFromRegistry()
{
	HKEY hk; 
	char reg_permanent[REGBUF], reg_minimize[REGBUF];
	DWORD size;
	DWORD dwType;
	//LONG size_l;

	if (RegCreateKey(HKEY_LOCAL_MACHINE, L"SAM\\PDF2BMP", &hk)) {
//		MessageBox(L"registry failed111.", L"PDF2BMP");
		return;
	}
	memset(reg_permanent, '\0', REGBUF);
	memset(reg_minimize, '\0', REGBUF);

	if (RegQueryValueEx(hk, L"permanent", NULL, &dwType, (LPBYTE)reg_permanent, &size) != ERROR_SUCCESS) {

	}

	if (RegQueryValueEx(hk, L"minimize", NULL, &dwType, (LPBYTE)reg_minimize, &size) != ERROR_SUCCESS) {

	}

	if (strlen(reg_permanent) > 0 && !strncmp(reg_permanent, "1", strlen(reg_permanent))) {
		m_Permanent = TRUE;
	} else {
		m_Permanent = FALSE;
	}

	if (strlen(reg_minimize) > 0 && !strncmp(reg_minimize, "0", strlen(reg_minimize))) {
		m_SelectedOpt = FALSE;	//Exit Option
	} else {
		m_SelectedOpt = TRUE;	//Minimize Option
	}

	RegCloseKey(hk);
}


void CPDFExitDlg::OnBnClickedCheck1()
{
	if (((CButton*)GetDlgItem(IDC_PERMENANT))->GetCheck() == TRUE) {
		m_Permanent = TRUE;
	} else {
		m_Permanent = FALSE;
	}
}
