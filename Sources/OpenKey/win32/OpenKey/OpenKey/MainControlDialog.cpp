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

MainControlDialog::MainControlDialog(const HINSTANCE& hInstance, const int& resourceId)
    : BaseDialog(hInstance, resourceId) {
}

MainControlDialog::~MainControlDialog() {
}

void MainControlDialog::initDialog() {
    SET_DIALOG_ICON(IDI_APP_ICON);

    checkCtrl = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_CTRL);
    checkAlt = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_ALT);
    checkWin = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_WIN);
    checkShift = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_SHIFT);
    textSwitchKey = GetDlgItem(hDlg, IDC_SWITCH_KEY_KEY);
    checkBeep = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_BEEP);
    checkRunWithWindows = GetDlgItem(hDlg, IDC_CHECK_RUN_WITH_WINDOWS);
    radioEnglish = GetDlgItem(hDlg, IDC_RADIO_UI_EN);
    radioVietnamese = GetDlgItem(hDlg, IDC_RADIO_UI_VI);

    fillData();
}

void MainControlDialog::applyLanguage() {
    const bool vi = vUILanguage == 1;
    SetDlgItemText(hDlg, IDC_STATIC_SHORTCUT, vi ? _T("Phím tắt chuyển Việt / Anh") : _T("Switch Vietnamese / English shortcut"));
    SetWindowText(checkBeep, vi ? _T("Kêu Beep") : _T("Beep"));
    SetWindowText(checkRunWithWindows, vi ? _T("Khởi động cùng Windows") : _T("Start with Windows"));
    SetDlgItemText(hDlg, IDC_STATIC_LANGUAGE, vi ? _T("Ngôn ngữ") : _T("Language"));
    SetDlgItemText(hDlg, IDBUTTON_EXIT, vi ? _T("Tắt OpenKey") : _T("Exit OpenKey"));
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
        case IDBUTTON_OK: //sent by the window close button (IDCANCEL)
            AppDelegate::getInstance()->closeDialog(this);
            break;
        case IDBUTTON_EXIT:
            AppDelegate::getInstance()->onOpenKeyExit();
            break;
        default:
            if (HIWORD(wParam) == BN_CLICKED) {
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

void MainControlDialog::fillData() {
    SendMessage(checkCtrl, BM_SETCHECK, HAS_CONTROL(vSwitchKeyStatus) ? 1 : 0, 0);
    SendMessage(checkAlt, BM_SETCHECK, HAS_OPTION(vSwitchKeyStatus) ? 1 : 0, 0);
    SendMessage(checkWin, BM_SETCHECK, HAS_COMMAND(vSwitchKeyStatus) ? 1 : 0, 0);
    SendMessage(checkShift, BM_SETCHECK, HAS_SHIFT(vSwitchKeyStatus) ? 1 : 0, 0);
    setSwitchKeyText(textSwitchKey, (vSwitchKeyStatus >> 24) & 0xFF);
    SendMessage(checkBeep, BM_SETCHECK, HAS_BEEP(vSwitchKeyStatus) ? 1 : 0, 0);
    SendMessage(checkRunWithWindows, BM_SETCHECK, vRunWithWindows ? 1 : 0, 0);
    SendMessage(radioEnglish, BM_SETCHECK, vUILanguage == 0 ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(radioVietnamese, BM_SETCHECK, vUILanguage == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
    applyLanguage();
}

void MainControlDialog::setSwitchKey(const unsigned short& code) {
    vSwitchKeyStatus &= 0xFFFFFF00;
    vSwitchKeyStatus |= code;
    vSwitchKeyStatus &= 0x00FFFFFF;
    vSwitchKeyStatus |= ((unsigned int)code << 24);
    APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
}

void MainControlDialog::onCheckboxClicked(const HWND& hWnd) {
    int val = 0;
    if (hWnd == checkCtrl) {
        val = (int)SendMessage(checkCtrl, BM_GETCHECK, 0, 0);
        vSwitchKeyStatus &= (~0x100);
        vSwitchKeyStatus |= val << 8;
        APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
    }
    else if (hWnd == checkAlt) {
        val = (int)SendMessage(checkAlt, BM_GETCHECK, 0, 0);
        vSwitchKeyStatus &= (~0x200);
        vSwitchKeyStatus |= val << 9;
        APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
    }
    else if (hWnd == checkWin) {
        val = (int)SendMessage(checkWin, BM_GETCHECK, 0, 0);
        vSwitchKeyStatus &= (~0x400);
        vSwitchKeyStatus |= val << 10;
        APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
    }
    else if (hWnd == checkShift) {
        val = (int)SendMessage(checkShift, BM_GETCHECK, 0, 0);
        vSwitchKeyStatus &= (~0x800);
        vSwitchKeyStatus |= val << 11;
        APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
    }
    else if (hWnd == checkBeep) {
        val = (int)SendMessage(checkBeep, BM_GETCHECK, 0, 0);
        vSwitchKeyStatus &= (~0x8000);
        vSwitchKeyStatus |= val << 15;
        APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
    }
    else if (hWnd == checkRunWithWindows) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        APP_SET_DATA(vRunWithWindows, val ? 1 : 0);
        OpenKeyHelper::registerRunOnStartup(vRunWithWindows);
    }
    else if (hWnd == radioEnglish || hWnd == radioVietnamese) {
        APP_SET_DATA(vUILanguage, hWnd == radioVietnamese ? 1 : 0);
        applyLanguage();
        SystemTrayHelper::updateData();
    }
}

void MainControlDialog::onCharacter(const HWND& hWnd, const UINT16& keyCode) {
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

void MainControlDialog::setSwitchKeyText(const HWND& hWnd, const UINT16& keyCode) {
    if (keyCode == KEY_SPACE) {
        SetWindowText(hWnd, _T("Space"));
    }
    else if (keyCode == 0xFE) {
        SetWindowText(hWnd, _T(""));
    }
    else {
        Uint16 key[] = { keyCode, 0 };
        SetWindowText(hWnd, (LPCWSTR)&key);
    }
}
