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
#include "MacroDialog.h"
#include "stdafx.h"
#include "AppDelegate.h"
#include <commdlg.h>

#define MAX_MACRO_BUFFER 4096
#define BTN_ADD_TEXT _T("+ Thêm")
#define BTN_UPDATE_TEXT _T("+ Sửa")

MacroDialog::MacroDialog(const HINSTANCE & hInstance, const int & resourceId)
	: BaseDialog(hInstance, resourceId)  {
}

MacroDialog::~MacroDialog() {
}

INT_PTR MacroDialog::eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
		case IDC_BUTTON_ADD:
			onAddMacroButton();
			break;
		case IDC_BUTTON_DELETE:
			onDeleteMacroButton();
			break;
		case IDC_BUTTON_IMPORT_MACRO:
			onImportMacroButton();
			break;
		case IDC_BUTTON_EXPORT_MACRO:
			onExportMacrobutton();
			break;
		default:
			if (HIWORD(wParam) == EN_CHANGE) {
				if ((HWND)lParam == hMacroName) {
					TCHAR buffer[128];
					GetWindowText(hMacroName, buffer, 128);
					wstring name = buffer;
					if (hasMacro(wideStringToUtf8(name))) {
						SetWindowText(hAddButton, BTN_UPDATE_TEXT);
					} else {
						SetWindowText(hAddButton, BTN_ADD_TEXT);
					}
				}
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
			if (pData->uNewState != 0)
				onSelectItem(pData->iItem);
		}
			break;
		}
		break;
	}
	}
	return FALSE;
}

void MacroDialog::initDialog() {
	//dialog icon
	SET_DIALOG_ICON(IDI_APP_ICON);

	listMacro = GetDlgItem(hDlg, IDC_LIST_MACRO_DATA);
	hMacroName = GetDlgItem(hDlg, IDC_EDIT_MACRO_NAME);
	hMacroContent = GetDlgItem(hDlg, IDC_EDIT_MACRO_CONTENT);
	hAddButton = GetDlgItem(hDlg, IDC_BUTTON_ADD);
	hAutoCaps = GetDlgItem(hDlg, IDC_CHECK_AUTO_CAPS);

	LVCOLUMN LvCol;
	memset(&LvCol, 0, sizeof(LvCol));
	LvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	LvCol.pszText = (LPWSTR)L"Macro Name";
	LvCol.cx = 165;
	
	SendMessage(listMacro, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT); // Set style
	SendMessage(listMacro, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol); //macro name column

	LvCol.pszText = (LPWSTR)L"Macro Content";  
	LvCol.cx = 695;
	SendMessage(listMacro, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol); //macro content column

	fillData();
}

void MacroDialog::fillData() {
	SendMessage(hAutoCaps, BM_SETCHECK, vAutoCapsMacro ? 1 : 0, 0);
	SendMessage(listMacro, LVM_DELETEALLITEMS, 0, 0);
	getAllMacro(keys, macroText, macroContent);
	for (size_t i = 0; i < macroText.size(); i++) {
		insertItem((int)i, (LPTSTR)utf8ToWideString(macroText[macroText.size() - 1 - i]).c_str(),
					(LPTSTR)utf8ToWideString(macroContent[macroText.size() - 1 - i]).c_str());
	}
}

void MacroDialog::saveAndReload() {
	//save
	vector<Byte> macroData;
	getMacroSaveData(macroData);
	OpenKeyHelper::setRegBinary(_T("macroData"), macroData.data(), (int)macroData.size());

	//reload data
	fillData();
}

void MacroDialog::insertItem(const int& index, LPTSTR macroName, LPTSTR macroContent) {
	LV_ITEM data;
	memset(&data, 0, sizeof(data));
	data.mask = LVIF_TEXT;
	data.cchTextMax = 256;
	data.iItem = index;
	data.iSubItem = 0;
	data.pszText = macroName;

	//insert macro name
	SendMessage(listMacro, LVM_INSERTITEM, 0, (LPARAM)&data);

	//insert content
	data.iSubItem = 1;
	data.pszText = macroContent;
	SendMessage(listMacro, LVM_SETITEM, 0, (LPARAM)&data);
}

void MacroDialog::onSelectItem(const int & index) {
	SetWindowText(hMacroName, utf8ToWideString(macroText[macroText.size() - 1 - index]).c_str());
	SetWindowText(hMacroContent, utf8ToWideString(macroContent[macroText.size() - 1 - index]).c_str());
	SetWindowText(hAddButton, BTN_UPDATE_TEXT);
}

void MacroDialog::onAddMacroButton() {
	TCHAR buffer[MAX_MACRO_BUFFER];
	GetWindowText(hMacroName, buffer, MAX_MACRO_BUFFER);
	wstring name = buffer;

	GetWindowText(hMacroContent, buffer, MAX_MACRO_BUFFER);
	wstring content = buffer;

	if (name.compare(L"") == 0 || content.compare(L"") == 0) {
		MessageBox(hDlg, _T("Bạn hãy nhập từ cần gõ tắt!"), _T("OpenKey"), MB_OK);
		return;
	}

	addMacro(wideStringToUtf8(name), wideStringToUtf8(content));
	SetWindowText(hMacroName, _T(""));
	SetWindowText(hMacroContent, _T(""));
	saveAndReload();
	SetFocus(hMacroName);
}

void MacroDialog::onDeleteMacroButton() {
	TCHAR buffer[MAX_MACRO_BUFFER];
	GetWindowText(hMacroName, buffer, MAX_MACRO_BUFFER);
	wstring name = buffer;

	if (name.compare(L"") == 0) {
		MessageBox(hDlg, _T("Bạn hãy chọn từ cần xoá!"), _T("OpenKey"), MB_OK);
		return;
	}

	if (deleteMacro(wideStringToUtf8(name))) {
		saveAndReload();
		SetWindowText(hMacroName, _T(""));
		SetWindowText(hMacroContent, _T(""));
		SetFocus(hMacroName);
	}
	SetWindowText(hAddButton, BTN_ADD_TEXT);
}

void MacroDialog::onImportMacroButton() {
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH] = { 0 };
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("Text file (*.txt)\0*.txt\0All (*.*)\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE) {
		int msgboxID = MessageBox(
			hDlg,
			_T("Bạn có muốn giữ lại dữ liệu hiện tại không?"),
			_T("Dữ liệu gõ tắt"),
			MB_ICONEXCLAMATION | MB_YESNO
		);
		wstring path = ofn.lpstrFile;
		readFromFile(wideStringToUtf8(path), msgboxID == IDYES);
		saveAndReload();
	}
}

void MacroDialog::onExportMacrobutton() {
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH] = { 'O', 'p', 'e', 'n', 'K', 'e', 'y', 'M', 'a', 'c', 'r', 'o' };
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("Text file (*.txt)\0*.txt\0");
	ofn.lpstrFileTitle = (LPTSTR)_T("lpstrFile");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrDefExt = (LPCWSTR)L"txt";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (GetSaveFileName(&ofn) == TRUE) {
		wstring path = ofn.lpstrFile;
		saveToFile(wideStringToUtf8(path));
	}
}

void MacroDialog::onCheckboxClicked(const HWND& hWnd) {
	if (hWnd == hAutoCaps) {
		int val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vAutoCapsMacro, val ? 1 : 0);
	}
}