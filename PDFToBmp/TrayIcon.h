// TrayIcon.h: interface for the CTrayIcon class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TRAYICON_H__23B2B751_E9FA_4FCF_89FE_2A5281805299__INCLUDED_)
#define AFX_TRAYICON_H__23B2B751_E9FA_4FCF_89FE_2A5281805299__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CTrayIcon  
{
public:
	NOTIFYICONDATA m_tnd;
public:
	void RemoveIcon();
	BOOL SetTooltipText(LPCTSTR pszTip);
	CTrayIcon();
	virtual ~CTrayIcon();
	BOOL Create(CWnd *pWnd, UINT uCallbMessage, LPCTSTR szToolTip, HICON icon, UINT uID);
	BOOL SetIcon(HICON hIcon);
	BOOL SetIcon(LPCTSTR lpszIconName);
	BOOL SetIcon(UINT nIDResource);
};

#endif // !defined(AFX_TRAYICON_H__23B2B751_E9FA_4FCF_89FE_2A5281805299__INCLUDED_)
