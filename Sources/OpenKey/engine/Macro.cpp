//
//  Macro.cpp
//  OpenKey
//
//  Created by Tuyen on 8/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#include "Macro.h"
#include <iostream>

using namespace std;

map<vector<Uint32>, vector<string>> macroMap;

/**
 * data structure:
 * byte 0 and 1: macro count
 *
 * byte 2: key size (keySize)
 * byte keySize + 2: key data
 *
 * byte n: macroText size (macroTextSize)
 * byte n + macroTextSize: macroText data
 *
 * byte m, m+1: macroContentSize
 * byte m+1 + macroContentSize: macroContent data
 *
 * ...
 * next macro
 */
void initMacroMap(const Byte* pData, const int& size) {
    Uint16 macroCount = 0;
    Uint32 cursor = 0;
    if (size >= 2) {
        memcpy(&macroCount, pData + cursor, 2);
        cursor+=2;
    }
    Uint8 keySize, macroTextSize;
    Uint16 macroContentSize;
    for (int i = 0; i < macroCount; i++) {
        keySize = pData[cursor++];
        vector<Uint32> keyData;
        for (int j = 0; j < keySize; j++) {
            keyData.push_back(pData[cursor++]);
        }
        
        macroTextSize = pData[cursor++];
        string macroText((char*)pData + cursor, macroTextSize);
        cursor += macroTextSize;
        
        memcpy(&macroContentSize, pData + cursor, 2);
        cursor+=2;
        string macroContent((char*)pData + cursor, macroContentSize);
        cursor += macroContentSize;
        
        vector<string> data{macroText, macroContent};
        
        macroMap[keyData] = data;
    }
}

bool findMacro(const vector<Uint32>& key, string& macroText, string& macroContent) {
    if (macroMap.find(key) != macroMap.end()) {
        vector<string>& data = macroMap[key];
        macroText = data[0];
        macroContent = data[1];
        return true;
    }
    return false;
}
