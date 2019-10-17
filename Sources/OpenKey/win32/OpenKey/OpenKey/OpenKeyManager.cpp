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
#include <shlobj.h>

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
	versionCode = (versionCode << 16) | (versionCode & 0x00FF00) | (versionCode >> 16 & 0xFF);
	currentVersion = (currentVersion << 16) | (currentVersion & 0x00FF00) | (currentVersion >> 16 & 0xFF);
	if (versionCode > currentVersion) {
		return true;
	}
	return false;
}

void OpenKeyManager::createDesktopShortcut() {
	CoInitialize(NULL);
	IShellLink* pShellLink = NULL;
	HRESULT hres;
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_ALL,
							IID_IShellLink, (void**)&pShellLink);
	if (SUCCEEDED(hres)) {
		wstring path = OpenKeyHelper::getFullPath();
		pShellLink->SetPath(path.c_str());
		pShellLink->SetDescription(_T("OpenKey - Bộ gõ Tiếng Việt"));
		pShellLink->SetIconLocation(path.c_str(), 0);

		IPersistFile* pPersistFile;
		hres = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);

		if (SUCCEEDED(hres)) {
			wchar_t desktopPath[MAX_PATH + 1];
			wchar_t savePath[MAX_PATH + 10];
			SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath);
			wsprintf(savePath, _T("%s\\OpenKey.lnk"), desktopPath);
			hres = pPersistFile->Save(savePath, TRUE);
			pPersistFile->Release();
			pShellLink->Release();
		}
	}
}
