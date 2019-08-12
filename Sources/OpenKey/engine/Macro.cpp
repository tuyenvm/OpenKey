//
//  Macro.cpp
//  OpenKey
//
//  Created by Tuyen on 8/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#include "Macro.h"
#include "Vietnamese.h"
#include "Engine.h"
#include <iostream>
#include <locale>
#include <codecvt>

using namespace std;

//main data
map<vector<Uint32>, MacroData> macroMap;

extern int vCodeTable;

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

static wstring utf8ToWideString(const string& str) {
    return converter.from_bytes(str.c_str());
}

static void convert(const string& str, vector<Uint32>& outData) {
    outData.clear();
    wstring data = utf8ToWideString(str);
    Uint32 t = 0;
    int kSign = -1;
    int k = 0;
    for (int i = 0; i < data.size(); i++) {
        t = (Uint32)data[i];
        
        //find normal character fist
        if (_characterMap.find(t) != _characterMap.end()) {
            outData.push_back(_characterMap[t]);
            continue;
        }
        
        //find character which has tone/mark
        for (map<Uint32, vector<Uint16>>::iterator it = _codeTable[0].begin(); it != _codeTable[0].end(); ++it) {
            kSign = -1;
            k = 0;
            for (int j = 0; j < it->second.size(); j++) {
                if ((Uint16)t == it->second[j]) {
                    kSign = 0;
                    outData.push_back(_codeTable[vCodeTable][it->first][k]);
                    break;
                }//end if
                k++;
            }
            if (kSign != -1)
                break;
        }
        if (kSign != -1)
            continue;
        
        //find other character
        outData.push_back(t | PURE_CHARACTER_MASK); //mark it as pure character
    }
}

/**
 * data structure:
 * byte 0 and 1: macro count
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
    Uint8 macroTextSize;
    Uint16 macroContentSize;
    for (int i = 0; i < macroCount; i++) {
        macroTextSize = pData[cursor++];
        string macroText((char*)pData + cursor, macroTextSize);
        cursor += macroTextSize;
        
        memcpy(&macroContentSize, pData + cursor, 2);
        cursor+=2;
        string macroContent((char*)pData + cursor, macroContentSize);
        cursor += macroContentSize;
        
        MacroData data;
        data.macroText = macroText;
        data.macroContent = macroContent;
        
        vector<Uint32> key;
        convert(macroText, key);
        convert(macroContent, data.macroContentCode);
        
        macroMap[key] = data;
    }
}

void getSaveData(vector<Byte>& outData) {
    Uint16 totalMacro = macroMap.size();
    outData.push_back((Byte)totalMacro);
    outData.push_back((Byte)(totalMacro>>8));
    
    for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
        outData.push_back(it->second.macroText.size());
        for (int j = 0; j < it->second.macroText.size(); j++) {
            outData.push_back(it->second.macroText[j]);
        }
        
        Uint16 macroContentSize = it->second.macroContent.size();
        outData.push_back(macroContentSize);
        outData.push_back(macroContentSize>>8);
        for (int j = 0; j < macroContentSize; j++) {
            outData.push_back(it->second.macroContent[j]);
        }
    }
}

bool findMacro(vector<Uint32>& key, vector<Uint32>& macroContentCode) {
    for (int i = 0; i < key.size(); i++) {
        key[i] = getCharacterCode(key[i]);
    }
    if (macroMap.find(key) != macroMap.end()) {
        macroContentCode.clear();
        MacroData data = macroMap[key];
        macroContentCode = data.macroContentCode;
        return true;
    }
    return false;
}

void getAllMacro(vector<vector<Uint32>>& keys, vector<string>& macroTexts, vector<string>& macroContents) {
    keys.clear();
    macroTexts.clear();
    macroContents.clear();
    for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
        keys.push_back(it->first);
        macroTexts.push_back(it->second.macroText);
        macroContents.push_back(it->second.macroContent);
    }
}

bool addMacro(const string& macroText, const string& macroContent) {
    vector<Uint32> key;
    convert(macroText, key);
    if (macroMap.find(key) == macroMap.end()) {
        MacroData data;
        data.macroText = macroText;
        data.macroContent = macroContent;
        convert(macroContent, data.macroContentCode);
        
        macroMap[key] = data;
        return true;
    }
    return false;
}

bool deleteMacro(const string& macroText) {
    vector<Uint32> key;
    convert(macroText, key);
    if (macroMap.find(key) != macroMap.end()) {
        macroMap.erase(key);
        return true;
    }
    return false;
}

void onTableCodeChange() {
    for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
        convert(it->second.macroContent, it->second.macroContentCode);
    }
}
