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

static map<Uint32, Uint32> _characterMap = {
    {'a', KEY_A}, {'A', KEY_A|CAPS_MASK},
    {'b', KEY_B}, {'B', KEY_B|CAPS_MASK},
    {'c', KEY_C}, {'C', KEY_C|CAPS_MASK},
    {'d', KEY_D}, {'D', KEY_D|CAPS_MASK},
    {'e', KEY_E}, {'E', KEY_E|CAPS_MASK},
    {'f', KEY_F}, {'F', KEY_F|CAPS_MASK},
    {'g', KEY_G}, {'G', KEY_G|CAPS_MASK},
    {'h', KEY_H}, {'H', KEY_H|CAPS_MASK},
    {'i', KEY_I}, {'I', KEY_I|CAPS_MASK},
    {'j', KEY_J}, {'J', KEY_J|CAPS_MASK},
    {'k', KEY_K}, {'K', KEY_K|CAPS_MASK},
    {'l', KEY_L}, {'L', KEY_L|CAPS_MASK},
    {'m', KEY_M}, {'M', KEY_M|CAPS_MASK},
    {'n', KEY_N}, {'N', KEY_N|CAPS_MASK},
    {'o', KEY_O}, {'O', KEY_O|CAPS_MASK},
    {'p', KEY_P}, {'P', KEY_P|CAPS_MASK},
    {'q', KEY_Q}, {'Q', KEY_Q|CAPS_MASK},
    {'r', KEY_R}, {'R', KEY_R|CAPS_MASK},
    {'s', KEY_S}, {'S', KEY_S|CAPS_MASK},
    {'t', KEY_T}, {'T', KEY_T|CAPS_MASK},
    {'u', KEY_U}, {'U', KEY_U|CAPS_MASK},
    {'v', KEY_V}, {'V', KEY_V|CAPS_MASK},
    {'w', KEY_W}, {'W', KEY_W|CAPS_MASK},
    {'x', KEY_X}, {'X', KEY_X|CAPS_MASK},
    {'y', KEY_Y}, {'Y', KEY_Y|CAPS_MASK},
    {'z', KEY_Z}, {'Z', KEY_Z|CAPS_MASK},
    {'1', KEY_1}, {'!', KEY_1|CAPS_MASK},
    {'2', KEY_2}, {'@', KEY_2|CAPS_MASK},
    {'3', KEY_3}, {'#', KEY_3|CAPS_MASK},
    {'4', KEY_4}, {'$', KEY_4|CAPS_MASK},
    {'5', KEY_5}, {'%', KEY_5|CAPS_MASK},
    {'6', KEY_6}, {'^', KEY_6|CAPS_MASK},
    {'7', KEY_7}, {'&', KEY_7|CAPS_MASK},
    {'8', KEY_8}, {'*', KEY_8|CAPS_MASK},
    {'9', KEY_9}, {'(', KEY_9|CAPS_MASK},
    {'0', KEY_0}, {')', KEY_0|CAPS_MASK},
    {'`', 50}, {'~', 50|CAPS_MASK},
    {'-', 27}, {'_', 27|CAPS_MASK},
    {'=', 24}, {'+', 24|CAPS_MASK},
    {'[', KEY_LEFT_BRACKET}, {'{', KEY_LEFT_BRACKET|CAPS_MASK},
    {']', KEY_RIGHT_BRACKET}, {'}', KEY_RIGHT_BRACKET|CAPS_MASK},
    {'\\', 42}, {'|', 42|CAPS_MASK},
    {';', 41}, {':', 41|CAPS_MASK},
    {'\'', 39}, {'"', 39|CAPS_MASK},
    {',', 43}, {'<', 43|CAPS_MASK},
    {'.', 47}, {'>', 47|CAPS_MASK},
    {'/', 44}, {'?', 44|CAPS_MASK},
    {' ', KEY_SPACE}
};

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
