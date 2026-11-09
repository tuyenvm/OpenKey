//
//  SmartSwitchKey.cpp
//  OpenKey
//
//  Created by Tuyen on 8/13/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#include "SmartSwitchKey.h"
#include <map>
#include <iostream>
#include <memory.h>
#include <algorithm>
#include <cctype>

//main data, i use `map` because it has O(Log(n))
static map<string, Int8> _smartSwitchKeyData;
static string _cacheKey = ""; //use cache for faster
static Int8 _cacheData = 0; //use cache for faster

static string normalizeBundleId(const string& bundleId) {
    string res = bundleId;
#ifdef _WIN32
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });
#endif
    return res;
}

void initSmartSwitchKey(const Byte* pData, const int& size) {
    _smartSwitchKeyData.clear();
    _cacheKey = "";
    _cacheData = 0;
    if (pData == NULL) return;
    Uint16 count = 0;
    Uint32 cursor = 0;
    if (size >= 2) {
        memcpy(&count, pData + cursor, 2);
        cursor+=2;
    }
    Uint8 bundleIdSize;
    Uint8 value;
    for (int i = 0; i < count; i++) {
        if (cursor >= (Uint32)size) break;
        bundleIdSize = pData[cursor++];
        if (cursor + bundleIdSize + 1 > (Uint32)size) break;
        string bundleId((char*)pData + cursor, bundleIdSize);
        cursor += bundleIdSize;
        value = pData[cursor++];
        string key = normalizeBundleId(bundleId);
        if (key == "theopenkeyproject" || key == "unknownprogram" ||
            key == "explorer.exe" || key == "openkey.exe" ||
            key == "openkey64.exe" || key == "openkey32.exe") {
            continue;
        }
        _smartSwitchKeyData[key] = value;
    }
}

void getSmartSwitchKeySaveData(vector<Byte>& outData) {
    outData.clear();
    Uint16 count = (Uint16)_smartSwitchKeyData.size();
    outData.push_back((Byte)count);
    outData.push_back((Byte)(count>>8));
    
    for (std::map<string, Int8>::iterator it = _smartSwitchKeyData.begin(); it != _smartSwitchKeyData.end(); ++it) {
        outData.push_back((Byte)it->first.length());
        for (int j = 0; j < it->first.length(); j++) {
            outData.push_back(it->first[j]);
        }
        outData.push_back(it->second);
    }
}

int getAppInputMethodStatus(const string& bundleId, const int& currentInputMethod) {
    string key = normalizeBundleId(bundleId);
    if (key.empty()) {
        return currentInputMethod;
    }
    if (_cacheKey.compare(key) == 0) {
        return _cacheData;
    }
    if (_smartSwitchKeyData.find(key) != _smartSwitchKeyData.end()) {
        _cacheKey = key;
        _cacheData = _smartSwitchKeyData[key];
        return _cacheData;
    }
    _cacheKey = key;
    _cacheData = currentInputMethod;
    _smartSwitchKeyData[key] = _cacheData;
    return -1;
}

void setAppInputMethodStatus(const string& bundleId, const int& language) {
    string key = normalizeBundleId(bundleId);
    if (key.empty()) return;
    _smartSwitchKeyData[key] = language;
    _cacheKey = key;
    _cacheData = language;
}

bool removeAppInputMethodStatus(const string& bundleId) {
    string key = normalizeBundleId(bundleId);
    auto it = _smartSwitchKeyData.find(key);
    if (it != _smartSwitchKeyData.end()) {
        _smartSwitchKeyData.erase(it);
        if (_cacheKey.compare(key) == 0) {
            _cacheKey = "";
            _cacheData = 0;
        }
        return true;
    }
    return false;
}

void getAllSmartSwitchKey(vector<string>& bundleIds, vector<int>& statuses) {
    bundleIds.clear();
    statuses.clear();
    for (auto it = _smartSwitchKeyData.begin(); it != _smartSwitchKeyData.end(); ++it) {
        bundleIds.push_back(it->first);
        statuses.push_back((int)(Uint8)it->second);
    }
}

void initDefaultExcludedApps() {
    static const vector<string> defaultExcluded = {
        "cmd.exe",
        "powershell.exe",
        "pwsh.exe",
        "windowsterminal.exe",
        "conhost.exe",
        "openconsole.exe"
    };
    for (const auto& app : defaultExcluded) {
        string key = normalizeBundleId(app);
        auto it = _smartSwitchKeyData.find(key);
        if (it != _smartSwitchKeyData.end()) {
            // Keep existing table code bits, set language bit (bit 0) to 0 (English)
            it->second = (Int8)(it->second & ~0x01);
        } else {
            _smartSwitchKeyData[key] = 0;
        }
    }
    _cacheKey = "";
    _cacheData = 0;
}
