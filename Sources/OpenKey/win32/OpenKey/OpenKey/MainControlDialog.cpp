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
#include "MainControlDialog.h"
#include "AppDelegate.h"

static Uint16 _lastKeyCode;

MainControlDialog::MainControlDialog(const HINSTANCE & hInstance, const int & resourceId) 
	: BaseDialog(hInstance, resourceId) {
}

MainControlDialog::~MainControlDialog() {
}

void MainControlDialog::initDialog() {
	HINSTANCE hIns = GetModuleHandleW(NULL);
	//dialog icon
	SET_DIALOG_ICON(IDI_APP_ICON);

	//set title version
	TCHAR title[256];
	TCHAR titleBuffer[256];
	LoadString(hIns, IDS_MAIN_DIALOG_TITLE, title, 256);
	wsprintfW(titleBuffer, title, OpenKeyHelper::getVersionString().c_str());
	SetWindowText(hDlg, titleBuffer);

	//input type
	comboBoxInputType = GetDlgItem(hDlg, IDC_COMBO_INPUT_TYPE);
	vector<LPCTSTR>& inputType = OpenKeyManager::getInputType();
	for (int i = 0; i < inputType.size(); i++) {
		SendMessage(comboBoxInputType, CB_ADDSTRING, i, reinterpret_cast<LPARAM>(inputType[i]));
	}

	//code table
	comboBoxTableCode = GetDlgItem(hDlg, IDC_COMBO_TABLE_CODE);
	vector<LPCTSTR>& tableCode = OpenKeyManager::getTableCode();
	for (int i = 0; i < tableCode.size(); i++) {
		SendMessage(comboBoxTableCode, CB_ADDSTRING, i, reinterpret_cast<LPARAM>(tableCode[i]));
	}

	checkCtrl = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_CTRL);
	checkAlt = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_ALT);
	checkWin = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_WIN); 
	checkShift = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_SHIFT); 
	textSwitchKey = GetDlgItem(hDlg, IDC_SWITCH_KEY_KEY);
	checkBeep = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_BEEP);

	checkVietnamese = GetDlgItem(hDlg, IDC_RADIO_METHOD_VIETNAMESE); 
	checkEnglish = GetDlgItem(hDlg, IDC_RADIO_METHOD_ENGLISH);
	checkModernOrthorgraphy = GetDlgItem(hDlg, IDC_CHECK_MODERN_ORTHORGRAPHY); 
	checkFixRecommendBrowser = GetDlgItem(hDlg, IDC_CHECK_FIX_RECOMMEND_BROWSER); 
	checkShowOnStartup = GetDlgItem(hDlg, IDC_CHECK_SHOW_ON_STARTUP); 
	checkRunWithWindows = GetDlgItem(hDlg, IDC_CHECK_RUN_WITH_WINDOWS); 
	checkSpelling = GetDlgItem(hDlg, IDC_CHECK_SPELLING);
	checkRestoreIfWrongSpelling = GetDlgItem(hDlg, IDC_CHECK_RESTORE_IF_WRONG_SPELLING); 
	checkUseClipboard = GetDlgItem(hDlg, IDC_CHECK_USE_CLIPBOARD);
	checkModernIcon = GetDlgItem(hDlg, IDC_CHECK_MODERN_ICON);

	checkSmartSwitchKey = GetDlgItem(hDlg, IDC_CHECK_SMART_SWITCH_KEY);
	checkCapsFirstChar = GetDlgItem(hDlg, IDC_CHECK_CAPS_FIRST_CHAR); 
	checkQuickTelex = GetDlgItem(hDlg, IDC_CHECK_QUICK_TELEX);
	checkUseMacro = GetDlgItem(hDlg, IDC_CHECK_USE_MACRO);
	checkUseMacroInEnglish = GetDlgItem(hDlg, IDC_CHECK_USE_MACRO_IN_ENGLISH);

	SendDlgItemMessage(hDlg, IDBUTTON_OK, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(hIns, MAKEINTRESOURCEW(IDI_ICON_OK_BUTTON)));
	SendDlgItemMessage(hDlg, ID_BTN_DEFAULT, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(hIns, MAKEINTRESOURCEW(IDI_ICON_DEFAULT_BUTTON)));
	SendDlgItemMessage(hDlg, IDBUTTON_EXIT, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(hIns, MAKEINTRESOURCEW(IDI_ICON_EXIT_BUTTON)));
	fillData();
}

INT_PTR MainControlDialog::eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		this->hDlg = hDlg;
		initDialog();
		return TRUE;
	case WM_COMMAND: {
		int wmId = LOWORD(wParam);
		switch (wmId) {
		case IDBUTTON_OK:
			AppDelegate::getInstance()->closeDialog(this);
			break;
		case IDBUTTON_EXIT:
			AppDelegate::getInstance()->onOpenKeyExit();
			break;
		case ID_BTN_DEFAULT:
			AppDelegate::getInstance()->onDefaultConfig();
			break;
		case IDC_BUTTON_MACRO_TABLE:
			AppDelegate::getInstance()->onMacroTable();
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
	}
	}

	return FALSE;
}

void MainControlDialog::fillData() {
	SendMessage(comboBoxInputType, CB_SETCURSEL, vInputType, 0);
	SendMessage(comboBoxTableCode, CB_SETCURSEL, vCodeTable, 0);
	
	SendMessage(checkCtrl, BM_SETCHECK, HAS_CONTROL(vSwitchKeyStatus) ? 1 : 0, 0);
	SendMessage(checkAlt, BM_SETCHECK, HAS_OPTION(vSwitchKeyStatus) ? 1 : 0, 0);
	SendMessage(checkWin, BM_SETCHECK, HAS_COMMAND(vSwitchKeyStatus) ? 1 : 0, 0);
	SendMessage(checkShift, BM_SETCHECK, HAS_SHIFT(vSwitchKeyStatus) ? 1 : 0, 0);
	setSwitchKeyText(textSwitchKey, (vSwitchKeyStatus >> 24) & 0xFF);
	SendMessage(checkBeep, BM_SETCHECK, HAS_BEEP(vSwitchKeyStatus) ? 1 : 0, 0);

	SendMessage(checkVietnamese, BM_SETCHECK, vLanguage, 0);
	SendMessage(checkEnglish, BM_SETCHECK, !vLanguage, 0);
	
	SendMessage(checkModernOrthorgraphy, BM_SETCHECK, vUseModernOrthography ? 1 : 0, 0);
	SendMessage(checkFixRecommendBrowser, BM_SETCHECK, vFixRecommendBrowser ? 1 : 0, 0);
	SendMessage(checkShowOnStartup, BM_SETCHECK, vShowOnStartUp ? 1 : 0, 0);
	SendMessage(checkRunWithWindows, BM_SETCHECK, vRunWithWindows ? 1 : 0, 0);
	SendMessage(checkSpelling, BM_SETCHECK, vCheckSpelling ? 1 : 0, 0);
	SendMessage(checkRestoreIfWrongSpelling, BM_SETCHECK, vRestoreIfWrongSpelling ? 1 : 0, 0);
	EnableWindow(checkRestoreIfWrongSpelling, vCheckSpelling);
	SendMessage(checkUseClipboard, BM_SETCHECK, vSendKeyStepByStep ? 0 : 1, 0);
	SendMessage(checkModernIcon, BM_SETCHECK, vUseGrayIcon ? 1 : 0, 0);
	
	SendMessage(checkSmartSwitchKey, BM_SETCHECK, vUseSmartSwitchKey ? 1 : 0, 0);
	SendMessage(checkCapsFirstChar, BM_SETCHECK, vUpperCaseFirstChar ? 1 : 0, 0);
	SendMessage(checkQuickTelex, BM_SETCHECK, vQuickTelex ? 1 : 0, 0);
	SendMessage(checkUseMacro, BM_SETCHECK, vUseMacro ? 1 : 0, 0);
	SendMessage(checkUseMacroInEnglish, BM_SETCHECK, vUseMacroInEnglishMode ? 1 : 0, 0);
}

void MainControlDialog::setSwitchKey(const unsigned short& code) {
	vSwitchKeyStatus &= 0xFFFFFF00;
	vSwitchKeyStatus |= code;
	vSwitchKeyStatus &= 0x00FFFFFF;
	vSwitchKeyStatus |= ((unsigned int)code << 24);
	APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
}

void MainControlDialog::onComboBoxSelected(const HWND & hCombobox, const int & comboboxId) {
	if (hCombobox == comboBoxInputType) {
		APP_SET_DATA(vInputType, (int)SendMessage(hCombobox, CB_GETCURSEL, 0, 0));
	} else if (hCombobox == comboBoxTableCode) {
		APP_SET_DATA(vCodeTable, (int)SendMessage(hCombobox, CB_GETCURSEL, 0, 0));
	}
	SystemTrayHelper::updateData();
}

void MainControlDialog::onCheckboxClicked(const HWND & hWnd) {
	int val = 0;
	if (hWnd == checkCtrl) {
		val = (int)SendMessage(checkCtrl, BM_GETCHECK, 0, 0);
		vSwitchKeyStatus &= (~0x100);
		vSwitchKeyStatus |= val << 8;
		APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	} else if (hWnd == checkAlt) {
		val = (int)SendMessage(checkAlt, BM_GETCHECK, 0, 0);
		vSwitchKeyStatus &= (~0x200);
		vSwitchKeyStatus |= val << 9;
		APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	} else if (hWnd == checkWin) {
		val = (int)SendMessage(checkWin, BM_GETCHECK, 0, 0);
		vSwitchKeyStatus &= (~0x400);
		vSwitchKeyStatus |= val << 10;
		APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	} else if (hWnd == checkShift) {
		val = (int)SendMessage(checkShift, BM_GETCHECK, 0, 0);
		vSwitchKeyStatus &= (~0x800);
		vSwitchKeyStatus |= val << 11;
		APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	} else if (hWnd == checkBeep) {
		val = (int)SendMessage(checkBeep, BM_GETCHECK, 0, 0);
		vSwitchKeyStatus &= (~0x8000);
		vSwitchKeyStatus |= val << 15;
		APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	} else if (hWnd == checkVietnamese) {
		val = (int)SendMessage(checkVietnamese, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vLanguage, val ? 1 : 0);
	} else if (hWnd == checkEnglish) {
		val = (int)SendMessage(checkVietnamese, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vLanguage, val ? 1 : 0);
		if (vUseSmartSwitchKey) {
			setAppInputMethodStatus(OpenKeyHelper::getFrontMostAppExecuteName(), vLanguage);
			saveSmartSwitchKeyData();
		}
	} else if (hWnd == checkModernOrthorgraphy) {
		val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vUseModernOrthography, val ? 1 : 0);
	} else if (hWnd == checkFixRecommendBrowser) {
		val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vFixRecommendBrowser, val ? 1 : 0);
	} else if (hWnd == checkShowOnStartup) {
		val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vShowOnStartUp, val ? 1 : 0);
	} else if (hWnd == checkRunWithWindows) {
		val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vRunWithWindows, val ? 1 : 0);
		OpenKeyHelper::registerRunOnStartup(vRunWithWindows);
	} else if (hWnd == checkSpelling) {
		val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vCheckSpelling, val ? 1 : 0);
		EnableWindow(checkRestoreIfWrongSpelling, vCheckSpelling);
	} else if (hWnd == checkRestoreIfWrongSpelling) {
		val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vRestoreIfWrongSpelling, val ? 1 : 0);
	} else if (hWnd == checkUseClipboard) {
		val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vSendKeyStepByStep, val ? 0 : 1);
	} else if (hWnd == checkSmartSwitchKey) {
		val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vUseSmartSwitchKey, val ? 1 : 0);
	} else if (hWnd == checkCapsFirstChar) {
		val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vUpperCaseFirstChar, val ? 1 : 0);
	} else if (hWnd == checkQuickTelex) {
		val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vQuickTelex, val ? 1 : 0);
	} else if (hWnd == checkUseMacro) {
		val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vUseMacro, val ? 1 : 0);
	} else if (hWnd == checkUseMacroInEnglish) {
		val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vUseMacroInEnglishMode, val ? 1 : 0);
	} else if (hWnd == checkModernIcon) {
		val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
		APP_SET_DATA(vUseGrayIcon, val ? 1 : 0);
	}
	SystemTrayHelper::updateData();
}

void MainControlDialog::onCharacter(const HWND& hWnd, const UINT16 & keyCode) {
	if (keyCode == 0) return;
	if (hWnd == textSwitchKey) {
		UINT16 code = GET_SWITCH_KEY(vSwitchKeyStatus);
		if (keyCode == VK_DELETE || keyCode == VK_BACK) {
			code = 0xFE;
		}
		else if (keyCodeToCharacter(keyCode) != 0) {
			code = keyCode;
		}
		setSwitchKey(code);
		setSwitchKeyText(hWnd, code);
	}
}

void MainControlDialog::setSwitchKeyText(const HWND & hWnd, const UINT16 & keyCode) {
	if (keyCode == KEY_SPACE) {
		SetWindowText(hWnd, _T("Space"));
	} else if (keyCode == 0xFE) {
		SetWindowText(hWnd, _T(""));
	} else {
		Uint16 key[] = { keyCode, 0 };
		SetWindowText(hWnd, (LPCWSTR)&key);
	}
}
