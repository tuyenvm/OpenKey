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
#include "OpenKeyManager.h"

static vector<LPCTSTR> _inputType = {
	_T("Telex"),
	_T("VNI"),
	_T("Simple Telex"),
};

static vector<LPCTSTR> _tableCode = {
	_T("Unicode"),
	_T("TCVN3 (ABC)"),
	_T("VNI Windows"),
	_T("Unicode Tổ hợp"),
	_T("Vietnamese Locale CP 1258")
};

/*-----------------------------------------------------------------------*/

extern void OpenKeyInit();
extern void OpenKeyFree();

unsigned short  OpenKeyManager::_lastKeyCode = 0;

vector<LPCTSTR>& OpenKeyManager::getInputType() {
	return _inputType;
}

vector<LPCTSTR>& OpenKeyManager::getTableCode() {
	return _tableCode;
}

void OpenKeyManager::initEngine() {
	OpenKeyInit();
}

void OpenKeyManager::freeEngine() {
	OpenKeyFree();
}

bool OpenKeyManager::checkUpdate(string& newVersion) {
	wstring dataW = OpenKeyHelper::getContentOfUrl(L"https://raw.githubusercontent.com/tuyenvm/OpenKey/master/version.json");
	string data = wideStringToUtf8(dataW);

	//simple parse
	data = data.substr(data.find("latestWinVersion"));
	data = data.substr(data.find("\"versionName\":"));
	data = data.substr(14);
	data = data.substr(data.find("\""));
	data = data.substr(1);
	string versionName = data.substr(0, data.find("\""));
	newVersion = versionName;
	data = data.substr(data.find("\"versionCode\":"));
	data = data.substr(14);
	string versionCodeString = data.substr(0, data.find("}"));
	DWORD versionCode = (DWORD)atoi(versionCodeString.c_str());
	DWORD currentVersion = OpenKeyHelper::getVersionNumber();
	if ((versionCode & 0xFF) > (currentVersion & 0xFF) ||
		(versionCode>>8 & 0xFF) > (currentVersion>>8 & 0xFF) ||
		(versionCode>>16 & 0xFF) > (currentVersion>>16 & 0xFF)) {
		return true;
	}
	return false;
}
