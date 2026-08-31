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
#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include "AppDelegate.h"
#include <fstream>

#pragma comment(lib, "imm32")
#define IMC_GETOPENSTATUS 0x0005

#define MASK_SHIFT				0x01
#define MASK_CONTROL			0x02
#define MASK_ALT				0x04
#define MASK_CAPITAL			0x08
#define MASK_NUMLOCK			0x10
#define MASK_WIN				0x20
#define MASK_SCROLL				0x40

#define OTHER_CONTROL_KEY (_flag & MASK_ALT) || (_flag & MASK_CONTROL) || (_flag & MASK_WIN)
#define DYNA_DATA(macro, pos) (macro ? pData->macroData[pos] : pData->charData[pos])
#define EMPTY_HOTKEY 0xFE0000FE

static vector<string> _chromiumBrowser = {
	"chrome.exe", "brave.exe", "msedge.exe"
};

extern int vSendKeyStepByStep;
extern int vUseGrayIcon;
extern int vShowOnStartUp;
extern int vRunWithWindows;

static HHOOK hKeyboardHook;
static HHOOK hMouseHook;
static HWINEVENTHOOK hSystemEvent;
static KBDLLHOOKSTRUCT* keyboardData;
static MSLLHOOKSTRUCT* mouseData;
static vKeyHookState* pData;
static vector<Uint16> _syncKey;
static Uint32 _flag = 0, _lastFlag = 0, _privateFlag;
static bool _flagChanged = false, _isFlagKey;
static Uint16 _keycode;
static Uint16 _newChar, _newCharHi;

static inline void SyncModifierFlags() {
	if ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) || (GetAsyncKeyState(VK_RSHIFT) & 0x8000)) _flag |= MASK_SHIFT;
	else _flag &= ~MASK_SHIFT;

	if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) || (GetAsyncKeyState(VK_RCONTROL) & 0x8000)) _flag |= MASK_CONTROL;
	else _flag &= ~MASK_CONTROL;

	if ((GetAsyncKeyState(VK_LMENU) & 0x8000) || (GetAsyncKeyState(VK_RMENU) & 0x8000)) _flag |= MASK_ALT;
	else _flag &= ~MASK_ALT;

	if ((GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000)) _flag |= MASK_WIN;
	else _flag &= ~MASK_WIN;

	if (GetKeyState(VK_CAPITAL) & 0x0001) _flag |= MASK_CAPITAL;
	else _flag &= ~MASK_CAPITAL;

	if (GetKeyState(VK_NUMLOCK) & 0x0001) _flag |= MASK_NUMLOCK;
	else _flag &= ~MASK_NUMLOCK;

	if (GetKeyState(VK_SCROLL) & 0x0001) _flag |= MASK_SCROLL;
	else _flag &= ~MASK_SCROLL;
}

static HWND hOverlayWnd = NULL;
static UINT_PTR overlayTimerId = 0;
static wstring overlayText = L"";

static LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rc;
		GetClientRect(hWnd, &rc);

		HBRUSH bgBrush = CreateSolidBrush(RGB(30, 30, 30));
		FillRect(hdc, &rc, bgBrush);
		DeleteObject(bgBrush);

		HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(0, 150, 255));
		HGDIOBJ oldPen = SelectObject(hdc, borderPen);
		HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
		Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject(hdc, oldPen);
		SelectObject(hdc, oldBrush);
		DeleteObject(borderPen);

		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(255, 255, 255));
		HFONT hFont = CreateFont(13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("Segoe UI"));
		HGDIOBJ oldFont = SelectObject(hdc, hFont);

		DrawText(hdc, overlayText.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		SelectObject(hdc, oldFont);
		DeleteObject(hFont);
		EndPaint(hWnd, &ps);
		return 0;
	} else if (msg == WM_TIMER) {
		KillTimer(hWnd, wParam);
		ShowWindow(hWnd, SW_HIDE);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

static void InitCaretOverlay() {
	if (hOverlayWnd) return;
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = OverlayWndProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = _T("OpenKeyCaretOverlay");
	RegisterClass(&wc);

	hOverlayWnd = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
		_T("OpenKeyCaretOverlay"), _T(""),
		WS_POPUP, 0, 0, 24, 18,
		NULL, NULL, GetModuleHandle(NULL), NULL
	);
	if (hOverlayWnd) {
		SetLayeredWindowAttributes(hOverlayWnd, 0, 225, LWA_ALPHA);
	}
}

void ShowCaretOverlay(bool isVietnamese) {
	if (!hOverlayWnd) InitCaretOverlay();
	if (!hOverlayWnd) return;

	overlayText = isVietnamese ? L"V" : L"E";

	POINT pt = { 0, 0 };
	HWND fg = GetForegroundWindow();
	bool foundPos = false;

	if (fg) {
		DWORD threadId = GetWindowThreadProcessId(fg, NULL);
		GUITHREADINFO gti = { sizeof(GUITHREADINFO) };

		if (GetGUIThreadInfo(threadId, &gti)) {
			if (gti.hwndCaret && (gti.rcCaret.right > gti.rcCaret.left || gti.rcCaret.bottom > gti.rcCaret.top)) {
				pt.x = gti.rcCaret.left;
				pt.y = gti.rcCaret.bottom + 4;
				ClientToScreen(gti.hwndCaret, &pt);
				foundPos = true;
			} else if (gti.hwndFocus) {
				RECT focusRect;
				GetWindowRect(gti.hwndFocus, &focusRect);
				pt.x = focusRect.left + 16;
				pt.y = focusRect.top + 16;
				foundPos = true;
			}
		}

		if (!foundPos) {
			RECT winRect;
			GetWindowRect(fg, &winRect);
			pt.x = winRect.left + 24;
			pt.y = winRect.top + 32;
			foundPos = true;
		}
	}

	if (!foundPos) {
		GetCursorPos(&pt);
		pt.x += 12;
		pt.y += 18;
	}

	SetWindowPos(hOverlayWnd, HWND_TOPMOST, pt.x, pt.y, 24, 20, SWP_SHOWWINDOW | SWP_NOACTIVATE);
	InvalidateRect(hOverlayWnd, NULL, TRUE);

	if (overlayTimerId) {
		KillTimer(hOverlayWnd, overlayTimerId);
	}
	overlayTimerId = SetTimer(hOverlayWnd, 1, 1600, NULL);
}

static vector<Uint16> _newCharString;
static Uint16 _newCharSize;
static bool _willSendControlKey = false;

static Uint16 _uniChar[2];
static int _i, _j, _k;
static Uint32 _tempChar;
static string macroText, macroContent;
static int _languageTemp = 0; //use for smart switch key
static vector<Byte> savedSmartSwitchKeyData; ////use for smart switch key

static bool isTypingContext(const string& exeName) {
	HWND fg = GetForegroundWindow();
	if (!fg) return false;
	DWORD threadId = GetWindowThreadProcessId(fg, NULL);
	GUITHREADINFO gti = { sizeof(GUITHREADINFO) };
	if (GetGUIThreadInfo(threadId, &gti)) {
		if (gti.hwndCaret && (gti.rcCaret.right > gti.rcCaret.left || gti.rcCaret.bottom > gti.rcCaret.top)) {
			return true;
		}
		if (gti.hwndFocus && gti.hwndFocus != fg) {
			TCHAR className[64] = { 0 };
			if (GetClassName(gti.hwndFocus, className, 64)) {
				if (_tcsicmp(className, _T("Edit")) == 0 || _tcsicmp(className, _T("RichEdit")) == 0 ||
					_tcsnicmp(className, _T("RichEdit20"), 10) == 0 || _tcsnicmp(className, _T("RICHEDIT50"), 10) == 0) {
					return true;
				}
			}
		}
	}
	return false;
}

static bool _inGameChatTyping = false;

static bool isGameTypingActive(const string& exeName, Uint16 vkCode, WPARAM wParam) {
	if (!isGameApp(exeName)) return false;
	if (isTypingContext(exeName)) {
		_inGameChatTyping = true;
		return true;
	}
	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
		if (vkCode == VK_RETURN) {
			_inGameChatTyping = !_inGameChatTyping;
			if (_inGameChatTyping) {
				ShowCaretOverlay(true);
			}
			return _inGameChatTyping;
		} else if (vkCode == VK_ESCAPE) {
			_inGameChatTyping = false;
			return false;
		}
	}
	return _inGameChatTyping;
}

static bool _hasJustUsedHotKey = false;

static INPUT backspaceEvent[2];
static INPUT keyEvent[2];

static wstring getExecuteFolderPath() {
	wstring path = OpenKeyHelper::getExecutePath();
	size_t pos = path.find_last_of(L"\\/");
	if (pos != wstring::npos) {
		path = path.substr(0, pos);
	}
	return path;
}

static bool fileExists(const wstring& path) {
	ifstream file(wideStringToUtf8(path).c_str(), ios::in | ios::binary);
	return file.is_open();
}

LRESULT CALLBACK keyboardHookProcess(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK mouseHookProcess(int nCode, WPARAM wParam, LPARAM lParam);
VOID CALLBACK winEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

void OpenKeyFree() {
	UnhookWindowsHookEx(hMouseHook);
	UnhookWindowsHookEx(hKeyboardHook);
	UnhookWinEvent(hSystemEvent);
}

void OpenKeyInit() {
	APP_GET_DATA(vLanguage, 1);
	APP_GET_DATA(vInputType, 0);
	vFreeMark = 0;
	APP_GET_DATA(vCodeTable, 0);
	APP_GET_DATA(vCheckSpelling, 1);
	APP_GET_DATA(vUseModernOrthography, 0);
	APP_GET_DATA(vQuickTelex, 0);
	APP_GET_DATA(vSwitchKeyStatus, 0xFE0009FE);
	APP_GET_DATA(vRestoreIfWrongSpelling, 1);
	APP_GET_DATA(vFixRecommendBrowser, 1);
	APP_GET_DATA(vUseMacro, 1);
	APP_GET_DATA(vUseMacroInEnglishMode, 0);
	APP_GET_DATA(vAutoCapsMacro, 0);
	APP_GET_DATA(vSendKeyStepByStep, 1);
	APP_GET_DATA(vMacroTriggerMask, 3);
	APP_GET_DATA(vUseGrayIcon, 0);
	APP_GET_DATA(vShowOnStartUp, 1);
	APP_GET_DATA(vRunWithWindows, 1);
	OpenKeyHelper::registerRunOnStartup(vRunWithWindows);
	APP_GET_DATA(vUseSmartSwitchKey, 1);
	APP_GET_DATA(vUpperCaseFirstChar, 0);
	APP_GET_DATA(vAllowConsonantZFWJ, 0);
	APP_GET_DATA(vTempOffSpelling, 0);
	APP_GET_DATA(vQuickStartConsonant, 0);
	APP_GET_DATA(vQuickEndConsonant, 0);
	APP_GET_DATA(vSupportMetroApp, 0);
	APP_GET_DATA(vRunAsAdmin, 0);
	APP_GET_DATA(vCreateDesktopShortcut, 0);
	APP_GET_DATA(vCheckNewVersion, 0);
	APP_GET_DATA(vRememberCode, 1);
	APP_GET_DATA(vOtherLanguage, 1);
	APP_GET_DATA(vTempOffOpenKey, 0);
	APP_GET_DATA(vFixChromiumBrowser, 0);
	APP_GET_DATA(vBlockBackslash, 0);
	APP_GET_DATA(vShortcutModifier, (MASK_CONTROL | MASK_ALT));
	APP_GET_DATA(vShortcutF4CodeTable, 2);
	APP_GET_DATA(vShortcutEnableMask, 0x0FFF);

	//init convert tool
	APP_GET_DATA(convertToolDontAlertWhenCompleted, 0);
	APP_GET_DATA(convertToolToAllCaps, 0);
	APP_GET_DATA(convertToolToAllNonCaps, 0);
	APP_GET_DATA(convertToolToCapsFirstLetter, 0);
	APP_GET_DATA(convertToolToCapsEachWord, 0);
	APP_GET_DATA(convertToolRemoveMark, 0);
	APP_GET_DATA(convertToolFromCode, 0);
	APP_GET_DATA(convertToolToCode, 0);
	APP_GET_DATA(convertToolHotKey, EMPTY_HOTKEY);
	if (convertToolHotKey == 0) {
		convertToolHotKey = EMPTY_HOTKEY;
	}

	pData = (vKeyHookState*)vKeyInit();

	//pre-create back key
	backspaceEvent[0].type = INPUT_KEYBOARD;
	backspaceEvent[0].ki.dwFlags = 0;
	backspaceEvent[0].ki.wVk = VK_BACK;
	backspaceEvent[0].ki.wScan = 0;
	backspaceEvent[0].ki.time = 0;
	backspaceEvent[0].ki.dwExtraInfo = 1;

	backspaceEvent[1].type = INPUT_KEYBOARD;
	backspaceEvent[1].ki.dwFlags = KEYEVENTF_KEYUP;
	backspaceEvent[1].ki.wVk = VK_BACK;
	backspaceEvent[1].ki.wScan = 0;
	backspaceEvent[1].ki.time = 0;
	backspaceEvent[1].ki.dwExtraInfo = 1;

	//get key state
	SyncModifierFlags();

	//init and load macro data from text file, fallback to old text/binary data
	wstring macroPath = getExecuteFolderPath() + L"\\openkeymacro.txt";
	wstring oldMacroPath = getExecuteFolderPath() + L"\\openkeymacro.md";
	if (fileExists(macroPath)) {
		readFromFile(wideStringToUtf8(macroPath), false);
	} else if (fileExists(oldMacroPath)) {
		readFromFile(wideStringToUtf8(oldMacroPath), false);
		saveToFile(wideStringToUtf8(macroPath));
	} else {
		DWORD macroDataSize;
		BYTE* macroData = OpenKeyHelper::getRegBinary(_T("macroData"), macroDataSize);
		initMacroMap((Byte*)macroData, (int)macroDataSize);
		saveToFile(wideStringToUtf8(macroPath));
	}

	//init and load smart switch key data
	DWORD smartSwitchKeySize;
	BYTE* data = OpenKeyHelper::getRegBinary(_T("smartSwitchKey"), smartSwitchKeySize);
	initSmartSwitchKey((Byte*)data, (int)smartSwitchKeySize);

	//load custom rules
	DWORD customRulesSize = 0;
	BYTE* customRulesData = OpenKeyHelper::getRegBinary(_T("customRules"), customRulesSize);
	if (customRulesData && customRulesSize > 0) {
		customRules.assign((CustomRule*)customRulesData, (CustomRule*)customRulesData + (customRulesSize / sizeof(CustomRule)));
		free(customRulesData);
	}

	//init hook
	HINSTANCE hInstance = GetModuleHandle(NULL);
	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookProcess, hInstance, 0);
	hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseHookProcess, hInstance, 0);
	hSystemEvent = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, winEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
}

void saveSmartSwitchKeyData() {
	getSmartSwitchKeySaveData(savedSmartSwitchKeyData);
	OpenKeyHelper::setRegBinary(_T("smartSwitchKey"), savedSmartSwitchKeyData.data(), (int)savedSmartSwitchKeyData.size());
}

static void InsertKeyLength(const Uint8& len) {
	_syncKey.push_back(len);
}

static inline void prepareKeyEvent(INPUT& input, const Uint16& keycode, const bool& isPress, const DWORD& flag=0) {
	input.type = INPUT_KEYBOARD;
	input.ki.dwFlags = isPress ? flag : flag|KEYEVENTF_KEYUP;
	input.ki.wVk = keycode;
	input.ki.wScan = 0;
	input.ki.time = 0;
	input.ki.dwExtraInfo = 1;
}

static inline void prepareUnicodeEvent(INPUT& input, const Uint16& unicode, const bool& isPress) {
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = 0;
	input.ki.wScan = unicode;
	input.ki.time = 0;
	input.ki.dwFlags = (isPress ? 0 : KEYEVENTF_KEYUP) | KEYEVENTF_UNICODE;
	input.ki.dwExtraInfo = 1;
}

static vector<INPUT> _batchInputEvents;

static inline void pushKeyEvent(const Uint16& keycode, const bool& isPress, const DWORD& flag=0) {
	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki.dwFlags = isPress ? flag : flag|KEYEVENTF_KEYUP;
	input.ki.wVk = keycode;
	input.ki.wScan = 0;
	input.ki.time = 0;
	input.ki.dwExtraInfo = 1;
	_batchInputEvents.push_back(input);
}

static inline void pushUnicodeEvent(const Uint16& unicode, const bool& isPress) {
	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = 0;
	input.ki.wScan = unicode;
	input.ki.time = 0;
	input.ki.dwFlags = (isPress ? 0 : KEYEVENTF_KEYUP) | KEYEVENTF_UNICODE;
	input.ki.dwExtraInfo = 1;
	_batchInputEvents.push_back(input);
}

static void FlushBatchInput() {
	if (!_batchInputEvents.empty()) {
		SendInput((UINT)_batchInputEvents.size(), _batchInputEvents.data(), sizeof(INPUT));
		_batchInputEvents.clear();
	}
}

static void SendCombineKey(const Uint16& key1, const Uint16& key2, const DWORD& flagKey1=0, const DWORD& flagKey2 = 0) {
	prepareKeyEvent(keyEvent[0], key1, true, flagKey1);
	SendInput(1, keyEvent, sizeof(INPUT));

	prepareKeyEvent(keyEvent[0], key2, true, flagKey2);
	prepareKeyEvent(keyEvent[1], key2, false, flagKey2);
	SendInput(2, keyEvent, sizeof(INPUT));

	prepareKeyEvent(keyEvent[0], key1, false, flagKey1);
	SendInput(1, keyEvent, sizeof(INPUT));
}

static void QueueKeyCode(Uint32 data) {
	if (data & PURE_CHARACTER_MASK) {
		Uint32 pureChar = data & ~PURE_CHARACTER_MASK;
		if (pureChar <= 0xFFFF) {
			pushUnicodeEvent((Uint16)pureChar, true);
			pushUnicodeEvent((Uint16)pureChar, false);
		} else {
			Uint16 high = (Uint16)(((pureChar - 0x10000) >> 10) + 0xD800);
			Uint16 low = (Uint16)(((pureChar - 0x10000) & 0x3FF) + 0xDC00);
			pushUnicodeEvent(high, true);
			pushUnicodeEvent(high, false);
			pushUnicodeEvent(low, true);
			pushUnicodeEvent(low, false);
		}
		return;
	}
	_newChar = (Uint16)data;
	if (!(data & CHAR_CODE_MASK)) {
		if (IS_DOUBLE_CODE(vCodeTable)) //VNI
			InsertKeyLength(1);

		_newChar = keyCodeToCharacter(data);
		if (_newChar == 0) {
			_newChar = (Uint16)data;
			pushKeyEvent(_newChar, true);
			pushKeyEvent(_newChar, false);
		} else {
			pushUnicodeEvent(_newChar, true);
			pushUnicodeEvent(_newChar, false);
		}
	} else {
		if (vCodeTable == 0) { //unicode 2 bytes code
			pushUnicodeEvent(_newChar, true);
			pushUnicodeEvent(_newChar, false);
		} else if (vCodeTable == 1 || vCodeTable == 2 || vCodeTable == 4) { //others such as VNI Windows, TCVN3: 1 byte code
			_newCharHi = HIBYTE(_newChar);
			_newChar = LOBYTE(_newChar);

			pushUnicodeEvent(_newChar, true);
			pushUnicodeEvent(_newChar, false);

			if (_newCharHi > 32) {
				if (vCodeTable == 2) //VNI
					InsertKeyLength(2);
				pushUnicodeEvent(_newCharHi, true);
				pushUnicodeEvent(_newCharHi, false);
			} else {
				if (vCodeTable == 2) //VNI
					InsertKeyLength(1);
			}
		} else if (vCodeTable == 3) { //Unicode Compound
			_newCharHi = (_newChar >> 13);
			_newChar &= 0x1FFF;
			_uniChar[0] = _newChar;
			_uniChar[1] = _newCharHi > 0 ? (_unicodeCompoundMark[_newCharHi - 1]) : 0;
			InsertKeyLength(_newCharHi > 0 ? 2 : 1);
			pushUnicodeEvent(_uniChar[0], true);
			pushUnicodeEvent(_uniChar[0], false);
			if (_newCharHi > 0) {
				pushUnicodeEvent(_uniChar[1], true);
				pushUnicodeEvent(_uniChar[1], false);
			}
		}
	}
}

static void SendKeyCode(Uint32 data) {
	QueueKeyCode(data);
	FlushBatchInput();
}

static void QueueBackspace() {
	pushKeyEvent(VK_BACK, true);
	pushKeyEvent(VK_BACK, false);
	if (IS_DOUBLE_CODE(vCodeTable) && !_syncKey.empty()) { //VNI or Unicode Compound
		if (_syncKey.back() > 1) {
			pushKeyEvent(VK_BACK, true);
			pushKeyEvent(VK_BACK, false);
		}
		_syncKey.pop_back();
	}
}

static void SendBackspace() {
	QueueBackspace();
	FlushBatchInput();
	if (vSupportMetroApp && OpenKeyHelper::getLastAppExecuteName().compare("ApplicationFrameHost.exe") == 0) {//Metro App
		SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
		SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
	}
}

static void SendEmptyCharacter() {
	if (IS_DOUBLE_CODE(vCodeTable)) //VNI or Unicode Compound
		InsertKeyLength(1);

	_newChar = 0x202F; //empty char

	prepareUnicodeEvent(keyEvent[0], _newChar, true);
	prepareUnicodeEvent(keyEvent[1], _newChar, false);
	SendInput(2, keyEvent, sizeof(INPUT));
}

static void SendNewCharString(const bool& dataFromMacro = false) {
	_j = 0;
	_newCharSize = dataFromMacro ? (Uint16)pData->macroData.size() : pData->newCharCount;
	if (_newCharString.size() < _newCharSize + 2) {
		_newCharString.resize(_newCharSize + 2);
	}
	_willSendControlKey = false;
	
	if (_newCharSize > 0) {
		for (_k = dataFromMacro ? 0 : pData->newCharCount - 1;
			dataFromMacro ? _k < pData->macroData.size() : _k >= 0;
			dataFromMacro ? _k++ : _k--) {

			_tempChar = DYNA_DATA(dataFromMacro, _k);
			if (_tempChar & PURE_CHARACTER_MASK) {
				Uint32 pureChar = _tempChar & ~PURE_CHARACTER_MASK;
				if (pureChar <= 0xFFFF) {
					if (_newCharString.size() < _j + 2) {
						_newCharString.resize(_j + 2);
					}
					_newCharString[_j++] = (Uint16)pureChar;
					if (IS_DOUBLE_CODE(vCodeTable)) {
						InsertKeyLength(1);
					}
				} else {
					Uint16 high = (Uint16)(((pureChar - 0x10000) >> 10) + 0xD800);
					Uint16 low = (Uint16)(((pureChar - 0x10000) & 0x3FF) + 0xDC00);
					if (_newCharString.size() < _j + 3) {
						_newCharString.resize(_j + 3);
					}
					_newCharString[_j++] = high;
					_newCharString[_j++] = low;
					_newCharSize++;
					if (IS_DOUBLE_CODE(vCodeTable)) {
						InsertKeyLength(2);
					}
				}
			} else if (!(_tempChar & CHAR_CODE_MASK)) {
				if (IS_DOUBLE_CODE(vCodeTable)) //VNI
					InsertKeyLength(1);
				if (_newCharString.size() < _j + 2) {
					_newCharString.resize(_j + 2);
				}
				_newCharString[_j++] = keyCodeToCharacter(_tempChar);
			} else {
				_newChar = _tempChar;
				if (vCodeTable == 0) {  //unicode 2 bytes code
					if (_newCharString.size() < _j + 2) {
						_newCharString.resize(_j + 2);
					}
					_newCharString[_j++] = _newChar;
				} else if (vCodeTable == 1 || vCodeTable == 2 || vCodeTable == 4) { //others such as VNI Windows, TCVN3: 1 byte code
					_newCharHi = HIBYTE(_newChar);
					_newChar = LOBYTE(_newChar);
					if (_newCharString.size() < _j + 2) {
						_newCharString.resize(_j + 2);
					}
					_newCharString[_j++] = _newChar;

					if (_newCharHi > 32) {
						if (vCodeTable == 2) //VNI
							InsertKeyLength(2);
						if (_newCharString.size() < _j + 2) {
							_newCharString.resize(_j + 2);
						}
						_newCharString[_j++] = _newCharHi;
						_newCharSize++;
					}
					else {
						if (vCodeTable == 2) //VNI
							InsertKeyLength(1);
					}
				} else if (vCodeTable == 3) { //Unicode Compound
					_newCharHi = (_newChar >> 13);
					_newChar &= 0x1FFF;

					InsertKeyLength(_newCharHi > 0 ? 2 : 1);
					if (_newCharString.size() < _j + 2) {
						_newCharString.resize(_j + 2);
					}
					_newCharString[_j++] = _newChar;
					if (_newCharHi > 0) {
						_newCharSize++;
						if (_newCharString.size() < _j + 2) {
							_newCharString.resize(_j + 2);
						}
						_newCharString[_j++] = _unicodeCompoundMark[_newCharHi - 1];
					}

				}
			}
		}//end for
	}

	if (pData->code == vRestore || pData->code == vRestoreAndStartNewSession) { //if is restore
		if (keyCodeToCharacter(_keycode) != 0) {
			_newCharSize++;
			if (_newCharString.size() < _j + 2) {
				_newCharString.resize(_j + 2);
			}
			_newCharString[_j++] = keyCodeToCharacter(_keycode | ((_flag & MASK_SHIFT) || (_flag & MASK_CAPITAL) ? CAPS_MASK : 0));
		} else {
			_willSendControlKey = true;
		}
	}
	if (pData->code == vRestoreAndStartNewSession) {
		startNewSession();
	}

	if (_newCharString.size() < _j + 1) {
		_newCharString.resize(_j + 1);
	}
	_newCharString[_j] = 0;

	OpenKeyHelper::setClipboardText((LPCTSTR)_newCharString.data(), _j + 1, CF_UNICODETEXT);

	if (!dataFromMacro) {
		//Send shift + insert
		SendCombineKey(KEY_LEFT_SHIFT, VK_INSERT, 0, KEYEVENTF_EXTENDEDKEY);
	}
	
	//the case when hCode is vRestore or vRestoreAndStartNewSession,
	//the word is invalid and last key is control key such as TAB, LEFT ARROW, RIGHT ARROW,...
	if (_willSendControlKey) {
		SendKeyCode(_keycode);
	}
}

static void SendNewCharStringWithClipboardRestore() {
	wstring backup = OpenKeyHelper::getClipboardText(CF_UNICODETEXT);
	SendNewCharString();
	if (!backup.empty()) {
		Sleep(100);
		for (int retry = 3; retry-- > 0;) {
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (backup.length() + 1) * sizeof(WCHAR));
			if (hMem) {
				memcpy(GlobalLock(hMem), backup.c_str(), (backup.length() + 1) * sizeof(WCHAR));
				GlobalUnlock(hMem);
				if (OpenClipboard(0)) {
					EmptyClipboard();
					SetClipboardData(CF_UNICODETEXT, hMem);
					CloseClipboard();
					break;
				}
				GlobalFree(hMem);
			}
			Sleep(10);
		}
	}
}

bool checkHotKey(int hotKeyData, bool checkKeyCode = true) {
	if ((hotKeyData & (~0x8000)) == EMPTY_HOTKEY)
		return false;
	if (HAS_CONTROL(hotKeyData) ^ GET_BOOL(_lastFlag & MASK_CONTROL))
		return false;
	if (HAS_OPTION(hotKeyData) ^ GET_BOOL(_lastFlag & MASK_ALT))
		return false;
	if (HAS_COMMAND(hotKeyData) ^ GET_BOOL(_lastFlag & MASK_WIN))
		return false;
	if (HAS_SHIFT(hotKeyData) ^ GET_BOOL(_lastFlag & MASK_SHIFT))
		return false;
	if (checkKeyCode) {
		if (GET_SWITCH_KEY(hotKeyData) != _keycode)
			return false;
	}
	return true;
}

void switchLanguage() {
	if (vLanguage == 0)
		vLanguage = 1;
	else
		vLanguage = 0;
	ShowCaretOverlay(vLanguage == 1);
	if (HAS_BEEP(vSwitchKeyStatus)) {
		Beep(vLanguage ? 1000 : 400, 100);
	}
	AppDelegate::getInstance()->onInputMethodChangedFromHotKey();
	if (vUseSmartSwitchKey) {
		setAppInputMethodStatus(OpenKeyHelper::getFrontMostAppExecuteName(), vLanguage | (vCodeTable << 1));
		saveSmartSwitchKeyData();
	}
	startNewSession();
}

static void SendPureCharacter(const Uint32& ch) {
	Uint32 pureChar = ch & ~PURE_CHARACTER_MASK;
	if (pureChar <= 0xFFFF) {
		prepareUnicodeEvent(keyEvent[0], (Uint16)pureChar, true);
		prepareUnicodeEvent(keyEvent[1], (Uint16)pureChar, false);
		SendInput(2, keyEvent, sizeof(INPUT));
		if (IS_DOUBLE_CODE(vCodeTable)) {
			InsertKeyLength(1);
		}
	} else {
		Uint16 high = (Uint16)(((pureChar - 0x10000) >> 10) + 0xD800);
		Uint16 low = (Uint16)(((pureChar - 0x10000) & 0x3FF) + 0xDC00);
		
		prepareUnicodeEvent(keyEvent[0], high, true);
		prepareUnicodeEvent(keyEvent[1], high, false);
		SendInput(2, keyEvent, sizeof(INPUT));
		
		prepareUnicodeEvent(keyEvent[0], low, true);
		prepareUnicodeEvent(keyEvent[1], low, false);
		SendInput(2, keyEvent, sizeof(INPUT));
		if (IS_DOUBLE_CODE(vCodeTable)) {
			InsertKeyLength(2);
		}
	}
}

static void handleMacro() {
	//fix autocomplete
	if (vFixRecommendBrowser) {
		SendEmptyCharacter();
		pData->backspaceCount++;
	}

	//send backspace
	if (pData->backspaceCount > 0) {
		for (int i = 0; i < pData->backspaceCount; i++) {
			SendBackspace();
		}
	}

	size_t newlineCount = 0;
	for (size_t i = 0; i < pData->macroData.size(); i++) {
		Uint32 d = pData->macroData[i];
		Uint32 ch = (d & PURE_CHARACTER_MASK) ? (d & ~PURE_CHARACTER_MASK) : ((d & CHAR_CODE_MASK) ? (d & ~CHAR_CODE_MASK) : d);
		if (ch == '\n' || ch == KEY_ENTER || ch == KEY_RETURN) {
			newlineCount++;
		}
	}

	if (newlineCount < 2) {
		for (size_t i = 0; i < pData->macroData.size(); i++) {
			QueueKeyCode(pData->macroData[i]);
		}
		FlushBatchInput();
	} else {
		// Backup clipboard
		wstring backupText = OpenKeyHelper::getClipboardText(CF_UNICODETEXT);

		// Send real data to clipboard
		SendNewCharString(true);

		// Dynamic wait: Ensure the clipboard content has updated to the macro text before pasting
		wstring macroString = (wchar_t*)_newCharString.data();
		for (int retry = 0; retry < 10; ++retry) {
			wstring currentClip = OpenKeyHelper::getClipboardText(CF_UNICODETEXT);
			if (currentClip == macroString) {
				break;
			}
			Sleep(5);
		}

		// Send shift + insert to paste
		SendCombineKey(KEY_LEFT_SHIFT, VK_INSERT, 0, KEYEVENTF_EXTENDEDKEY);

		// Wait for target application to process the paste message before restoring clipboard
		Sleep(120);

		// Restore clipboard with dynamic retry to handle clipboard locks
		if (!backupText.empty()) {
			int retry = 5;
			while (retry-- > 0) {
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (backupText.length() + 1) * sizeof(WCHAR));
				if (hMem) {
					memcpy(GlobalLock(hMem), backupText.c_str(), (backupText.length() + 1) * sizeof(WCHAR));
					GlobalUnlock(hMem);
					if (OpenClipboard(0)) {
						EmptyClipboard();
						SetClipboardData(CF_UNICODETEXT, hMem);
						CloseClipboard();
						break;
					}
					GlobalFree(hMem);
				}
				Sleep(10);
			}
		}
	}

	if (pData->extCode == 6) {
		SendKeyCode(KEY_SPACE);
	} else if (pData->extCode == 8) {
		SendKeyCode(KEY_RETURN);
	} else if (pData->extCode != 5 && pData->extCode != 7 && _keycode != VK_LSHIFT && _keycode != VK_RSHIFT) {
		SendKeyCode(_keycode | (_flag & MASK_SHIFT ? CAPS_MASK : 0));
	}
}

static bool SetModifierMask(const Uint16& vkCode) {
	// For caps lock case, toggling the flag isn't enough. We need to check the actual state, which should be done before each key press.
	// Example: the caps lock state can be changed without the key being pressed, or the key toggle is made with admin privilege, making the app not able to detect the change.
	if (GetKeyState(VK_CAPITAL) & 0x0001) _flag |= MASK_CAPITAL;
	else _flag &= ~MASK_CAPITAL;

	if (vkCode == VK_LSHIFT || vkCode == VK_RSHIFT) _flag |= MASK_SHIFT;
	else if (vkCode == VK_LCONTROL || vkCode == VK_RCONTROL) _flag |= MASK_CONTROL;
	else if (vkCode == VK_LMENU || vkCode == VK_RMENU) _flag |= MASK_ALT;
	else if (vkCode == VK_LWIN || vkCode == VK_RWIN) _flag |= MASK_WIN;
	else if (vkCode == VK_NUMLOCK) _flag |= MASK_NUMLOCK;
	else if (vkCode == VK_SCROLL) _flag |= MASK_SCROLL;
	else { 
		_isFlagKey = false;
		SyncModifierFlags();
		return false; 
	}
	_isFlagKey = true;
	return true;
}

static bool UnsetModifierMask(const Uint16& vkCode) {
	if (vkCode == VK_LSHIFT || vkCode == VK_RSHIFT) _flag &= ~MASK_SHIFT;
	else if (vkCode == VK_LCONTROL || vkCode == VK_RCONTROL) _flag &= ~MASK_CONTROL;
	else if (vkCode == VK_LMENU || vkCode == VK_RMENU) _flag &= ~MASK_ALT;
	else if (vkCode == VK_LWIN || vkCode == VK_RWIN) _flag &= ~MASK_WIN;
	else if (vkCode == VK_NUMLOCK) _flag &= ~MASK_NUMLOCK;
	else if (vkCode == VK_SCROLL) _flag &= ~MASK_SCROLL;
	else { 
		_isFlagKey = false;
		SyncModifierFlags();
		return false; 
	}
	_isFlagKey = true;
	return true;
}

LRESULT CALLBACK keyboardHookProcess(int nCode, WPARAM wParam, LPARAM lParam) {
	keyboardData = (KBDLLHOOKSTRUCT *)lParam;
	//ignore my event
	if (keyboardData->dwExtraInfo != 0) {
		return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
	}
	
	//ignore if IME pad is open when typing Japanese/Chinese...
	HWND hWnd = GetForegroundWindow();
	HWND hIME = ImmGetDefaultIMEWnd(hWnd);
	if (hIME) {
		LRESULT isImeON = SendMessage(hIME, WM_IME_CONTROL, IMC_GETOPENSTATUS, 0);
		if (isImeON) {
			return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
		}
	}
	
	static Uint16 _lastDoubleModKey = 0;
	static DWORD _lastDoubleModTime = 0;

	//check modifier key
	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
		//LOG(L"Key down: %d\n", keyboardData->vkCode);
		SetModifierMask((Uint16)keyboardData->vkCode);
	} else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
		//LOG(L"Key up: %d\n", keyboardData->vkCode);
		UnsetModifierMask((Uint16)keyboardData->vkCode);
	}
	
	bool isTriggerModifier = false;
	if (vUseMacro && (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)) {
		DWORD now = GetTickCount();
		DWORD doubleClickTime = GetDoubleClickTime();
		if (doubleClickTime < 300) doubleClickTime = 300;
		if (doubleClickTime > 600) doubleClickTime = 600;

		Uint16 vk = (Uint16)keyboardData->vkCode;
		if ((vk == VK_LSHIFT && (vMacroTriggerMask & 0x04)) ||
			(vk == VK_RSHIFT && (vMacroTriggerMask & 0x08))) {
			if (_lastDoubleModKey == vk && (now - _lastDoubleModTime) <= doubleClickTime) {
				isTriggerModifier = true;
				_lastDoubleModKey = 0;
				_lastDoubleModTime = 0;
			} else {
				_lastDoubleModKey = vk;
				_lastDoubleModTime = now;
			}
		} else {
			_lastDoubleModKey = 0;
		}
	} else if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
		Uint16 vk = (Uint16)keyboardData->vkCode;
		if (vk != VK_LSHIFT && vk != VK_RSHIFT) {
			_lastDoubleModKey = 0;
		}
	}

	if (isTriggerModifier) {
		_isFlagKey = false;
		_keycode = (Uint16)keyboardData->vkCode;
	}

	if (!_isFlagKey && wParam != WM_KEYUP && wParam != WM_SYSKEYUP) {
		_keycode = (Uint16)keyboardData->vkCode;
		if (_keycode == VK_DECIMAL) {
			_keycode = KEY_DOT;
		} else if (_keycode == VK_DIVIDE) {
			_keycode = KEY_SLASH;
		} else if (_keycode == VK_SUBTRACT) {
			_keycode = KEY_MINUS;
		}
	}

	//switch language shortcut; convert hotkey
	if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && !_isFlagKey && _keycode != 0) {
		if (_keycode == VK_OEM_5 && (_flag & MASK_ALT)) {
			vBlockBackslash = !vBlockBackslash;
			APP_SET_DATA(vBlockBackslash, vBlockBackslash);
			if (HAS_BEEP(vSwitchKeyStatus)) {
				Beep(vBlockBackslash ? 500 : 750, 80);
				Beep(vBlockBackslash ? 750 : 500, 80);
			}
			if (AppDelegate::getInstance()) {
				AppDelegate::getInstance()->onInputMethodChangedFromHotKey();
			}
			_hasJustUsedHotKey = true;
			_keycode = 0;
			return -1;
		}
		// Cycle Case shortcut: Shift + F3 or Ctrl + Alt + U
		if ((_keycode == VK_F3 && (_flag & MASK_SHIFT) && ((vShortcutModifier & MASK_SHIFT) == 0 || (_flag & vShortcutModifier) != vShortcutModifier)) ||
			((_keycode == 'U' || _keycode == 'u') && (_flag & MASK_CONTROL) && (_flag & MASK_ALT))) {
			OpenKeyHelper::cycleCase();
			_hasJustUsedHotKey = true;
			_keycode = 0;
			return -1;
		}

		// Tab Phím tắt (F1-F12 with customizable modifier)
		if (vShortcutModifier != 0 && (_flag & vShortcutModifier) == vShortcutModifier) {
			if (_keycode == VK_F1 && (vShortcutEnableMask & (1 << 1))) { // F1: Bật TV
				AppDelegate::getInstance()->onSetVietnameseLanguage(1);
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
			if (_keycode == VK_F2 && (vShortcutEnableMask & (1 << 2))) { // F2: Tắt TV
				AppDelegate::getInstance()->onSetVietnameseLanguage(0);
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
			if (_keycode == VK_F3 && (vShortcutEnableMask & (1 << 3))) { // F3: Unicode
				AppDelegate::getInstance()->onTableCode(0);
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
			if (_keycode == VK_F4 && (vShortcutEnableMask & (1 << 4))) { // F4: Custom CodeTable (mặc định VNI Windows)
				AppDelegate::getInstance()->onTableCode(vShortcutF4CodeTable);
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
			if (_keycode == VK_F5 && (vShortcutEnableMask & (1 << 5))) { // F5: Cài đặt
				AppDelegate::getInstance()->createMainDialog();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
			if (_keycode == VK_F8 && (vShortcutEnableMask & (1 << 8))) { // F8: Bảng gõ tắt
				AppDelegate::getInstance()->onMacroTable();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
			if (_keycode == VK_F9 && (vShortcutEnableMask & (1 << 9))) { // F9: Bật/Tắt gõ tắt
				AppDelegate::getInstance()->onToggleUseMacro();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
			if (_keycode == VK_F11 && (vShortcutEnableMask & (1 << 11))) { // F11: Loại trừ app
				AppDelegate::getInstance()->onToggleUseSmartSwitchKey();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
			if (_keycode == VK_F12 && (vShortcutEnableMask & (1 << 12))) { // F12: Reset OpenKey
				AppDelegate::getInstance()->onResetOpenKey();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
		}
		if (GET_SWITCH_KEY(vSwitchKeyStatus) != _keycode && GET_SWITCH_KEY(convertToolHotKey) != _keycode) {
			_lastFlag = 0;
		} else {
			if (GET_SWITCH_KEY(vSwitchKeyStatus) == _keycode && checkHotKey(vSwitchKeyStatus, GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE)) {
				switchLanguage();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
			if (GET_SWITCH_KEY(convertToolHotKey) == _keycode && checkHotKey(convertToolHotKey, GET_SWITCH_KEY(convertToolHotKey) != 0xFE)) {
				AppDelegate::getInstance()->onQuickConvert();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
		}
		_hasJustUsedHotKey = _lastFlag != 0;
	} else if (_isFlagKey) {
		if (_lastFlag == 0 || _lastFlag < _flag)
			_lastFlag = _flag;
		else if (_lastFlag > _flag) {
			//check switch
			if (checkHotKey(vSwitchKeyStatus, GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE)) {
				switchLanguage();
				_hasJustUsedHotKey = true;
			}
			if (checkHotKey(convertToolHotKey, GET_SWITCH_KEY(convertToolHotKey) != 0xFE)) {
				AppDelegate::getInstance()->onQuickConvert();
				_hasJustUsedHotKey = true;
			}
			//check temporarily turn off spell checking
			if (vTempOffSpelling && !_hasJustUsedHotKey && _lastFlag & MASK_CONTROL) {
				vTempOffSpellChecking();
			}
			if (vTempOffOpenKey && !_hasJustUsedHotKey && _lastFlag & MASK_ALT) {
				vTempOffEngine();
			}
			if (!_hasJustUsedHotKey && (_lastFlag & MASK_CONTROL)) {
				vClearMacroKey();
			}
			_lastFlag = _flag;
			_hasJustUsedHotKey = false;
		}
		_keycode = 0;
		return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
	}

	string& curExe = OpenKeyHelper::getLastAppExecuteName();
	bool isGame = isGameApp(curExe);
	bool isGameTyping = isGame ? isGameTypingActive(curExe, _keycode, wParam) : true;
	int effectiveLanguage = (isGame && !isGameTyping) ? 0 : vLanguage;

	//if is in english mode
	if (effectiveLanguage == 0) {
		if (vUseMacro && vUseMacroInEnglishMode && ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) || isTriggerModifier)) {
			vEnglishMode(((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) ? vKeyEventState::KeyDown : (isTriggerModifier ? vKeyEventState::KeyUp : vKeyEventState::MouseDown)),
				_keycode,
				(_flag & MASK_SHIFT) || (_flag & MASK_CAPITAL),
				OTHER_CONTROL_KEY);

			if (pData->code == vReplaceMaro) { //handle macro in english mode
				Uint8 savedExt = pData->extCode;
				handleMacro();
				startNewSession();
				if (isGame && (savedExt == 8 || _keycode == VK_RETURN)) {
					_inGameChatTyping = false;
				}
				if (savedExt == 7) {
					return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
				}
				return NULL;
			}
		}
		return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
	}

	//handle keyboard
	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN || isTriggerModifier) {
		//send event signal to Engine
		vKeyHandleEvent(vKeyEvent::Keyboard,
						isTriggerModifier ? vKeyEventState::KeyUp : vKeyEventState::KeyDown,
						_keycode,
						(_flag & MASK_SHIFT && _flag & MASK_CAPITAL) ? 0 : (_flag & MASK_SHIFT ? 1 : (_flag & MASK_CAPITAL ? 2 : 0)),
						OTHER_CONTROL_KEY);
		if (pData->code == vDoNothing) { //do nothing
			if (IS_DOUBLE_CODE(vCodeTable)) { //VNI
				if (pData->extCode == 1) { //break key
					_syncKey.clear();
				} else if (pData->extCode == 2) { //delete key
					if (_syncKey.size() > 0) {
						if (_syncKey.back() > 1 && (vCodeTable == 2 || vCodeTable == 3)) {
							//send one more backspace
							SendInput(2, backspaceEvent, sizeof(INPUT));
						}
						_syncKey.pop_back();
					}
				} else if (pData->extCode == 3) { //normal key
					InsertKeyLength(1);
				}
			}
			return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
		} else if (pData->code == vWillProcess || pData->code == vRestore || pData->code == vRestoreAndStartNewSession) { //handle result signal
			//fix autocomplete
			if (vFixRecommendBrowser && pData->extCode != 4) {
				if (vFixChromiumBrowser && 
					std::find(_chromiumBrowser.begin(), _chromiumBrowser.end(), OpenKeyHelper::getLastAppExecuteName()) != _chromiumBrowser.end()) {
					SendCombineKey(KEY_LEFT_SHIFT, KEY_LEFT, 0, KEYEVENTF_EXTENDEDKEY);
					SendBackspace();
				} else {
					SendEmptyCharacter();
				}
				pData->backspaceCount++;
			}
			
			//send backspace
			if (pData->backspaceCount > 0 && pData->backspaceCount < MAX_BUFF) {
				for (_i = 0; _i < pData->backspaceCount; _i++) {
					QueueBackspace();
				}
			}

			//send new character
			bool forceClipboard = OpenKeyHelper::getLastAppExecuteName().compare("IDMan.exe") == 0;
			if (!vSendKeyStepByStep || forceClipboard) {
				FlushBatchInput();
				if (forceClipboard)
					SendNewCharStringWithClipboardRestore();
				else
					SendNewCharString();
			} else {
				if (pData->newCharCount > 0 && pData->newCharCount <= MAX_BUFF) {
					for (int i = pData->newCharCount - 1; i >= 0; i--) {
						QueueKeyCode(pData->charData[i]);
					}
				}
				if (pData->code == vRestore || pData->code == vRestoreAndStartNewSession) {
					QueueKeyCode(_keycode | ((_flag & MASK_CAPITAL) || (_flag & MASK_SHIFT) ? CAPS_MASK : 0));
				}
				FlushBatchInput();
				if (pData->code == vRestoreAndStartNewSession) {
					startNewSession();
				}
			}
		} else if (pData->code == vReplaceMaro) { //MACRO
			Uint8 savedExt = pData->extCode;
			handleMacro();
			startNewSession();
			if (isGame && (savedExt == 8 || _keycode == VK_RETURN)) {
				_inGameChatTyping = false;
			}
			if (savedExt == 7 || isTriggerModifier) {
				return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
			}
		}
		return -1; //consume event
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK mouseHookProcess(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0) {
		mouseData = (MSLLHOOKSTRUCT *)lParam;
		switch (wParam) {
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
		case WM_NCXBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
		case WM_NCXBUTTONUP:
			if (!(GetAsyncKeyState(VK_LCONTROL) & 0x8000) && !(GetAsyncKeyState(VK_RCONTROL) & 0x8000)) {
				_flag &= ~MASK_CONTROL;
				_lastFlag &= ~MASK_CONTROL;
				if (GetKeyState(VK_CONTROL) < 0 || GetKeyState(VK_LCONTROL) < 0 || GetKeyState(VK_RCONTROL) < 0) {
					INPUT inputs[2] = {};
					inputs[0].type = INPUT_KEYBOARD;
					inputs[0].ki.wVk = VK_LCONTROL;
					inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
					inputs[0].ki.dwExtraInfo = 1;

					inputs[1].type = INPUT_KEYBOARD;
					inputs[1].ki.wVk = VK_RCONTROL;
					inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
					inputs[1].ki.dwExtraInfo = 1;

					SendInput(2, inputs, sizeof(INPUT));
				}
			}
			if (!(GetAsyncKeyState(VK_LSHIFT) & 0x8000) && !(GetAsyncKeyState(VK_RSHIFT) & 0x8000)) {
				_flag &= ~MASK_SHIFT;
			}
			if (!(GetAsyncKeyState(VK_LMENU) & 0x8000) && !(GetAsyncKeyState(VK_RMENU) & 0x8000)) {
				_flag &= ~MASK_ALT;
			}
			if (!(GetAsyncKeyState(VK_LWIN) & 0x8000) && !(GetAsyncKeyState(VK_RWIN) & 0x8000)) {
				_flag &= ~MASK_WIN;
			}

			//send event signal to Engine
			vKeyHandleEvent(vKeyEvent::Mouse, vKeyEventState::MouseDown, 0);
			if (IS_DOUBLE_CODE(vCodeTable)) { //VNI
				_syncKey.clear();
			}
			break;
		}
	}
	return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

VOID CALLBACK winEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
	// resync modifier flags from actual key state to avoid stale _flag after switching windows/tabs
	SyncModifierFlags();
	_lastFlag = 0;
	if (!(GetAsyncKeyState(VK_LCONTROL) & 0x8000) && !(GetAsyncKeyState(VK_RCONTROL) & 0x8000)) {
		if (GetKeyState(VK_CONTROL) < 0 || GetKeyState(VK_LCONTROL) < 0 || GetKeyState(VK_RCONTROL) < 0) {
			INPUT inputs[2] = {};
			inputs[0].type = INPUT_KEYBOARD;
			inputs[0].ki.wVk = VK_LCONTROL;
			inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
			inputs[0].ki.dwExtraInfo = 1;

			inputs[1].type = INPUT_KEYBOARD;
			inputs[1].ki.wVk = VK_RCONTROL;
			inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
			inputs[1].ki.dwExtraInfo = 1;

			SendInput(2, inputs, sizeof(INPUT));
		}
	}
	//smart switch key
	if (vUseSmartSwitchKey || vRememberCode) {
		string& exe = OpenKeyHelper::getFrontMostAppExecuteName();
		if (exe.compare("explorer.exe") == 0) //dont apply with windows explorer
			return;
		_languageTemp = getAppInputMethodStatus(exe, vLanguage | (vCodeTable << 1));
		vTempOffEngine(false);
		if (vUseSmartSwitchKey) {
			if ((_languageTemp & 0x01) != vLanguage) {
				if (_languageTemp != -1) {
					vLanguage = _languageTemp;
					AppDelegate::getInstance()->onInputMethodChangedFromHotKey();
				} else {
					saveSmartSwitchKeyData();
				}
			}
			ShowCaretOverlay(vLanguage == 1);
		}
		startNewSession();
		if (vRememberCode && (_languageTemp >> 1) != vCodeTable) { //for remember table code feature
			if (_languageTemp != -1) {
				AppDelegate::getInstance()->onTableCode(_languageTemp >> 1);
			} else {
				saveSmartSwitchKeyData();
			}
		}
		if (vSupportMetroApp && exe.compare("ApplicationFrameHost.exe") == 0) {//Metro App
			SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
			SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
		}
	}
}
