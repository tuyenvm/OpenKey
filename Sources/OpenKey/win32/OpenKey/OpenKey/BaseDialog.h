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
#define SET_DIALOG_ICON(ICON_RES) HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP_ICON)); \
								if (hIcon) { \
									SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon); \
								}
class BaseDialog {
protected:
	UINT resId;
	HINSTANCE hInstance;
	HWND hDlg;
protected:
	void createToolTip(const HWND& control, LPCTSTR toolTipString);
	void createToolTip(const HWND& control, const UINT& stringResId);
	virtual INT_PTR eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
public:
	BaseDialog(const HINSTANCE& hInstance, const int& resourceId);
	void show();
	void bringOnTop();
	virtual void fillData() = 0;

	HWND getHwnd();
	void closeDialog();
	friend INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};