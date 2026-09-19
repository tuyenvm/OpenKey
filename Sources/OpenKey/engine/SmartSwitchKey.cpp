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

//main data, i use `map` because it has O(Log(n))
static map<string, Int8> _smartSwitchKeyData;
static string _cacheKey = ""; //use cache for faster
static Int8 _cacheData = 0; //use cache for faster

void initSmartSwitchKey(const Byte* pData, const int& size) {
    _smartSwitchKeyData.clear();
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
        bundleIdSize = pData[cursor++];
        string bundleId((char*)pData + cursor, bundleIdSize);
        cursor += bundleIdSize;
        value = pData[cursor++];
        _smartSwitchKeyData[bundleId] = value;
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

static bool isDefaultEnglishApp(const string& bundleId) {
    static const vector<string> defaultEnApps = {
        "com.apple.Terminal",
        "com.googlecode.iterm2",
        "com.microsoft.VSCode",
        "com.microsoft.VSCodeInsiders",
        "com.sublimetext.3",
        "com.sublimetext.4",
        "com.apple.dt.Xcode",
        "dev.zed.Zed",
        "com.mitchellh.ghostty",
        "io.alacritty",
        "org.alacritty",
        "com.github.wez.wezterm",
        "net.kovidgoyal.kitty"
    };
    for (size_t i = 0; i < defaultEnApps.size(); i++) {
        if (bundleId == defaultEnApps[i] || bundleId.rfind("com.jetbrains.", 0) == 0) {
            return true;
        }
    }
    return false;
}

int getAppInputMethodStatus(const string& bundleId, const int& currentInputMethod) {
    if (bundleId.empty()) return currentInputMethod;
    if (_cacheKey.compare(bundleId) == 0) {
        return _cacheData;
    }
    if (_smartSwitchKeyData.find(bundleId) != _smartSwitchKeyData.end()) {
        _cacheKey = bundleId;
        _cacheData = _smartSwitchKeyData[bundleId];
        return _cacheData;
    }
    _cacheKey = bundleId;
    if (isDefaultEnglishApp(bundleId)) {
        _cacheData = 0; // Default English for developer / terminal apps
    } else {
        _cacheData = currentInputMethod >= 0 ? currentInputMethod : 1; // Default Vietnamese for normal apps
    }
    _smartSwitchKeyData[bundleId] = _cacheData;
    return _cacheData;
}

void setAppInputMethodStatus(const string& bundleId, const int& language) {
    if (bundleId.empty()) return;
    _smartSwitchKeyData[bundleId] = (Int8)language;
    _cacheKey = bundleId;
    _cacheData = (Int8)language;
}
