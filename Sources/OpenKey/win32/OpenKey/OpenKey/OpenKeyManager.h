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
#pragma once
#include "stdafx.h"

class OpenKeyManager {
public:
	static unsigned short _lastKeyCode;
	static vector<LPCTSTR>& getInputType();
	static vector<LPCTSTR>& getTableCode();

	static void initEngine();
	static void freeEngine();

	static bool checkUpdate(string& newVersion);

	static void createDesktopShortcut();
};