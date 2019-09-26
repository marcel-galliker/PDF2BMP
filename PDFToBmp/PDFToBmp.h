
// PDFToBmp.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CPDFToBmpApp:
// See PDFToBmp.cpp for the implementation of this class
//

class CPDFToBmpApp : public CWinApp
{
public:
	CPDFToBmpApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CPDFToBmpApp theApp;