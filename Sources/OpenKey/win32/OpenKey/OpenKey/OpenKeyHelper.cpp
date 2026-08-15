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
#include "OpenKeyHelper.h"
#include <stdarg.h>
#include <Urlmon.h>
#include <fstream>
#include <sstream>

#pragma comment(lib, "version.lib")
#pragma comment(lib, "Urlmon.lib")

static BYTE* _regData = 0;

static LPCTSTR sk = TEXT("SOFTWARE\\TuyenMai\\OpenKey");
static HKEY hKey;
static LPCTSTR _runOnStartupKeyPath = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
static TCHAR _executePath[MAX_PATH];
static bool _hasGetPath = false;
static bool _hasCheckedPortable = false;
static bool _isPortable = false;
static TCHAR _portablePath[MAX_PATH];

static DWORD _cacheProcessId = 0, _tempProcessId = 0;
static HWND _tempWnd;
static TCHAR _exePath[1024] = { 0 };
static LPCTSTR _exeName = _exePath;
static HANDLE _proc;
static string _exeNameUtf8 = "TheOpenKeyProject";
static string _unknownProgram = "UnknownProgram";

int CF_RTF = RegisterClipboardFormat(_T("Rich Text Format"));
int CF_HTML = RegisterClipboardFormat(_T("HTML Format"));
int CF_OPENKEY = RegisterClipboardFormat(_T("OpenKey Format"));

static LPCTSTR getPortablePath() {
	if (!_hasCheckedPortable) {
		lstrcpy(_portablePath, OpenKeyHelper::getExecutePath());
		TCHAR* slash = _tcsrchr(_portablePath, '\\');
		if (slash) {
			*(slash + 1) = 0;
			lstrcat(_portablePath, _T(".portable"));
			DWORD attr = GetFileAttributes(_portablePath);
			if (attr == INVALID_FILE_ATTRIBUTES) {
				CreateDirectory(_portablePath, NULL);
				attr = GetFileAttributes(_portablePath);
			}
			_isPortable = attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
		}
		_hasCheckedPortable = true;
	}
	return _portablePath;
}

static bool isPortableMode() {
	getPortablePath();
	return _isPortable;
}

static void getPortableConfigPath(LPCTSTR key, LPCTSTR ext, LPTSTR outPath) {
	lstrcpy(outPath, getPortablePath());
	lstrcat(outPath, _T("\\"));
	lstrcat(outPath, key);
	lstrcat(outPath, ext);
}

void OpenKeyHelper::openKey() {
	LONG nError = RegOpenKeyEx(HKEY_CURRENT_USER, sk, NULL, KEY_ALL_ACCESS, &hKey);
	if (nError == ERROR_FILE_NOT_FOUND) 	{
		nError = RegCreateKeyEx(HKEY_CURRENT_USER, sk, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, &hKey, NULL);
	}
	if (nError) {
		LOG(L"result %d\n", nError);
	}
}

void OpenKeyHelper::setRegInt(LPCTSTR key, const int & val) {
	if (isPortableMode()) {
		TCHAR path[MAX_PATH];
		getPortableConfigPath(_T("settings"), _T(".ini"), path);
		TCHAR value[32];
		wsprintf(value, _T("%d"), val);
		WritePrivateProfileString(_T("OpenKey"), key, value, path);
		return;
	}
	openKey();
	RegSetValueEx(hKey, key, 0, REG_DWORD, (LPBYTE)&val, sizeof(val));
	RegCloseKey(hKey);
}

int OpenKeyHelper::getRegInt(LPCTSTR key, const int & defaultValue) {
	if (isPortableMode()) {
		TCHAR path[MAX_PATH];
		getPortableConfigPath(_T("settings"), _T(".ini"), path);
		return GetPrivateProfileInt(_T("OpenKey"), key, defaultValue, path);
	}
	openKey();
	int val = defaultValue;
	DWORD size = sizeof(val);
	if (ERROR_SUCCESS != RegQueryValueEx(hKey, key, 0, 0, (LPBYTE)&val, &size)) {
		val = defaultValue;
	}
	RegCloseKey(hKey);
	return val;
}

void OpenKeyHelper::setRegBinary(LPCTSTR key, const BYTE * pData, const int & size) {
	if (isPortableMode()) {
		TCHAR path[MAX_PATH];
		getPortableConfigPath(key, _T(".bin"), path);
		HANDLE file = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file != INVALID_HANDLE_VALUE) {
			DWORD written = 0;
			WriteFile(file, pData, size, &written, NULL);
			CloseHandle(file);
		}
		return;
	}
	openKey();
	RegSetValueEx(hKey, key, 0, REG_BINARY, pData, size);
	RegCloseKey(hKey);
}

BYTE * OpenKeyHelper::getRegBinary(LPCTSTR key, DWORD& outSize) {
	if (isPortableMode()) {
		if (_regData) {
			delete[] _regData;
			_regData = NULL;
		}
		TCHAR path[MAX_PATH];
		getPortableConfigPath(key, _T(".bin"), path);
		HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE) {
			outSize = 0;
			return NULL;
		}
		DWORD size = GetFileSize(file, NULL);
		_regData = new BYTE[size];
		DWORD read = 0;
		if (!ReadFile(file, _regData, size, &read, NULL)) {
			delete[] _regData;
			_regData = NULL;
			size = 0;
		}
		CloseHandle(file);
		outSize = size;
		return _regData;
	}
	openKey();
	if (_regData) {
		delete[] _regData;
		_regData = NULL;
	}
	DWORD size = 0;
	RegQueryValueEx(hKey, key, 0, 0, 0, &size);
	_regData = new BYTE[size];
	if (ERROR_SUCCESS != RegQueryValueEx(hKey, key, 0, 0, _regData, &size)) {
		delete[] _regData;
		_regData = NULL;
	}
	outSize = size;
	RegCloseKey(hKey);
	return _regData;
}

void OpenKeyHelper::registerRunOnStartup(const int& val) {
	if (isPortableMode()) {
		return;
	}
	if (val) {
		if (vRunAsAdmin) {
			string path = wideStringToUtf8(getFullPath());
			char buff[MAX_PATH];
			sprintf_s(buff, "schtasks /create /sc onlogon /tn OpenKey /rl highest /tr \"%s\" /f", path.c_str());
			WinExec(buff, SW_HIDE);
		} else {
			RegOpenKeyEx(HKEY_CURRENT_USER, _runOnStartupKeyPath, NULL, KEY_ALL_ACCESS, &hKey);
			wstring path = getFullPath();
			RegSetValueEx(hKey, _T("OpenKey"), 0, REG_SZ, (byte*)path.c_str(), ((DWORD)path.size() + 1) * sizeof(TCHAR));
			RegCloseKey(hKey);
		}
	} else {
		RegOpenKeyEx(HKEY_CURRENT_USER, _runOnStartupKeyPath, NULL, KEY_ALL_ACCESS, &hKey);
		RegDeleteValue(hKey, _T("OpenKey"));
		RegCloseKey(hKey);
		WinExec("schtasks /delete  /tn OpenKey /f", SW_HIDE);
	}
}

LPTSTR OpenKeyHelper::getExecutePath() {
	if (!_hasGetPath) {
		HMODULE hModule = GetModuleHandleW(NULL);
		GetModuleFileNameW(hModule, _executePath, MAX_PATH);
		_hasGetPath = true;
	}
	return _executePath;
}

string& OpenKeyHelper::getFrontMostAppExecuteName() {
	_tempWnd = GetForegroundWindow();
	GetWindowThreadProcessId(_tempWnd, &_tempProcessId);
	if (_tempProcessId == _cacheProcessId) {
		return _exeNameUtf8;
	}
	_cacheProcessId = _tempProcessId;
	_proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, _tempProcessId);
	GetProcessImageFileName((HMODULE)_proc, _exePath, 1024);
	CloseHandle(_proc);
	
	if (wcscmp(_exePath, _T("")) == 0) {
		return _unknownProgram;
	}
	_exeName = _tcsrchr(_exePath, '\\') + 1;
	if (wcscmp(_exeName, _T("OpenKey64.exe")) == 0 ||
		wcscmp(_exeName, _T("OpenKey32.exe")) == 0 || 
		wcscmp(_exeName, _T("explorer.exe")) == 0) {
		return _exeNameUtf8;
	}
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, _exeName, (int)lstrlen(_exeName), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, _exeName, (int)lstrlen(_exeName), &strTo[0], size_needed, NULL, NULL);
	_exeNameUtf8 = strTo;
	//LOG(L"%s\n", utf8ToWideString(_exeNameUtf8).c_str());
	return _exeNameUtf8;
}

string & OpenKeyHelper::getLastAppExecuteName() {
	if (!vUseSmartSwitchKey)
		return getFrontMostAppExecuteName();
	return _exeNameUtf8;
}

wstring OpenKeyHelper::getFullPath() {
	HMODULE hModule = GetModuleHandle(NULL);
	TCHAR path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);
	wstring rs(path);
	return rs;
}

wstring OpenKeyHelper::getClipboardText(const int& type) {
	if (!IsClipboardFormatAvailable(type)) {
		return _T("");
	}
	// Try opening the clipboard with retry
	int retry = 5;
	bool opened = false;
	while (retry-- > 0) {
		if (OpenClipboard(nullptr)) {
			opened = true;
			break;
		}
		Sleep(10);
	}
	if (!opened) {
		return _T("");
	}

	// Get handle of clipboard object for ANSI text
	HANDLE hData = GetClipboardData(type);
	if (hData == nullptr) {
		CloseClipboard();
		return _T("");
	}

	// Lock the handle to get the actual text pointer
	wchar_t * pszText = static_cast<wchar_t*>(GlobalLock(hData));
	if (pszText == nullptr) {
		CloseClipboard();
		return _T("");
	}

	// Save text in a string class instance
	wstring text(pszText);
	
	// Release the lock
	GlobalUnlock(hData);

	// Release the clipboard
	CloseClipboard();
	
	return text;
}

void OpenKeyHelper::setClipboardText(LPCTSTR data, const int & len, const int& type) {
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(WCHAR));
	if (!hMem) return;
	memcpy(GlobalLock(hMem), data, len * sizeof(WCHAR));
	GlobalUnlock(hMem);
	int retry = 5;
	while (retry-- > 0) {
		if (OpenClipboard(0)) {
			EmptyClipboard();
			SetClipboardData(type, hMem);
			CloseClipboard();
			return;
		}
		Sleep(10);
	}
	GlobalFree(hMem);
}

bool OpenKeyHelper::quickConvert() {
	//read data from clipboard
	//support Unicode raw string, Rich Text Format and HTML

	if (!OpenClipboard(nullptr)) {
		return false;
	}

	string dataHTML, dataRTF;
	wstring dataUnicode;

	char* pHTML = 0, pRTF = 0;
	wchar_t* pUnicode = 0;

	//HTML
	HANDLE hData = GetClipboardData(CF_HTML);
	if (hData) {
		pHTML = static_cast<char*>(GlobalLock(hData));
		GlobalUnlock(hData);
	}
	if (pHTML) {
		dataHTML = pHTML;
		dataHTML = convertUtil(dataHTML);
	}

	//UNICODE
	hData = GetClipboardData(CF_UNICODETEXT);
	if (hData) {
		pUnicode = static_cast<wchar_t*>(GlobalLock(hData));
		GlobalUnlock(hData);
	}
	if (pUnicode) {
		dataUnicode = pUnicode;
		dataUnicode = utf8ToWideString(convertUtil(wideStringToUtf8(dataUnicode)));
	}

	OpenClipboard(0);
	EmptyClipboard();

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (int)(dataHTML.size() + 1) * sizeof(char));
	memcpy(GlobalLock(hMem), dataHTML.c_str(), (int)(dataHTML.size() + 1) * sizeof(char));
	GlobalUnlock(hMem);
	SetClipboardData(CF_HTML, hMem);

	hMem = GlobalAlloc(GMEM_MOVEABLE, (int)(dataUnicode.size() + 1) * sizeof(wchar_t));
	memcpy(GlobalLock(hMem), dataUnicode.c_str(), (int)(dataUnicode.size() + 1) * sizeof(wchar_t));
	GlobalUnlock(hMem);
	SetClipboardData(CF_UNICODETEXT, hMem);

	CloseClipboard();
	return true;
}

DWORD OpenKeyHelper::getVersionNumber() {
	// get the filename of the executable containing the version resource
	TCHAR szFilename[MAX_PATH + 1] = { 0 };
	if (GetModuleFileName(NULL, szFilename, MAX_PATH) == 0) {
		return 0;
	}

	// allocate a block of memory for the version info
	DWORD dummy;
	UINT dwSize = GetFileVersionInfoSize(szFilename, &dummy);
	if (dwSize == 0) {
		return 0;
	}
	std::vector<BYTE> data(dwSize);

	// load the version info
	if (!GetFileVersionInfo(szFilename, NULL, dwSize, &data[0])) {
		return 0;
	}

	LPBYTE lpBuffer = NULL;

	if (VerQueryValue(&data[0], _T("\\"), (VOID FAR * FAR*) & lpBuffer, &dwSize)) {
		if (dwSize) {
			VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
			if (verInfo->dwSignature == 0xfeef04bd) {
				return ((verInfo->dwFileVersionMS >> 16) & 0xffff) |
					(((verInfo->dwFileVersionMS >> 0) & 0xffff) << 8) |
					(((verInfo->dwFileVersionLS >> 16) & 0xffff) << 16);
			}
		}
	}

	return 0;
}

wstring OpenKeyHelper::getVersionString() {
	TCHAR versionBuffer[MAX_PATH];
	DWORD ver = getVersionNumber();
	wsprintfW(versionBuffer, _T("%d.%d.%d"), ver & 0xFF, (ver>>8) & 0xFF, (ver >> 16) & 0xFF);
	return wstring(versionBuffer);

	// get the filename of the executable containing the version resource
	TCHAR szFilename[MAX_PATH + 1] = { 0 };
	if (GetModuleFileName(NULL, szFilename, MAX_PATH) == 0) { 
		return _T("");
	}
}

wstring OpenKeyHelper::getContentOfUrl(LPCTSTR url){
	WCHAR path[MAX_PATH];
	GetTempPath(MAX_PATH, path);
	wsprintf(path, TEXT("%s\\_OpenKey.tempf"), path);
	HRESULT res = URLDownloadToFile(NULL, url, path, 0, NULL);
	
	if (res == S_OK) {
		std::wifstream t(path);
		std::wstringstream buffer;
		buffer << t.rdbuf();
		t.close();
		DeleteFile(path);
		return buffer.str();
	} else if (res == E_OUTOFMEMORY) {
		
	} else if (res == INET_E_DOWNLOAD_FAILURE) {
		
	} else {
		
	}
	return L"";
}
