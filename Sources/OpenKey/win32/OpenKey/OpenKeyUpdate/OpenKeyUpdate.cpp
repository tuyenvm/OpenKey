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

#include "framework.h"
#include "OpenKeyUpdate.h"
#include <Urlmon.h>
#include <fstream>
#include <sstream>
#include <string>
#pragma comment(lib, "Urlmon.lib")

using namespace std;

INT_PTR CALLBACK MainDialogProcess(HWND, UINT, WPARAM, LPARAM);
void StartUpdate();
HWND hDlg;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	hDlg = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG_UPDATER), 0, MainDialogProcess, 0);
	ShowWindow(hDlg, SW_SHOWNORMAL);
 
	MSG msg;
	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (!IsDialogMessage(hDlg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK MainDialogProcess(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:{
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_OPENKEYUPDATE));
		if (hIcon) {
			SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		}
		StartUpdate();
		return (INT_PTR)TRUE;
	}
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

DWORD WINAPI UpdateThreadFunction(LPVOID lpParam) {
	WCHAR path[MAX_PATH];
	WCHAR currentDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, currentDir);
	wsprintf(path, TEXT("%s\\_OpenKey.tempf"), currentDir);
	HRESULT res = URLDownloadToFile(NULL, L"https://raw.githubusercontent.com/tuyenvm/OpenKey/master/version.json", path, 0, NULL);

	wstring data;
	if (res == S_OK) {
		std::wifstream t(path);
		std::wstringstream buffer;
		buffer << t.rdbuf();
		t.close();
		DeleteFile(path);
		data = buffer.str();
	} else {
		MessageBox(hDlg, _T("Có lỗi trong quá trình cập nhật, vui lòng thử lại sau!"), _T("OpenKey Update"), MB_OK);
		ExitProcess(0);
		return 0;
	}

	//simple parse
	data = data.substr(data.find(L"latestWinVersion"));
	data = data.substr(data.find(L"\"versionName\":"));
	data = data.substr(14);
	data = data.substr(data.find(L"\""));
	data = data.substr(1);
	wstring versionName = data.substr(0, data.find(L"\""));
	
	//download zip file
	WCHAR updateUrl[MAX_PATH];
	wsprintf(updateUrl, TEXT("https://github.com/tuyenvm/OpenKey/releases/download/%s/OpenKey%s-Windows.zip"),
		versionName.c_str(),
		versionName.c_str());
	wsprintf(path, TEXT("%s\\_OpenKeyUpdate.zip"), currentDir);
	res = URLDownloadToFile(NULL, updateUrl, path, 0, NULL);

	if (res == S_OK) {
		//remove old file
		DeleteFile(L"OpenKey64.exe");
		//extract zip file
		WinExec("powershell.exe -NoP -NonI -Command \"Expand-Archive '.\\_OpenKeyUpdate.zip' '.\\_OpenKeyUpdate'\" ", SW_HIDE);
		Sleep(5000);
		MoveFile(L"_OpenKeyUpdate\\OpenKey64.exe", L"OpenKey64.exe");
		DeleteFile(path);
		DeleteFile(L"_OpenKeyUpdate\\OpenKeyUpdate.exe");
		DeleteFile(L"_OpenKeyUpdate\\OpenKey64.exe");
		DeleteFile(L"_OpenKeyUpdate\\OpenKey32.exe");
		RemoveDirectory(L".\\_OpenKeyUpdate");
		MessageBox(hDlg, _T("Bạn đã cập nhật OpenKey bản mới nhất thành công!"), _T("OpenKey Update"), MB_OK);
		ExitProcess(0);
	} else {
		MessageBox(hDlg, _T("Có lỗi trong quá trình cập nhật, vui lòng thử lại sau!"), _T("OpenKey Update"), MB_OK);
		ExitProcess(0);
	}
	return 0;
}

void StartUpdate() {
	DWORD hThread;
	HANDLE t = CreateThread(
							NULL,                   // default security attributes
							0,                      // use default stack size  
							UpdateThreadFunction,       // thread function name
							0,          // argument to thread function 
							0,                      // use default creation flags 
							&hThread);   // returns the thread identifier 
}