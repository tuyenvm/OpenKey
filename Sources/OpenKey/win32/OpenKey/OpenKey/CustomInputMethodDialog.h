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
#include "BaseDialog.h"
#include <vector>

class CustomInputMethodDialog : public BaseDialog {
private:
	HWND comboPresets, btnLoadPreset;
	HWND editKey, comboAction, btnAdd, btnReplace, btnDelete, listRules;
private:
	void loadRulesList();
	void saveRules();
	void onAddRule();
	void onReplaceRule();
	void onDeleteRule();
	void onLoadPreset();
protected:
	INT_PTR eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void initDialog();
public:
	CustomInputMethodDialog(const HINSTANCE & hInstance, const int & resourceId);
	~CustomInputMethodDialog();
	virtual void fillData() override;
};
