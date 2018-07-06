/*
* Copyright (c) 2013-2018 Vladimir Alemasov
* All rights reserved
*
* This program and the accompanying materials are distributed under 
* the terms of GNU General Public License version 2 
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "stdafx.h"

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>

#include "resource.h"
#include "MainDlg.h"

CAppModule _Module;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpstrCmdLine, int nCmdShow)
{
	WSADATA data;

	::CoInitialize(NULL);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);

	WSAStartup(MAKEWORD(2, 2), &data);

	CMessageLoop theLoop;
	_Module.Init(NULL, hInstance);
	_Module.AddMessageLoop(&theLoop);

	int nRet = 0;
	{
		CMainDlg dlgMain;
		dlgMain.Create(NULL);
		dlgMain.ShowWindow(nCmdShow);
		int nRet = theLoop.Run();
	}

	_Module.RemoveMessageLoop();
	_Module.Term();

	WSACleanup();

	::CoUninitialize();

	return nRet;
}
