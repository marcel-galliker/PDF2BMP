// TrayIcon.cpp: implementation of the CTrayIcon class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TrayIcon.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTrayIcon::CTrayIcon()
{

}

CTrayIcon::~CTrayIcon()
{

}

BOOL CTrayIcon::Create(CWnd *pWnd, UINT uCallbMessage, LPCTSTR szToolTip, HICON icon, UINT uID)
{
	m_tnd.cbSize = sizeof(NOTIFYICONDATA);
	m_tnd.hWnd = pWnd->GetSafeHwnd();
	m_tnd.uID = uID;
	m_tnd.hIcon = icon;
	m_tnd.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP;
	m_tnd.uCallbackMessage = uCallbMessage;
	lstrcpy(m_tnd.szTip, szToolTip);

	return Shell_NotifyIcon(NIM_ADD, &m_tnd);
}

BOOL CTrayIcon::SetIcon(UINT nIDResource)
{
	HICON hIcon = AfxGetApp()->LoadIcon(nIDResource);
	return SetIcon(hIcon);
}

BOOL CTrayIcon::SetIcon(HICON hIcon)
{
	m_tnd.uFlags = NIF_ICON;
	m_tnd.hIcon = hIcon;
	return Shell_NotifyIcon(NIM_MODIFY, &m_tnd);
}

BOOL CTrayIcon::SetTooltipText(LPCTSTR pszTip)
{
	return FALSE;
}

void CTrayIcon::RemoveIcon()
{
	m_tnd.uFlags=0;
	Shell_NotifyIcon(NIM_DELETE, &m_tnd);
}
