//
//  SmartSwitchKey.h
//  OpenKey
//
//  Created by Tuyen on 8/13/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#ifndef SmartSwitchKey_h
#define SmartSwitchKey_h

#include "DataType.h"
#include <string>
#include <vector>

using namespace std;

void initSmartSwitchKey(const Byte* pData, const int& size);

/**
 * convert all data to save on disk
 */
void getSmartSwitchKeySaveData(vector<Byte>& outData);

/**
 * find and get language input method, if don't has set @currentInputMethod value for this app
 * return:
 * -1: don't have this bundleId
 * 0: English
 * 1: Vietnamese
 */
int getAppInputMethodStatus(const string& bundleId, const int& currentInputMethod);

/**
 * Set default language for this @bundleId
 */
void setAppInputMethodStatus(const string& bundleId, const int& language);

/**
 * Remove @bundleId from smart switch key map
 */
bool removeAppInputMethodStatus(const string& bundleId);

/**
 * Get all saved apps and their input method status
 */
void getAllSmartSwitchKey(vector<string>& bundleIds, vector<int>& statuses);

/**
 * Add default excluded command-line apps (cmd.exe, powershell.exe, etc.) as English mode (0)
 */
void initDefaultExcludedApps();

#endif /* SmartSwitchKey_h */
