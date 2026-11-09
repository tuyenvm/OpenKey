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
#include "ExcludedAppsDialog.h"
#include "stdafx.h"
#include "AppDelegate.h"
#include <commdlg.h>
#include <set>
#include <algorithm>

extern void saveSmartSwitchKeyData();

ExcludedAppsDialog::ExcludedAppsDialog(const HINSTANCE& hInstance, const int& resourceId)
	: BaseDialog(hInstance, resourceId) {
}

ExcludedAppsDialog::~ExcludedAppsDialog() {
}

INT_PTR ExcludedAppsDialog::eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		this->hDlg = hDlg;
		initDialog();
		return TRUE;
	case WM_COMMAND: {
		int wmId = LOWORD(wParam);
		switch (wmId) {
		case IDOK:
		case IDBUTTON_OK:
			AppDelegate::getInstance()->closeDialog(this);
			break;
		case IDC_BUTTON_ADD_ENG:
			onAddAppWithLanguage(0);
			break;
		case IDC_BUTTON_ADD_VIET:
			onAddAppWithLanguage(1);
			break;
		case IDC_BUTTON_DELETE_APP:
			onDeleteAppButton();
			break;
		case IDC_BUTTON_BROWSE_APP:
			onBrowseAppButton();
			break;
		case IDC_BUTTON_ADD_DEFAULT_APPS:
			onAddDefaultAppsButton();
			break;
		default:
			if (HIWORD(wParam) == CBN_DROPDOWN && (HWND)lParam == hComboAppName) {
				populateRunningApps();
			} else if (HIWORD(wParam) == BN_CLICKED) {
				this->onCheckboxClicked((HWND)lParam);
			}
			break;
		}
		break;
	}
	case WM_NOTIFY: {
		switch (((LPNMHDR)lParam)->code) {
		case LVN_ITEMCHANGED: {
			NMLISTVIEW* pData = (NMLISTVIEW*)lParam;
			if (pData->uNewState & LVIS_SELECTED)
				onSelectItem(pData->iItem);
			break;
		}
		case NM_DBLCLK: {
			NMITEMACTIVATE* pItem = (NMITEMACTIVATE*)lParam;
			if (pItem->iItem >= 0) {
				onToggleItem(pItem->iItem);
			}
			break;
		}
		}
		break;
	}
	}
	return FALSE;
}

void ExcludedAppsDialog::initDialog() {
	SET_DIALOG_ICON(IDI_APP_ICON);

	listApp = GetDlgItem(hDlg, IDC_LIST_APP_DATA);
	hComboAppName = GetDlgItem(hDlg, IDC_COMBO_APP_NAME);
	hCheckSmartSwitch = GetDlgItem(hDlg, IDC_CHECK_SMART_SWITCH_KEY);

	RECT rcList;
	GetClientRect(listApp, &rcList);
	int totalWidth = rcList.right - rcList.left - GetSystemMetrics(SM_CXVSCROLL) - 4;
	int col0Width = (totalWidth * 42) / 100;
	int col1Width = totalWidth - col0Width;

	LVCOLUMN LvCol;
	memset(&LvCol, 0, sizeof(LvCol));
	LvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	LvCol.pszText = (LPWSTR)_T("Chương trình (.exe)");
	LvCol.cx = col0Width > 180 ? col0Width : 180;

	SendMessage(listApp, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	SendMessage(listApp, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);

	LvCol.pszText = (LPWSTR)_T("Chế độ gõ (Nháy đúp để đổi)");
	LvCol.cx = col1Width > 220 ? col1Width : 220;
	SendMessage(listApp, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);

	populateRunningApps();
	fillData();
}

void ExcludedAppsDialog::fillData() {
	SendMessage(hCheckSmartSwitch, BM_SETCHECK, vUseSmartSwitchKey ? 1 : 0, 0);
	SendMessage(listApp, LVM_DELETEALLITEMS, 0, 0);

	vector<string> rawNames;
	vector<int> rawStatuses;
	getAllSmartSwitchKey(rawNames, rawStatuses);

	appNames.clear();
	appStatuses.clear();

	// Put apps with Vietnamese OFF (0) first, then Vietnamese ON (1)
	for (size_t i = 0; i < rawNames.size(); i++) {
		if ((rawStatuses[i] & 0x01) == 0) {
			appNames.push_back(rawNames[i]);
			appStatuses.push_back(rawStatuses[i]);
		}
	}
	for (size_t i = 0; i < rawNames.size(); i++) {
		if ((rawStatuses[i] & 0x01) != 0) {
			appNames.push_back(rawNames[i]);
			appStatuses.push_back(rawStatuses[i]);
		}
	}

	for (size_t i = 0; i < appNames.size(); i++) {
		wstring wName = utf8ToWideString(appNames[i]);
		LPCTSTR statusText = (appStatuses[i] & 0x01) == 0
			? _T("Tắt Tiếng Việt (Tiếng Anh)")
			: _T("Bật Tiếng Việt");
		insertItem((int)i, (LPTSTR)wName.c_str(), (LPTSTR)statusText);
	}
}

void ExcludedAppsDialog::saveAndReload() {
	saveSmartSwitchKeyData();
	fillData();
	SystemTrayHelper::updateData();
}

void ExcludedAppsDialog::insertItem(const int& index, LPTSTR appName, LPTSTR statusText) {
	LV_ITEM data;
	memset(&data, 0, sizeof(data));
	data.mask = LVIF_TEXT;
	data.cchTextMax = 256;
	data.iItem = index;
	data.iSubItem = 0;
	data.pszText = appName;

	SendMessage(listApp, LVM_INSERTITEM, 0, (LPARAM)&data);

	data.iSubItem = 1;
	data.pszText = statusText;
	SendMessage(listApp, LVM_SETITEM, 0, (LPARAM)&data);
}

static BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam) {
	if (!IsWindowVisible(hwnd)) return TRUE;
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (pid == 0 || pid == GetCurrentProcessId()) return TRUE;

	HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!proc) {
		proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	}
	if (proc) {
		TCHAR exePath[1024] = { 0 };
		if (GetProcessImageFileName(proc, exePath, 1024) > 0) {
			LPCTSTR slash = _tcsrchr(exePath, '\\');
			LPCTSTR exeName = slash ? (slash + 1) : exePath;
			if (_tcsicmp(exeName, _T("OpenKey64.exe")) != 0 &&
				_tcsicmp(exeName, _T("OpenKey32.exe")) != 0 &&
				_tcsicmp(exeName, _T("OpenKey.exe")) != 0 &&
				_tcsicmp(exeName, _T("explorer.exe")) != 0) {
				wstring wExe(exeName);
				std::transform(wExe.begin(), wExe.end(), wExe.begin(), ::towlower);
				std::set<wstring>* appSet = (std::set<wstring>*)lParam;
				appSet->insert(wExe);
			}
		}
		CloseHandle(proc);
	}
	return TRUE;
}

void ExcludedAppsDialog::populateRunningApps() {
	TCHAR currentText[256] = { 0 };
	GetWindowText(hComboAppName, currentText, 256);

	SendMessage(hComboAppName, CB_RESETCONTENT, 0, 0);

	std::set<wstring> runningApps = {
		L"cmd.exe",
		L"powershell.exe",
		L"pwsh.exe",
		L"windowsterminal.exe"
	};
	EnumWindows(enumWindowsProc, (LPARAM)&runningApps);

	for (const auto& app : runningApps) {
		SendMessage(hComboAppName, CB_ADDSTRING, 0, (LPARAM)app.c_str());
	}

	SetWindowText(hComboAppName, currentText);
}

string ExcludedAppsDialog::getNormalizedInputAppName() {
	TCHAR buffer[MAX_PATH] = { 0 };
	GetWindowText(hComboAppName, buffer, MAX_PATH);
	wstring wName = buffer;

	// Trim spaces
	while (!wName.empty() && (wName.front() == L' ' || wName.front() == L'\t')) wName.erase(wName.begin());
	while (!wName.empty() && (wName.back() == L' ' || wName.back() == L'\t')) wName.pop_back();

	if (wName.empty()) return "";

	// If full path was pasted, extract filename
	size_t slashPos = wName.find_last_of(L"\\/");
	if (slashPos != wstring::npos) {
		wName = wName.substr(slashPos + 1);
	}

	std::transform(wName.begin(), wName.end(), wName.begin(), ::towlower);

	// Append .exe if no extension provided
	if (wName.find(L'.') == wstring::npos) {
		wName += L".exe";
	}

	return wideStringToUtf8(wName);
}

void ExcludedAppsDialog::onSelectItem(const int& index) {
	if (index >= 0 && index < (int)appNames.size()) {
		SetWindowText(hComboAppName, utf8ToWideString(appNames[index]).c_str());
	}
}

void ExcludedAppsDialog::onToggleItem(const int& index) {
	if (index >= 0 && index < (int)appNames.size()) {
		int oldStatus = appStatuses[index];
		int newLang = (oldStatus & 0x01) ? 0 : 1;
		int newStatus = (oldStatus & ~0x01) | newLang;
		setAppInputMethodStatus(appNames[index], newStatus);
		saveAndReload();
	}
}

void ExcludedAppsDialog::onAddAppWithLanguage(const int& lang) {
	string name = getNormalizedInputAppName();
	if (name.empty()) {
		MessageBox(hDlg, _T("Bạn hãy nhập hoặc chọn tên chương trình (ví dụ: cmd.exe)!"), _T("OpenKey"), MB_OK | MB_ICONINFORMATION);
		SetFocus(hComboAppName);
		return;
	}

	if (!vUseSmartSwitchKey) {
		APP_SET_DATA(vUseSmartSwitchKey, 1);
	}

	setAppInputMethodStatus(name, lang | (vCodeTable << 1));
	SetWindowText(hComboAppName, _T(""));
	saveAndReload();
	SetFocus(hComboAppName);
}

void ExcludedAppsDialog::onDeleteAppButton() {
	string name = getNormalizedInputAppName();
	if (name.empty()) {
		MessageBox(hDlg, _T("Bạn hãy chọn chương trình cần xóa trong danh sách!"), _T("OpenKey"), MB_OK | MB_ICONINFORMATION);
		return;
	}

	if (removeAppInputMethodStatus(name)) {
		SetWindowText(hComboAppName, _T(""));
		saveAndReload();
		SetFocus(hComboAppName);
	}
}

void ExcludedAppsDialog::onBrowseAppButton() {
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH] = { 0 };
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("Chương trình (*.exe)\0*.exe\0All (*.*)\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE) {
		LPCTSTR slash = _tcsrchr(ofn.lpstrFile, '\\');
		LPCTSTR exeName = slash ? (slash + 1) : ofn.lpstrFile;
		SetWindowText(hComboAppName, exeName);
		onAddAppWithLanguage(0);
	}
}

void ExcludedAppsDialog::onAddDefaultAppsButton() {
	initDefaultExcludedApps();
	if (!vUseSmartSwitchKey) {
		APP_SET_DATA(vUseSmartSwitchKey, 1);
	}
	saveAndReload();
}

void ExcludedAppsDialog::onCheckboxClicked(const HWND& hWnd) {
	if (hWnd == hCheckSmartSwitch) {
		int val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vUseSmartSwitchKey, val ? 1 : 0);
		SystemTrayHelper::updateData();
	}
}

