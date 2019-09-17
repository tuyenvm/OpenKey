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
#include "ConvertToolDialog.h"
#include "stdafx.h"
#include "AppDelegate.h"

static Uint16 _lastKeyCode;

ConvertToolDialog::ConvertToolDialog(const HINSTANCE & hInstance, const int & resourceId)
	: BaseDialog(hInstance, resourceId) {
}

ConvertToolDialog::~ConvertToolDialog() {
}

INT_PTR ConvertToolDialog::eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		this->hDlg = hDlg;
		initDialog();
		return TRUE;
	case WM_COMMAND: {
		int wmId = LOWORD(wParam);
		switch (wmId) {
		case IDM_EXIT:
		case IDOK:
		case IDBUTTON_OK:
			AppDelegate::getInstance()->closeDialog(this);
			break;
		case IDC_BUTTON_CONVERT_SWITCH:
			onSwitchButton();
			break;
		case IDC_BUTTON_CONVERT_BUTTON:
			onConvertButton();
			break;
		default:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				this->onComboBoxSelected((HWND)lParam, LOWORD(wParam));
			}
			else if (HIWORD(wParam) == BN_CLICKED) {
				this->onCheckboxClicked((HWND)lParam);
			}
			else if (HIWORD(wParam) == EN_CHANGE) {
				_lastKeyCode = OpenKeyManager::_lastKeyCode;
				if (_lastKeyCode > 0) {
					OpenKeyManager::_lastKeyCode = 0;
					this->onCharacter((HWND)lParam, _lastKeyCode);
				}
			}
			break;
		}
		break;
	}
	}
	return FALSE;
}

void ConvertToolDialog::initDialog() {
	//dialog icon
	SET_DIALOG_ICON(IDI_APP_ICON);

	//code table
	comboboxSource = GetDlgItem(hDlg, IDC_COMBO_TABLE_CODE_SRC);
	comboboxDest = GetDlgItem(hDlg, IDC_COMBO_TABLE_CODE_DST);

	vector<LPCTSTR>& tableCode = OpenKeyManager::getTableCode();
	for (int i = 0; i < tableCode.size(); i++) {
		SendMessage(comboboxSource, CB_ADDSTRING, i, reinterpret_cast<LPARAM>(tableCode[i]));
		SendMessage(comboboxDest, CB_ADDSTRING, i, reinterpret_cast<LPARAM>(tableCode[i]));
	}

	checkAllCaps = GetDlgItem(hDlg, IDC_CHECK_All_CAPS);
	checkNonAllCaps = GetDlgItem(hDlg, IDC_CHECK_NON_ALL_CAPS);
	checkRemoveMark = GetDlgItem(hDlg, IDC_CHECK_REMOVE_MARK);
	checkCapsEarchWord = GetDlgItem(hDlg, IDC_CHECK_EACH_WORD);
	checkCapsFirstLetter = GetDlgItem(hDlg, IDC_CHECK_CAPS_FIRST_LETTER);
	checkAlertWhenCompleted = GetDlgItem(hDlg, IDC_CHECK_ALERT_WHEN_COMPLETED);

	checkConvertClipboard = GetDlgItem(hDlg, IDC_CHECK_CONVERT_CLIPBOARD);
	checkCtrl = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_CTRL);
	checkAlt = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_ALT);
	checkWin = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_WIN);
	checkShift = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_SHIFT);

	checkKey = GetDlgItem(hDlg, IDC_SWITCH_KEY_KEY);

	SendDlgItemMessage(hDlg, IDOK, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCEW(IDI_ICON_OK_BUTTON)));
	SendDlgItemMessage(hDlg, IDC_BUTTON_CONVERT_BUTTON, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCEW(IDI_ICON_START_CONVERT)));

	fillData();
}

void ConvertToolDialog::fillData() {
	SendMessage(comboboxSource, CB_SETCURSEL, convertToolFromCode, 0);
	SendMessage(comboboxDest, CB_SETCURSEL, convertToolToCode, 0);

	SendMessage(checkAllCaps, BM_SETCHECK, convertToolToAllCaps ? 1 : 0, 0);
	SendMessage(checkNonAllCaps, BM_SETCHECK, convertToolToAllNonCaps ? 1 : 0, 0);
	SendMessage(checkRemoveMark, BM_SETCHECK, convertToolRemoveMark ? 1 : 0, 0);
	SendMessage(checkCapsEarchWord, BM_SETCHECK, convertToolToCapsEachWord ? 1 : 0, 0);
	SendMessage(checkCapsFirstLetter, BM_SETCHECK, convertToolToCapsFirstLetter ? 1 : 0, 0);
	SendMessage(checkAlertWhenCompleted, BM_SETCHECK, convertToolDontAlertWhenCompleted ? 0 : 1, 0);

	SendMessage(checkConvertClipboard, BM_SETCHECK, 1, 0);
	EnableWindow(checkConvertClipboard, false);
	SendMessage(checkCtrl, BM_SETCHECK, HAS_CONTROL(convertToolHotKey) ? 1 : 0, 0);
	SendMessage(checkAlt, BM_SETCHECK, HAS_OPTION(convertToolHotKey) ? 1 : 0, 0);
	SendMessage(checkWin, BM_SETCHECK, HAS_COMMAND(convertToolHotKey) ? 1 : 0, 0);
	SendMessage(checkShift, BM_SETCHECK, HAS_SHIFT(convertToolHotKey) ? 1 : 0, 0);
	setSwitchKeyText(checkKey, (convertToolHotKey >> 24) & 0xFF);
}

void ConvertToolDialog::setSwitchKeyText(const HWND & hWnd, const UINT16 & keyCode) {
	if (keyCode == KEY_SPACE) {
		SetWindowText(hWnd, _T("Space"));
	} else if (keyCode == 0xFE) {
		SetWindowText(hWnd, _T(""));
	} else {
		Uint16 key[] = { keyCode, 0 };
		SetWindowText(hWnd, (LPCWSTR)&key);
	}
}

void ConvertToolDialog::setLogic(const HWND & exceptWnd) {
	if (exceptWnd != checkAllCaps) {
		SendMessage(checkAllCaps, BM_SETCHECK, 0, 0); 
		APP_SET_DATA(convertToolToAllCaps, 0);
	}
	if (exceptWnd != checkNonAllCaps) {
		SendMessage(checkNonAllCaps, BM_SETCHECK, 0, 0);
		APP_SET_DATA(convertToolToAllNonCaps, 0);
	}
	if (exceptWnd != checkCapsEarchWord) {
		SendMessage(checkCapsEarchWord, BM_SETCHECK, 0, 0); 
		APP_SET_DATA(convertToolToCapsEachWord, 0);
	}
	if (exceptWnd != checkCapsFirstLetter) {
		SendMessage(checkCapsFirstLetter, BM_SETCHECK, 0, 0);
		APP_SET_DATA(convertToolToCapsFirstLetter, 0);
	}
}

void ConvertToolDialog::onComboBoxSelected(const HWND & hCombobox, const int & comboboxId) {
	if (hCombobox == comboboxSource) {
		APP_SET_DATA(convertToolFromCode, (int)SendMessage(hCombobox, CB_GETCURSEL, 0, 0));
	} else if (hCombobox == comboboxDest) {
		APP_SET_DATA(convertToolToCode, (int)SendMessage(hCombobox, CB_GETCURSEL, 0, 0));
	}
}

void ConvertToolDialog::onCheckboxClicked(const HWND & hWnd) {
	int val = 0;
	if (hWnd == checkCtrl) {
		val = (int)SendMessage(checkCtrl, BM_GETCHECK, 0, 0);
		convertToolHotKey &= (~0x100);
		convertToolHotKey |= val << 8;
		APP_SET_DATA(convertToolHotKey, convertToolHotKey);
	} else if (hWnd == checkAlt) {
		val = (int)SendMessage(checkAlt, BM_GETCHECK, 0, 0);
		convertToolHotKey &= (~0x200);
		convertToolHotKey |= val << 9;
		APP_SET_DATA(convertToolHotKey, convertToolHotKey);
	} else if (hWnd == checkWin) {
		val = (int)SendMessage(checkWin, BM_GETCHECK, 0, 0);
		convertToolHotKey &= (~0x400);
		convertToolHotKey |= val << 10;
		APP_SET_DATA(convertToolHotKey, convertToolHotKey);
	} else if (hWnd == checkShift) {
		val = (int)SendMessage(checkShift, BM_GETCHECK, 0, 0);
		convertToolHotKey &= (~0x800);
		convertToolHotKey |= val << 11;
		APP_SET_DATA(convertToolHotKey, convertToolHotKey);
	} else if (hWnd == checkAllCaps) {
		val = SendMessage(hWnd, BM_GETCHECK, 0, 0) ? 1 : 0;
		APP_SET_DATA(convertToolToAllCaps, val);
		setLogic(hWnd);
	} else if (hWnd == checkNonAllCaps) {
		val = SendMessage(hWnd, BM_GETCHECK, 0, 0) ? 1 : 0;
		APP_SET_DATA(convertToolToAllNonCaps, val);
		setLogic(hWnd);
	} else if (hWnd == checkRemoveMark) {
		val = SendMessage(hWnd, BM_GETCHECK, 0, 0) ? 1 : 0;
		APP_SET_DATA(convertToolRemoveMark, val);
	} else if (hWnd == checkCapsEarchWord) {
		val = SendMessage(hWnd, BM_GETCHECK, 0, 0) ? 1 : 0;
		APP_SET_DATA(convertToolToCapsEachWord, val);
		setLogic(hWnd);
	} else if (hWnd == checkCapsFirstLetter) {
		val = SendMessage(hWnd, BM_GETCHECK, 0, 0) ? 1 : 0;
		APP_SET_DATA(convertToolToCapsFirstLetter, val);
		setLogic(hWnd);
	} else if (hWnd == checkAlertWhenCompleted) {
		val = SendMessage(hWnd, BM_GETCHECK, 0, 0) ? 0 : 1;
		APP_SET_DATA(convertToolDontAlertWhenCompleted, val);
	}

	SystemTrayHelper::updateData();
}

void ConvertToolDialog::onCharacter(const HWND& hWnd, const UINT16 & keyCode) {
	if (keyCode == 0) return;
	if (hWnd == checkKey) {
		UINT16 code = GET_SWITCH_KEY(convertToolHotKey);
		if (keyCode == VK_DELETE || keyCode == VK_BACK) {
			code = 0xFE;
		} else if (keyCodeToCharacter(keyCode) != 0) {
			code = keyCode;
		}
	
		convertToolHotKey &= 0xFFFFFF00;
		convertToolHotKey |= code;
		convertToolHotKey &= 0x00FFFFFF;
		convertToolHotKey |= ((unsigned int)code << 24);
		APP_SET_DATA(convertToolHotKey, convertToolHotKey);
		setSwitchKeyText(hWnd, code);
		SystemTrayHelper::updateData();
	}
}

void ConvertToolDialog::onSwitchButton() {
	int temp = convertToolFromCode;
	APP_SET_DATA(convertToolFromCode, convertToolToCode);
	APP_SET_DATA(convertToolToCode, temp);
	fillData();
}

void ConvertToolDialog::onConvertButton() {
	if (OpenKeyHelper::quickConvert()) {
		//alert when complete
		if (!convertToolDontAlertWhenCompleted) {
			TCHAR msg[256];
			LoadString(hInstance, IDS_STRING_CONVERT_COMPLETED, msg, 256);
			MessageBox(this->hDlg, msg, _T("OpenKey"), MB_OK);
		}
	}
}