/*----------------------------------------------------------
OpenKey - The Cross platform Open source Vietnamese Keyboard application.

Copyright (C) 2019 Mai Vu Tuyen
Contact: maivutuyen.91@gmail.com
Github: https://github.com/tuyenvm/OpenKey
Fanpage: https://www.facebook.com/OpenKeyVN

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#include "stdafx.h"
#include "AppDelegate.h"
#include <Shlobj.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
						_In_opt_ HINSTANCE hPrevInstance,
						_In_ LPWSTR    lpCmdLine,
						_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	
#if NDEBUG
	//check the program is run as administrator mode
	APP_GET_DATA(vRunAsAdmin, 0);
	if (vRunAsAdmin && !IsUserAnAdmin()) {
		//create admin process
		ShellExecute(0, L"runas", OpenKeyHelper::getFullPath().c_str(), 0, 0, SW_SHOWNORMAL);
		return 1;
	}
#endif
	AppDelegate app;
	return app.run(hInstance);
}