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
#include "CustomInputMethodDialog.h"
#include "stdafx.h"
#include "AppDelegate.h"
#include "resource.h"
#include "OpenKeyHelper.h"
#include "../../../engine/Engine.h"
#include "../../../engine/DataType.h"
#include "../../../engine/Vietnamese.h"
#include <commctrl.h>

static const TCHAR* customActions[] = {
    _T("Xoá dấu"),
    _T("Dấu Sắc '"),
    _T("Dấu Huyền `"),
    _T("Dấu Hỏi ?"),
    _T("Dấu Ngã ~"),
    _T("Dấu Nặng ."),
    _T("Dấu mũ chung cho a, e, o thành â, ê, ô"),
    _T("Dấu mũ cho a thành â"),
    _T("Dấu mũ cho e thành ê"),
    _T("Dấu mũ cho o thành ô"),
    _T("Dấu móc cho a, u, o thành ă, ư, ơ"),
    _T("Dấu móc cho uo thành ươ"),
    _T("Dấu móc cho u thành ư"),
    _T("Dấu móc cho o thành ơ"),
    _T("Dấu móc cho a thành ă"),
    _T("Dấu gạch d thành đ"),
    _T("Dấu móc cho a, u, o thành ă, ư, ơ hoặc là chữ ư"),
    _T("Dấu móc cho a, u, o thành ă, ư, ơ hoặc là chữ ư trừ chữ bắt đầu của từ"),
    _T("Thoát bỏ dấu"),
    _T("Chữ ă"),
    _T("Chữ Ă"),
    _T("Chữ â"),
    _T("Chữ Â"),
    _T("Chữ đ"),
    _T("Chữ Đ"),
    _T("Chữ ê"),
    _T("Chữ Ê"),
    _T("Chữ ô"),
    _T("Chữ Ô"),
    _T("Chữ ơ"),
    _T("Chữ Ơ"),
    _T("Chữ ư"),
    _T("Chữ Ư")
};

static const TCHAR* presetNames[] = {
	_T("Telex"),
	_T("VNI"),
	_T("Simple Telex"),
	_T("Tư Bình Trần đơn giản")
};

static Uint32 getKeyCodeFromChar(wchar_t ch) {
    if (ch >= L'a' && ch <= L'z') {
        return (ch - L'a' + 0x41);
    }
    if (ch >= L'A' && ch <= L'Z') {
        return (ch - L'A' + 0x41) | CAPS_MASK;
    }
    if (_characterMap.find(ch) != _characterMap.end()) {
        return _characterMap[ch];
    }
    return 0;
}

static wchar_t getCharFromKeyCode(Uint32 key) {
    Uint32 baseKey = key & CHAR_MASK;
    bool isCaps = (key & CAPS_MASK) != 0;
    if (baseKey >= 0x41 && baseKey <= 0x5A) {
        return isCaps ? (wchar_t)baseKey : (wchar_t)(baseKey + 32);
    }
    if (_keyCodeToChar.empty()) {
        for (const auto& pair : _characterMap) {
            _keyCodeToChar[pair.second] = pair.first;
        }
    }
    if (_keyCodeToChar.find(key) != _keyCodeToChar.end()) {
        return _keyCodeToChar[key];
    }
    if (_keyCodeToChar.find(baseKey) != _keyCodeToChar.end()) {
        return _keyCodeToChar[baseKey];
    }
    return L'?';
}

CustomInputMethodDialog::CustomInputMethodDialog(const HINSTANCE & hInstance, const int & resourceId)
	: BaseDialog(hInstance, resourceId) {
}

CustomInputMethodDialog::~CustomInputMethodDialog() {
}

INT_PTR CustomInputMethodDialog::eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		this->hDlg = hDlg;
		initDialog();
		return TRUE;
	case WM_SIZE: {
		int newWidth = LOWORD(lParam);
		int newHeight = HIWORD(lParam);
		
		HWND listRules = GetDlgItem(hDlg, IDC_LIST_RULES);
		HWND btnDelete = GetDlgItem(hDlg, IDC_BTN_DELETE_RULE);
		HWND btnSave = GetDlgItem(hDlg, IDOK);
		HWND btnClose = GetDlgItem(hDlg, IDCANCEL);
		HWND grpPresets = GetDlgItem(hDlg, IDC_GRP_PRESETS);
		HWND comboPresets = GetDlgItem(hDlg, IDC_COMBO_PRESETS);
		HWND btnLoadPreset = GetDlgItem(hDlg, IDC_BTN_LOAD_PRESET);
		HWND grpDefineKey = GetDlgItem(hDlg, IDC_GRP_DEFINE_KEY);
		HWND comboAction = GetDlgItem(hDlg, IDC_COMBO_CUSTOM_ACTION);
		HWND btnAddRule = GetDlgItem(hDlg, IDC_BTN_ADD_RULE);
		HWND btnReplaceRule = GetDlgItem(hDlg, IDC_BTN_REPLACE_RULE);

		if (grpPresets) {
			RECT rect; GetWindowRect(grpPresets, &rect); MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2);
			MoveWindow(grpPresets, rect.left, rect.top, newWidth - rect.left * 2, rect.bottom - rect.top, TRUE);
		}
		if (comboPresets) {
			RECT rect; GetWindowRect(comboPresets, &rect); MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2);
			MoveWindow(comboPresets, rect.left, rect.top, newWidth - 110, rect.bottom - rect.top, TRUE);
		}
		if (btnLoadPreset) {
			RECT rect; GetWindowRect(btnLoadPreset, &rect); MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2);
			MoveWindow(btnLoadPreset, newWidth - 90, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
		}
		if (grpDefineKey) {
			RECT rect; GetWindowRect(grpDefineKey, &rect); MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2);
			MoveWindow(grpDefineKey, rect.left, rect.top, newWidth - rect.left * 2, newHeight - rect.top - 40, TRUE);
		}
		if (comboAction) {
			RECT rect; GetWindowRect(comboAction, &rect); MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2);
			MoveWindow(comboAction, rect.left, rect.top, newWidth - rect.left - 15, rect.bottom - rect.top, TRUE);
		}
		if (btnAddRule) {
			RECT rect; GetWindowRect(btnAddRule, &rect); MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2);
			MoveWindow(btnAddRule, newWidth - 145, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
		}
		if (btnReplaceRule) {
			RECT rect; GetWindowRect(btnReplaceRule, &rect); MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2);
			MoveWindow(btnReplaceRule, newWidth - 80, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
		}
		if (listRules) {
			RECT rect; GetWindowRect(listRules, &rect); MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2);
			MoveWindow(listRules, rect.left, rect.top, newWidth - rect.left * 2, newHeight - rect.top - 65, TRUE);
		}
		if (btnDelete) {
			RECT rect; GetWindowRect(btnDelete, &rect); MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2);
			MoveWindow(btnDelete, rect.left, newHeight - 50, rect.right - rect.left, rect.bottom - rect.top, TRUE);
		}
		if (btnSave) {
			RECT rect; GetWindowRect(btnSave, &rect); MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2);
			MoveWindow(btnSave, newWidth - 145, newHeight - 27, rect.right - rect.left, rect.bottom - rect.top, TRUE);
		}
		if (btnClose) {
			RECT rect; GetWindowRect(btnClose, &rect); MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2);
			MoveWindow(btnClose, newWidth - 75, newHeight - 27, rect.right - rect.left, rect.bottom - rect.top, TRUE);
		}
		break;
	}
	case WM_COMMAND: {
		int wmId = LOWORD(wParam);
		switch (wmId) {
		case IDCANCEL:
			AppDelegate::getInstance()->closeDialog(this);
			break;
		case IDOK:
			saveRules();
			AppDelegate::getInstance()->closeDialog(this);
			break;
		case IDC_BTN_LOAD_PRESET:
			onLoadPreset();
			break;
		case IDC_BTN_ADD_RULE:
			onAddRule();
			break;
		case IDC_BTN_REPLACE_RULE:
			onReplaceRule();
			break;
		case IDC_BTN_DELETE_RULE:
			onDeleteRule();
			break;
		}
		break;
	}
	case WM_NOTIFY: {
		LPNMHDR pnmh = (LPNMHDR)lParam;
		if (pnmh->hwndFrom == listRules && pnmh->code == LVN_ITEMCHANGED) {
			LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
			if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & LVIS_SELECTED)) {
				int index = pnmv->iItem;
				if (index >= 0 && index < (int)customRules.size()) {
					wchar_t keyStr[2] = { getCharFromKeyCode(customRules[index].key), 0 };
					SetWindowText(editKey, keyStr);
					SendMessage(comboAction, CB_SETCURSEL, customRules[index].action, 0);
				}
			}
		}
		break;
	}
	}
	return FALSE;
}

void CustomInputMethodDialog::initDialog() {
	SET_DIALOG_ICON(IDI_APP_ICON);

	comboPresets = GetDlgItem(hDlg, IDC_COMBO_PRESETS);
	btnLoadPreset = GetDlgItem(hDlg, IDC_BTN_LOAD_PRESET);
	editKey = GetDlgItem(hDlg, IDC_EDIT_CUSTOM_KEY);
	comboAction = GetDlgItem(hDlg, IDC_COMBO_CUSTOM_ACTION);
	btnAdd = GetDlgItem(hDlg, IDC_BTN_ADD_RULE);
	btnReplace = GetDlgItem(hDlg, IDC_BTN_REPLACE_RULE);
	btnDelete = GetDlgItem(hDlg, IDC_BTN_DELETE_RULE);
	listRules = GetDlgItem(hDlg, IDC_LIST_RULES);

	// Max 1 character in key editbox
	SendMessage(editKey, EM_SETLIMITTEXT, 1, 0);

	// Load presets combobox
	for (int i = 0; i < 4; i++) {
		SendMessage(comboPresets, CB_ADDSTRING, 0, (LPARAM)presetNames[i]);
	}
	SendMessage(comboPresets, CB_SETCURSEL, 0, 0);

	// Load actions combobox
	for (int i = 0; i < 33; i++) {
		SendMessage(comboAction, CB_ADDSTRING, 0, (LPARAM)customActions[i]);
	}
	SendMessage(comboAction, CB_SETCURSEL, 0, 0);

	// Config listview columns
	SendMessage(listRules, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	
	LVCOLUMN LvCol;
	memset(&LvCol, 0, sizeof(LvCol));
	LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	
	LvCol.pszText = (LPWSTR)L"Phím";
	LvCol.cx = 60;
	SendMessage(listRules, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);

	LvCol.pszText = (LPWSTR)L"Dùng cho";
	LvCol.cx = 280;
	SendMessage(listRules, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);

	fillData();
}

void CustomInputMethodDialog::fillData() {
	loadRulesList();
}

void CustomInputMethodDialog::loadRulesList() {
	SendMessage(listRules, LVM_DELETEALLITEMS, 0, 0);
	for (int i = 0; i < (int)customRules.size(); i++) {
		wchar_t keyStr[2] = { getCharFromKeyCode(customRules[i].key), 0 };
		
		LV_ITEM item;
		memset(&item, 0, sizeof(item));
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = keyStr;
		SendMessage(listRules, LVM_INSERTITEM, 0, (LPARAM)&item);

		item.iSubItem = 1;
		item.pszText = (LPWSTR)customActions[customRules[i].action];
		SendMessage(listRules, LVM_SETITEM, 0, (LPARAM)&item);
	}
}

void CustomInputMethodDialog::saveRules() {
	OpenKeyHelper::setRegBinary(_T("customRules"), (BYTE*)customRules.data(), (int)(customRules.size() * sizeof(CustomRule)));
}

void CustomInputMethodDialog::onAddRule() {
	TCHAR keyBuffer[2] = { 0 };
	GetWindowText(editKey, keyBuffer, 2);
	if (keyBuffer[0] == 0) {
		MessageBox(hDlg, _T("Hãy nhập phím!"), _T("OpenKey"), MB_OK);
		return;
	}

	Uint32 keyCode = getKeyCodeFromChar(keyBuffer[0]);
	if (keyCode == 0) {
		MessageBox(hDlg, _T("Phím không hợp lệ!"), _T("OpenKey"), MB_OK);
		return;
	}

	// Check if already exists
	for (const auto& r : customRules) {
		if (r.key == keyCode) {
			MessageBox(hDlg, _T("Phím đã tồn tại trong quy tắc!"), _T("OpenKey"), MB_OK);
			return;
		}
	}

	int action = (int)SendMessage(comboAction, CB_GETCURSEL, 0, 0);
	CustomRule newRule = { keyCode, action };
	customRules.push_back(newRule);
	loadRulesList();
}

void CustomInputMethodDialog::onReplaceRule() {
	TCHAR keyBuffer[2] = { 0 };
	GetWindowText(editKey, keyBuffer, 2);
	if (keyBuffer[0] == 0) {
		MessageBox(hDlg, _T("Hãy nhập phím!"), _T("OpenKey"), MB_OK);
		return;
	}

	Uint32 keyCode = getKeyCodeFromChar(keyBuffer[0]);
	if (keyCode == 0) {
		MessageBox(hDlg, _T("Phím không hợp lệ!"), _T("OpenKey"), MB_OK);
		return;
	}

	int action = (int)SendMessage(comboAction, CB_GETCURSEL, 0, 0);

	for (auto& r : customRules) {
		if (r.key == keyCode) {
			r.action = action;
			loadRulesList();
			return;
		}
	}

	MessageBox(hDlg, _T("Không tìm thấy phím để thay thế!"), _T("OpenKey"), MB_OK);
}

void CustomInputMethodDialog::onDeleteRule() {
	int sel = (int)SendMessage(listRules, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
	if (sel >= 0 && sel < (int)customRules.size()) {
		customRules.erase(customRules.begin() + sel);
		loadRulesList();
		SetWindowText(editKey, _T(""));
	}
}

void CustomInputMethodDialog::onLoadPreset() {
	int sel = (int)SendMessage(comboPresets, CB_GETCURSEL, 0, 0);
	customRules.clear();

	// Presets config
	if (sel == 0) { // Telex
		customRules.push_back({ getKeyCodeFromChar(L's'), 1 });
		customRules.push_back({ getKeyCodeFromChar(L'f'), 2 });
		customRules.push_back({ getKeyCodeFromChar(L'r'), 3 });
		customRules.push_back({ getKeyCodeFromChar(L'x'), 4 });
		customRules.push_back({ getKeyCodeFromChar(L'j'), 5 });
		customRules.push_back({ getKeyCodeFromChar(L'a'), 7 });
		customRules.push_back({ getKeyCodeFromChar(L'e'), 8 });
		customRules.push_back({ getKeyCodeFromChar(L'o'), 9 });
		customRules.push_back({ getKeyCodeFromChar(L'w'), 16 });
		customRules.push_back({ getKeyCodeFromChar(L'd'), 15 });
		customRules.push_back({ getKeyCodeFromChar(L'z'), 0 });
		customRules.push_back({ getKeyCodeFromChar(L'['), 31 });
		customRules.push_back({ getKeyCodeFromChar(L']'), 29 });
		customRules.push_back({ getKeyCodeFromChar(L'{'), 32 });
		customRules.push_back({ getKeyCodeFromChar(L'}'), 30 });
	}
	else if (sel == 1) { // VNI
		customRules.push_back({ getKeyCodeFromChar(L'1'), 1 });
		customRules.push_back({ getKeyCodeFromChar(L'2'), 2 });
		customRules.push_back({ getKeyCodeFromChar(L'3'), 3 });
		customRules.push_back({ getKeyCodeFromChar(L'4'), 4 });
		customRules.push_back({ getKeyCodeFromChar(L'5'), 5 });
		customRules.push_back({ getKeyCodeFromChar(L'6'), 6 });
		customRules.push_back({ getKeyCodeFromChar(L'7'), 10 });
		customRules.push_back({ getKeyCodeFromChar(L'8'), 10 });
		customRules.push_back({ getKeyCodeFromChar(L'9'), 15 });
		customRules.push_back({ getKeyCodeFromChar(L'0'), 0 });
	}
	else if (sel == 2) { // Simple Telex
		customRules.push_back({ getKeyCodeFromChar(L's'), 1 });
		customRules.push_back({ getKeyCodeFromChar(L'f'), 2 });
		customRules.push_back({ getKeyCodeFromChar(L'r'), 3 });
		customRules.push_back({ getKeyCodeFromChar(L'x'), 4 });
		customRules.push_back({ getKeyCodeFromChar(L'j'), 5 });
		customRules.push_back({ getKeyCodeFromChar(L'a'), 7 });
		customRules.push_back({ getKeyCodeFromChar(L'e'), 8 });
		customRules.push_back({ getKeyCodeFromChar(L'o'), 9 });
		customRules.push_back({ getKeyCodeFromChar(L'd'), 15 });
		customRules.push_back({ getKeyCodeFromChar(L'z'), 0 });
	}
	else if (sel == 3) { // Tư Bình Trần đơn giản
		customRules.push_back({ getKeyCodeFromChar(L'z'), 0 });
		customRules.push_back({ getKeyCodeFromChar(L'd'), 15 });
		customRules.push_back({ getKeyCodeFromChar(L'['), 31 });
		customRules.push_back({ getKeyCodeFromChar(L']'), 29 });
		customRules.push_back({ getKeyCodeFromChar(L'{'), 32 });
		customRules.push_back({ getKeyCodeFromChar(L'}'), 30 });
		customRules.push_back({ getKeyCodeFromChar(L'0'), 0 });
		customRules.push_back({ getKeyCodeFromChar(L'1'), 1 });
		customRules.push_back({ getKeyCodeFromChar(L'2'), 2 });
		customRules.push_back({ getKeyCodeFromChar(L'3'), 3 });
		customRules.push_back({ getKeyCodeFromChar(L'4'), 4 });
		customRules.push_back({ getKeyCodeFromChar(L'5'), 5 });
		customRules.push_back({ getKeyCodeFromChar(L'6'), 21 });
		customRules.push_back({ getKeyCodeFromChar(L'7'), 25 });
		customRules.push_back({ getKeyCodeFromChar(L'8'), 27 });
		customRules.push_back({ getKeyCodeFromChar(L'9'), 19 });
		customRules.push_back({ getKeyCodeFromChar(L'^'), 22 });
		customRules.push_back({ getKeyCodeFromChar(L'&'), 26 });
		customRules.push_back({ getKeyCodeFromChar(L'*'), 28 });
		customRules.push_back({ getKeyCodeFromChar(L'('), 20 });
	}

	loadRulesList();
}
