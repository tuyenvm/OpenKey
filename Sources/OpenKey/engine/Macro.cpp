//
//  Macro.cpp
//  OpenKey
//
#define _CRT_SECURE_NO_WARNINGS
//  Created by Tuyen on 8/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#include "Macro.h"
#include "Vietnamese.h"
#include "Engine.h"
#include <iostream>
#include <memory.h>
#include <fstream>

using namespace std;

//main data
map<vector<Uint32>, MacroData> macroMap;

extern int vCodeTable;
//local variable
static int c = 0;
static bool _macroFlag = false;
static Uint16 _kChar = 0;
static Uint32 _charBuff;
static int _kMacro;

static void convert(const string& str, vector<Uint32>& outData) {
    outData.clear();
    wstring data = utf8ToWideString(str);
    Uint32 t = 0;
    int kSign = -1;
    int k = 0;
    for (int i = 0; i < data.size(); i++) {
        t = (Uint32)data[i];
        if (t >= 0xD800 && t <= 0xDBFF && i + 1 < data.size()) {
            Uint32 low = data[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                t = 0x10000 + ((t - 0xD800) << 10) + (low - 0xDC00);
                i++;
            }
        }
        
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
                    outData.push_back(_codeTable[vCodeTable][it->first][k] | CHAR_CODE_MASK);
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
    macroMap.clear();
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

void getMacroSaveData(vector<Byte>& outData) {
    Uint16 totalMacro = (Uint16)macroMap.size();
    outData.push_back((Byte)totalMacro);
    outData.push_back((Byte)(totalMacro>>8));
    
    for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
        outData.push_back((Byte)it->second.macroText.size());
        for (int j = 0; j < it->second.macroText.size(); j++) {
            outData.push_back(it->second.macroText[j]);
        }
        
        Uint16 macroContentSize = (Uint16)it->second.macroContent.size();
        outData.push_back((Byte)macroContentSize);
        outData.push_back(macroContentSize>>8);
        for (int j = 0; j < macroContentSize; j++) {
            outData.push_back(it->second.macroContent[j]);
        }
    }
}

static bool modifyCaseUnicode(Uint32& code, const bool& isUpperCase=true) {
    _charBuff = code;
    if (!(code & CHAR_CODE_MASK)) { //for normal char
        code &= isUpperCase ? CAPS_MASK :  ~CAPS_MASK;
        return code != _charBuff;
    }
    
    //for unicode character
    for (map<Uint32, vector<Uint16>>::iterator it = _codeTable[vCodeTable].begin(); it != _codeTable[vCodeTable].end(); ++it) {
        for (_kMacro = 0; _kMacro < it->second.size(); _kMacro++) {
            if ((Uint16)code == it->second[_kMacro]) {
                if (_kMacro % 2 == 0 && !isUpperCase)
                    _kMacro++;
                else if (_kMacro % 2 != 0 && isUpperCase)
                    _kMacro--;
                code = _codeTable[vCodeTable][it->first][_kMacro] | CHAR_CODE_MASK;
                return code != _charBuff;;
            }//end if
        }
    }
    return false;
}

bool findMacro(vector<Uint32>& key, vector<Uint32>& macroContentCode) {
    for (c = 0; c < key.size(); c++) {
        key[c] = getCharacterCode(key[c]);
    }
    if (macroMap.find(key) != macroMap.end()) {
        macroContentCode.clear();
        MacroData data = macroMap[key];
        macroContentCode = data.macroContentCode;
        return true;
    }
    if (vAutoCapsMacro) {
        _macroFlag = false;
        if (key.size() > 1 && modifyCaseUnicode(key[1], false)) {
            _macroFlag = true;
            for (c = 2; c < key.size(); c++) {
                modifyCaseUnicode(key[c], false);
            }
        }
        
        if (key.size() > 0 && modifyCaseUnicode(key[0], false)) {
            if (macroMap.find(key) != macroMap.end()) {
                macroContentCode.clear();
                MacroData data = macroMap[key];
                macroContentCode = data.macroContentCode;
                for (c = 0; c < macroContentCode.size(); c++) {
                    if (c == 0 || _macroFlag) {
                        _kChar = keyCodeToCharacter(macroContentCode[c]);
                        if (_kChar != 0) {
                            _kChar = toupper(_kChar);
                            macroContentCode[c] = _characterMap[_kChar];
                            continue;
                        }
                        if (macroContentCode[c] & CHAR_CODE_MASK) {
                            modifyCaseUnicode(macroContentCode[c]);
                        }
                    }
                }
                return true;
            }
        }
    }
    return false;
}

bool hasMacro(const string& macroName) {
    vector<Uint32> key;
    convert(macroName, key);
    return (macroMap.find(key) != macroMap.end());
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
    if (macroMap.find(key) == macroMap.end()) { //add new macro
        MacroData data;
        data.macroText = macroText;
        data.macroContent = macroContent;
        convert(macroContent, data.macroContentCode);
        macroMap[key] = data;
    } else { //edit this macro
        macroMap[key].macroContent = macroContent;
        convert(macroContent, macroMap[key].macroContentCode);
    }
    return true;
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

void saveToFile(const string& path) {
    wstring wpath = utf8ToWideString(path);
    FILE* f = _wfopen(wpath.c_str(), L"w, ccs=UTF-8");
    if (f) {
        fwprintf(f, L";Compatible OpenKey Macro Data file for UniKey*** version=1 ***\n");
        for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
            wstring name = utf8ToWideString(it->second.macroText);
            wstring content = utf8ToWideString(it->second.macroContent);
            
            // Escape newlines as \n
            size_t start_pos = 0;
            while ((start_pos = content.find(L"\n", start_pos)) != wstring::npos) {
                content.replace(start_pos, 1, L"\\n");
                start_pos += 2;
            }
            // Remove \r just in case
            start_pos = 0;
            while ((start_pos = content.find(L"\r", start_pos)) != wstring::npos) {
                content.erase(start_pos, 1);
            }
            
            fwprintf(f, L"%s||%s\n", name.c_str(), content.c_str());
        }
        fclose(f);
    }
}

void readFromFile(const string& path, const bool& append) {
    wstring wpath = utf8ToWideString(path);
    FILE* f = _wfopen(wpath.c_str(), L"r, ccs=UTF-8");
    if (f) {
        if (!append) {
            macroMap.clear();
        }
        wchar_t line[8192];
        int k = 0;
        while (fgetws(line, 8192, f)) {
            size_t len = wcslen(line);
            while (len > 0 && (line[len-1] == L'\n' || line[len-1] == L'\r')) {
                line[len-1] = L'\0';
                len--;
            }
            k++;
            if (k == 1) continue;
            
            wstring wline(line);
            size_t pos = wline.find(L"||");
            if (pos != wstring::npos) {
                wstring wname = wline.substr(0, pos);
                wstring wcontent = wline.substr(pos + 2);
                
                // Replace escaped \n with real newlines
                size_t start_pos = 0;
                while ((start_pos = wcontent.find(L"\\n", start_pos)) != wstring::npos) {
                    wcontent.replace(start_pos, 2, L"\n");
                    start_pos += 1;
                }
                
                if (!wname.empty()) {
                    addMacro(wideStringToUtf8(wname), wideStringToUtf8(wcontent));
                }
            }
        }
        fclose(f);
    }
}
