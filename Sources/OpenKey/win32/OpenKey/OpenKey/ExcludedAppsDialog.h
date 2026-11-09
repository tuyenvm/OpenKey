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

class ExcludedAppsDialog : public BaseDialog {
private:
	HWND listApp;
	HWND hComboAppName;
	HWND hCheckSmartSwitch;
	vector<string> appNames;
	vector<int> appStatuses;
public:
	ExcludedAppsDialog(const HINSTANCE& hInstance, const int& resourceId);
	~ExcludedAppsDialog();
	virtual void fillData() override;
protected:
	INT_PTR eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void initDialog();
private:
	void saveAndReload();
	void insertItem(const int& index, LPTSTR appName, LPTSTR statusText);
	void populateRunningApps();
	string getNormalizedInputAppName();

	void onSelectItem(const int& index);
	void onToggleItem(const int& index);
	void onAddAppWithLanguage(const int& lang);
	void onDeleteAppButton();
	void onBrowseAppButton();
	void onAddDefaultAppsButton();
	void onCheckboxClicked(const HWND& hWnd);
};

