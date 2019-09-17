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
#include "BaseDialog.h"

static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_INITDIALOG) {
#ifdef _WIN64
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
#else
		SetWindowLong(hDlg, GWL_USERDATA, lParam);
#endif
		BaseDialog*b = (BaseDialog*)lParam;
		return b->eventProc(hDlg, uMsg, wParam, lParam);
	}
#ifdef _WIN64
	LONG_PTR attr = GetWindowLongPtr(hDlg, GWLP_USERDATA);
#else
	long attr = GetWindowLong(hDlg, GWL_USERDATA);
#endif
	if (attr != 0) {
		return ((BaseDialog*)attr)->eventProc(hDlg, uMsg, wParam, lParam);
	}
	return FALSE;
}

BaseDialog::BaseDialog(const HINSTANCE& hInstance, const int & resourceId) {
	resId = resourceId;
	this->hInstance = hInstance;
}

void BaseDialog::show() {
	hDlg = CreateDialogParam(hInstance, MAKEINTRESOURCE(resId), 0, DialogProc, (LPARAM)this);
	ShowWindow(hDlg, SW_SHOWNORMAL);
}

void BaseDialog::bringOnTop() {
	SetForegroundWindow(hDlg);
	SetActiveWindow(hDlg);
}

HWND BaseDialog::getHwnd() {
	return hDlg;
}

void BaseDialog::closeDialog() {
#ifdef _WIN64
	SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
#else
	SetWindowLong(hDlg, GWL_USERDATA, 0);
#endif
	DestroyWindow(hDlg);
}
