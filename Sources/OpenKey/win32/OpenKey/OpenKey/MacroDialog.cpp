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
#include <algorithm>

#define MAX_MACRO_BUFFER 65536

static wstring getMacroTextPath() {
	wstring macroPath = OpenKeyHelper::getExecutePath();
	size_t pos = macroPath.find_last_of(L"\\/");
	if (pos != wstring::npos) {
		macroPath = macroPath.substr(0, pos);
	}
	macroPath += L"\\openkeymacro.txt";
	return macroPath;
}
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
	case WM_MEASUREITEM: {
		LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
		if (lpmis->CtlID == IDC_COMBO_MACRO_TRIGGERS) {
			lpmis->itemHeight = 18;
			return TRUE;
		}
		break;
	}
	case WM_DRAWITEM: {
		LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
		if (lpdis->CtlID == IDC_COMBO_MACRO_TRIGGERS && lpdis->itemID != -1) {
			int bitIndex = lpdis->itemID;
			bool checked = (vMacroTriggerMask & (1 << bitIndex)) != 0;

			if (lpdis->itemState & ODS_SELECTED) {
				SetBkColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
				SetTextColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
			} else {
				SetBkColor(lpdis->hDC, GetSysColor(COLOR_WINDOW));
				SetTextColor(lpdis->hDC, GetSysColor(COLOR_WINDOWTEXT));
			}
			ExtTextOut(lpdis->hDC, 0, 0, ETO_OPAQUE, &lpdis->rcItem, NULL, 0, NULL);

			if (lpdis->itemState & ODS_COMBOBOXEDIT) {
				wstring summary = L"";
				if (vMacroTriggerMask & 0x01) summary += L"Space, ";
				if (vMacroTriggerMask & 0x02) summary += L"Enter, ";
				if (vMacroTriggerMask & 0x04) summary += L"LShift, ";
				if (vMacroTriggerMask & 0x08) summary += L"RShift, ";
				if (vMacroTriggerMask & 0x10) summary += L"LControl, ";
				if (vMacroTriggerMask & 0x20) summary += L"RControl, ";
				if (!summary.empty()) {
					summary = summary.substr(0, summary.length() - 2);
				} else {
					summary = L"(Không)";
				}
				RECT rcText = lpdis->rcItem;
				rcText.left += 4;
				DrawText(lpdis->hDC, summary.c_str(), -1, &rcText, DT_SINGLELINE | DT_VCENTER);
			} else {
				RECT rcCheck = lpdis->rcItem;
				rcCheck.left += 4;
				rcCheck.right = rcCheck.left + 14;
				rcCheck.top += (rcCheck.bottom - rcCheck.top - 14) / 2;
				rcCheck.bottom = rcCheck.top + 14;
				DrawFrameControl(lpdis->hDC, &rcCheck, DFC_BUTTON, DFCS_BUTTONCHECK | (checked ? DFCS_CHECKED : 0));

				TCHAR buffer[64];
				SendMessage(lpdis->hwndItem, CB_GETLBTEXT, lpdis->itemID, (LPARAM)buffer);

				RECT rcText = lpdis->rcItem;
				rcText.left += 22;
				DrawText(lpdis->hDC, buffer, -1, &rcText, DT_SINGLELINE | DT_VCENTER);
			}

			if (lpdis->itemState & ODS_FOCUS) {
				DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
			}
			return TRUE;
		}
		break;
	}
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
		case IDC_BUTTON_DELETE_ALL_MACRO:
			onDeleteAllMacroButton();
			break;
		case IDC_BUTTON_SORT_NAME:
			onSortNameButton();
			break;
		case IDC_BUTTON_SORT_CONTENT:
			onSortContentButton();
			break;
		case IDC_BUTTON_IMPORT_MACRO:
			onImportMacroButton();
			break;
		case IDC_BUTTON_CONVERT_EVKEY_MACRO:
			onConvertEvKeyMacroButton();
			break;
		case IDC_BUTTON_EXPORT_MACRO:
			onExportMacrobutton();
			break;
		case IDC_COMBO_MACRO_TRIGGERS:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				HWND hCombo = (HWND)lParam;
				int sel = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
				if (sel != -1) {
					vMacroTriggerMask ^= (1 << sel);
					APP_SET_DATA(vMacroTriggerMask, vMacroTriggerMask);
					InvalidateRect(hCombo, NULL, TRUE);
					SendMessage(hCombo, CB_SETCURSEL, (WPARAM)-1, 0);
				}
			}
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
	case WM_SIZE: {
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);

		HWND hList = GetDlgItem(hDlg, IDC_LIST_MACRO_DATA);
		HWND hName = GetDlgItem(hDlg, IDC_EDIT_MACRO_NAME);
		HWND hContent = GetDlgItem(hDlg, IDC_EDIT_MACRO_CONTENT);
		HWND hAdd = GetDlgItem(hDlg, IDC_BUTTON_ADD);
		HWND hDel = GetDlgItem(hDlg, IDC_BUTTON_DELETE);
		HWND hDeleteAll = GetDlgItem(hDlg, IDC_BUTTON_DELETE_ALL_MACRO);
		HWND hImport = GetDlgItem(hDlg, IDC_BUTTON_IMPORT_MACRO);
		HWND hConvertEvKey = GetDlgItem(hDlg, IDC_BUTTON_CONVERT_EVKEY_MACRO);
		HWND hExport = GetDlgItem(hDlg, IDC_BUTTON_EXPORT_MACRO);
		HWND hCaps = GetDlgItem(hDlg, IDC_CHECK_AUTO_CAPS);
		HWND hTriggerLabel = GetDlgItem(hDlg, IDC_LABEL_MACRO_TRIGGERS);
		HWND hTriggerCombo = GetDlgItem(hDlg, IDC_COMBO_MACRO_TRIGGERS);

		if (hName && hContent && hAdd && hDel && hDeleteAll && hList && hImport && hConvertEvKey && hExport && hCaps && hTriggerCombo) {
			SetWindowPos(hName, NULL, 10, 28, 110, 22, SWP_NOZORDER);

			int contentWidth = width - 275;
			if (contentWidth < 100) contentWidth = 100;
			SetWindowPos(hContent, NULL, 130, 28, contentWidth, 80, SWP_NOZORDER);

			SetWindowPos(hAdd, NULL, width - 135, 28, 125, 22, SWP_NOZORDER);
			SetWindowPos(hDel, NULL, width - 135, 54, 125, 22, SWP_NOZORDER);
			SetWindowPos(hDeleteAll, NULL, 10, 54, 110, 22, SWP_NOZORDER);
			if (hSortName) SetWindowPos(hSortName, NULL, 10, 80, 110, 22, SWP_NOZORDER);
			if (hSortContent) SetWindowPos(hSortContent, NULL, width - 135, 80, 125, 22, SWP_NOZORDER);

			int listHeight = height - 165;
			if (listHeight < 50) listHeight = 50;
			SetWindowPos(hList, NULL, 10, 120, width - 20, listHeight, SWP_NOZORDER);

			int bottomY = height - 35;
			SetWindowPos(hImport, NULL, 10, bottomY, 100, 22, SWP_NOZORDER);
			SetWindowPos(hExport, NULL, 120, bottomY, 100, 22, SWP_NOZORDER);
			SetWindowPos(hConvertEvKey, NULL, 230, bottomY, 125, 22, SWP_NOZORDER);
			SetWindowPos(hCaps, NULL, 365, bottomY, 130, 22, SWP_NOZORDER);
			if (hTriggerLabel) {
				SetWindowPos(hTriggerLabel, NULL, width - 260, bottomY + 3, 60, 22, SWP_NOZORDER);
			}
			SetWindowPos(hTriggerCombo, NULL, width - 195, bottomY - 3, 185, 22, SWP_NOZORDER);
		}
		return TRUE;
	}
	case WM_NOTIFY: {
		LPNMHDR pNmhdr = (LPNMHDR)lParam;
		if (pNmhdr->idFrom == IDC_LIST_MACRO_DATA) {
			if (pNmhdr->code == LVN_COLUMNCLICK) {
				LPNMLISTVIEW pNmlv = (LPNMLISTVIEW)lParam;
				int col = pNmlv->iSubItem;
				if (col == sortColumn) {
					sortAscending = !sortAscending;
				} else {
					sortColumn = col;
					sortAscending = true;
				}
				fillData();
			} else if (pNmhdr->code == LVN_ITEMCHANGED) {
				NMLISTVIEW* pData = (NMLISTVIEW*)lParam;
				if (pData->uNewState & LVIS_SELECTED)
					onSelectItem(pData->iItem);
			}
		}
		break;
	}
	}
	return FALSE;
}

void MacroDialog::initDialog() {
	SET_DIALOG_ICON(IDI_APP_ICON);

	listMacro = GetDlgItem(hDlg, IDC_LIST_MACRO_DATA);
	hMacroName = GetDlgItem(hDlg, IDC_EDIT_MACRO_NAME);
	hMacroContent = GetDlgItem(hDlg, IDC_EDIT_MACRO_CONTENT);
	hAddButton = GetDlgItem(hDlg, IDC_BUTTON_ADD);
	hAutoCaps = GetDlgItem(hDlg, IDC_CHECK_AUTO_CAPS);

	hSortName = CreateWindowEx(0, _T("BUTTON"), _T("Sort Từ gõ tắt ↕"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hDlg, (HMENU)IDC_BUTTON_SORT_NAME, hInstance, NULL);
	hSortContent = CreateWindowEx(0, _T("BUTTON"), _T("Sort Nội dung ↕"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hDlg, (HMENU)IDC_BUTTON_SORT_CONTENT, hInstance, NULL);
	if (hSortName) SendMessage(hSortName, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
	if (hSortContent) SendMessage(hSortContent, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

	if (hMacroContent) {
		SendMessage(hMacroContent, EM_SETLIMITTEXT, 0, 0);
	}

	HWND hComboTriggers = GetDlgItem(hDlg, IDC_COMBO_MACRO_TRIGGERS);
	if (hComboTriggers) {
		SendMessage(hComboTriggers, CB_ADDSTRING, 0, (LPARAM)_T("Space"));
		SendMessage(hComboTriggers, CB_ADDSTRING, 0, (LPARAM)_T("Enter"));
		SendMessage(hComboTriggers, CB_ADDSTRING, 0, (LPARAM)_T("Left Shift"));
		SendMessage(hComboTriggers, CB_ADDSTRING, 0, (LPARAM)_T("Right Shift"));
		SendMessage(hComboTriggers, CB_ADDSTRING, 0, (LPARAM)_T("Left Control"));
		SendMessage(hComboTriggers, CB_ADDSTRING, 0, (LPARAM)_T("Right Control"));
		SendMessage(hComboTriggers, CB_SETCURSEL, (WPARAM)-1, 0);
	}

	LVCOLUMN LvCol;
	memset(&LvCol, 0, sizeof(LvCol));
	LvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	LvCol.pszText = (LPWSTR)L"Từ gõ tắt";
	LvCol.cx = 165;
	
	SendMessage(listMacro, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	SendMessage(listMacro, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);

	LvCol.pszText = (LPWSTR)L"Nội dung thay thế";  
	LvCol.cx = 695;
	SendMessage(listMacro, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);

	fillData();
}

void MacroDialog::updateColumnHeaders() {
	LVCOLUMN LvCol;
	memset(&LvCol, 0, sizeof(LvCol));
	LvCol.mask = LVCF_TEXT;

	wstring textHeader = L"Từ gõ tắt";
	if (sortColumn == 0) {
		textHeader += sortAscending ? L" ▲" : L" ▼";
	}
	LvCol.pszText = (LPWSTR)textHeader.c_str();
	SendMessage(listMacro, LVM_SETCOLUMN, 0, (LPARAM)&LvCol);

	wstring contentHeader = L"Nội dung thay thế";
	if (sortColumn == 1) {
		contentHeader += sortAscending ? L" ▲" : L" ▼";
	}
	LvCol.pszText = (LPWSTR)contentHeader.c_str();
	SendMessage(listMacro, LVM_SETCOLUMN, 1, (LPARAM)&LvCol);
}

void MacroDialog::updateSortButtonTexts() {
	if (hSortName) {
		wstring txt = L"Sort Từ gõ tắt";
		if (sortColumn == 0) {
			txt += sortAscending ? L" ▲" : L" ▼";
		} else {
			txt += L" ↕";
		}
		SetWindowText(hSortName, txt.c_str());
	}
	if (hSortContent) {
		wstring txt = L"Sort Nội dung";
		if (sortColumn == 1) {
			txt += sortAscending ? L" ▲" : L" ▼";
		} else {
			txt += L" ↕";
		}
		SetWindowText(hSortContent, txt.c_str());
	}
}

void MacroDialog::onSortNameButton() {
	if (sortColumn == 0) {
		sortAscending = !sortAscending;
	} else {
		sortColumn = 0;
		sortAscending = true;
	}
	fillData();
}

void MacroDialog::onSortContentButton() {
	if (sortColumn == 1) {
		sortAscending = !sortAscending;
	} else {
		sortColumn = 1;
		sortAscending = true;
	}
	fillData();
}

struct MacroItemSort {
	string text;
	string content;
	wstring wideText;
	wstring wideContent;
};

void MacroDialog::fillData() {
	SendMessage(hAutoCaps, BM_SETCHECK, vAutoCapsMacro ? 1 : 0, 0);
	SendMessage(listMacro, LVM_DELETEALLITEMS, 0, 0);
	getAllMacro(keys, macroText, macroContent);

	vector<MacroItemSort> items(macroText.size());
	for (size_t i = 0; i < macroText.size(); i++) {
		items[i].text = macroText[i];
		items[i].content = macroContent[i];
		items[i].wideText = utf8ToWideString(macroText[i]);
		items[i].wideContent = utf8ToWideString(macroContent[i]);
	}

	if (sortColumn == 0) {
		std::sort(items.begin(), items.end(), [this](const MacroItemSort& a, const MacroItemSort& b) {
			int cmp = _wcsicmp(a.wideText.c_str(), b.wideText.c_str());
			return sortAscending ? (cmp < 0) : (cmp > 0);
		});
	} else if (sortColumn == 1) {
		std::sort(items.begin(), items.end(), [this](const MacroItemSort& a, const MacroItemSort& b) {
			int cmp = _wcsicmp(a.wideContent.c_str(), b.wideContent.c_str());
			return sortAscending ? (cmp < 0) : (cmp > 0);
		});
	}

	displayMacroText.clear();
	displayMacroContent.clear();
	displayMacroText.reserve(items.size());
	displayMacroContent.reserve(items.size());

	for (size_t i = 0; i < items.size(); i++) {
		size_t idx = (sortColumn == -1) ? (items.size() - 1 - i) : i;
		displayMacroText.push_back(items[idx].text);
		displayMacroContent.push_back(items[idx].content);

		insertItem((int)i, (LPTSTR)utf8ToWideString(items[idx].text).c_str(),
					(LPTSTR)utf8ToWideString(items[idx].content).c_str());
	}

	updateColumnHeaders();
	updateSortButtonTexts();
}

void MacroDialog::saveAndReload() {
	vector<Byte> macroData;
	getMacroSaveData(macroData);
	OpenKeyHelper::setRegBinary(_T("macroData"), macroData.data(), (int)macroData.size());

	saveToFile(wideStringToUtf8(getMacroTextPath()));

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

	SendMessage(listMacro, LVM_INSERTITEM, 0, (LPARAM)&data);

	data.iSubItem = 1;
	data.pszText = macroContent;
	SendMessage(listMacro, LVM_SETITEM, 0, (LPARAM)&data);
}

void MacroDialog::onSelectItem(const int & index) {
	if (index >= 0 && index < (int)displayMacroText.size()) {
		SetWindowText(hMacroName, utf8ToWideString(displayMacroText[index]).c_str());
		SetWindowText(hMacroContent, utf8ToWideString(displayMacroContent[index]).c_str());
		SetWindowText(hAddButton, BTN_UPDATE_TEXT);
	}
}

void MacroDialog::onAddMacroButton() {
	int nameLen = GetWindowTextLength(hMacroName);
	int contentLen = GetWindowTextLength(hMacroContent);

	vector<TCHAR> nameBuf(nameLen + 1);
	vector<TCHAR> contentBuf(contentLen + 1);

	GetWindowText(hMacroName, nameBuf.data(), nameLen + 1);
	wstring name = nameBuf.data();

	GetWindowText(hMacroContent, contentBuf.data(), contentLen + 1);
	wstring content = contentBuf.data();

	if (name.empty() || content.empty()) {
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
	int nameLen = GetWindowTextLength(hMacroName);
	vector<TCHAR> nameBuf(nameLen + 1);
	GetWindowText(hMacroName, nameBuf.data(), nameLen + 1);
	wstring name = nameBuf.data();

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

void MacroDialog::onDeleteAllMacroButton() {
	int msgboxID = MessageBox(
		hDlg,
		_T("Bạn có chắc muốn xóa toàn bộ gõ tắt không?"),
		_T("Xóa toàn bộ gõ tắt"),
		MB_ICONEXCLAMATION | MB_YESNO
	);
	if (msgboxID != IDYES) {
		return;
	}
	clearMacro();
	saveAndReload();
	SetWindowText(hMacroName, _T(""));
	SetWindowText(hMacroContent, _T(""));
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
	ofn.lpstrFilter = _T("Text file (*.txt)\0*.txt\0Markdown file (*.md)\0*.md\0All (*.*)\0*.*\0");
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
		if (readFromFileCount(wideStringToUtf8(path), msgboxID == IDYES) > 0) {
			saveAndReload();
		} else {
			MessageBox(hDlg, _T("Khong tim thay macro hop le trong file."), _T("OpenKey"), MB_OK | MB_ICONWARNING);
		}
	}
}

void MacroDialog::onConvertEvKeyMacroButton() {
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH] = { 0 };
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("EVKey macro (*.txt)\0*.txt\0All (*.*)\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE) {
		int msgboxID = MessageBox(
			hDlg,
			L"B\u1EA1n c\u00F3 mu\u1ED1n gi\u1EEF l\u1EA1i d\u1EEF li\u1EC7u hi\u1EC7n t\u1EA1i kh\u00F4ng?",
			L"Chuy\u1EC3n EvKey macro",
			MB_ICONEXCLAMATION | MB_YESNO
		);
		wstring path = ofn.lpstrFile;
		if (readEvKeyFromFileCount(wideStringToUtf8(path), msgboxID == IDYES) > 0) {
			saveAndReload();
		} else {
			MessageBox(hDlg, L"Kh\u00F4ng t\u00ECm th\u1EA5y macro EvKey h\u1EE3p l\u1EC7 trong file.", L"OpenKey", MB_OK | MB_ICONWARNING);
		}
	}
}
void MacroDialog::onExportMacrobutton() {
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH] = { 'o', 'p', 'e', 'n', 'k', 'e', 'y', 'm', 'a', 'c', 'r', 'o', '.', 't', 'x', 't' };
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
