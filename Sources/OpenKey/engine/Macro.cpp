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
#include <iterator>

using namespace std;

static string toLowercase(string str) {
    for (char &c : str) {
        c = tolower((unsigned char)c);
    }
    return str;
}

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

static string normalizeNewlines(const string& str) {
    string result;
    result.reserve(str.size() + 16);
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] == '\r') {
            if (i + 1 < str.size() && str[i + 1] == '\n') {
                i++;
            }
            result += "\r\n";
        } else if (str[i] == '\n') {
            result += "\r\n";
        } else {
            result += str[i];
        }
    }
    return result;
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
        
        string lowerText = toLowercase(macroText);
        MacroData data;
        data.macroText = lowerText;
        data.macroContent = normalizeNewlines(macroContent);
        
        vector<Uint32> key;
        convert(lowerText, key);
        convert(data.macroContent, data.macroContentCode);
        
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
    string lowerName = toLowercase(macroName);
    vector<Uint32> key;
    convert(lowerName, key);
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
    string lowerText = toLowercase(macroText);
    vector<Uint32> key;
    string normalizedContent = normalizeNewlines(macroContent);
    vector<Uint32> contentCode;
    try {
        convert(lowerText, key);
        convert(normalizedContent, contentCode);
    } catch (...) {
        return false;
    }
    if (macroMap.find(key) == macroMap.end()) { //add new macro
        MacroData data;
        data.macroText = lowerText;
        data.macroContent = normalizedContent;
        data.macroContentCode = contentCode;
        macroMap[key] = data;
    } else { //edit this macro
        macroMap[key].macroContent = normalizedContent;
        macroMap[key].macroContentCode = contentCode;
    }
    return true;
}

bool deleteMacro(const string& macroText) {
    string lowerText = toLowercase(macroText);
    vector<Uint32> key;
    convert(lowerText, key);
    if (macroMap.find(key) != macroMap.end()) {
        macroMap.erase(key);
        return true;
    }
    return false;
}

void clearMacro() {
    macroMap.clear();
}

void onTableCodeChange() {
    for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
        convert(it->second.macroContent, it->second.macroContentCode);
    }
}

void saveToFile(const string& path) {
    ofstream myfile(path.c_str(), ios::out | ios::binary);
    if (myfile.is_open()) {
        // Write UTF-8 BOM
        unsigned char bom[] = {0xEF, 0xBB, 0xBF};
        myfile.write((char*)bom, 3);
        
        string header = ";OpenKey Macro Text Data version=2\n";
        myfile.write(header.c_str(), header.length());
        
        for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
            string name = it->second.macroText;
            string content = it->second.macroContent;
            
            // Normalize CRLF or lone CR to \n first
            size_t start_pos = 0;
            while ((start_pos = content.find("\r\n", start_pos)) != string::npos) {
                content.replace(start_pos, 2, "\n");
                start_pos += 1;
            }
            start_pos = 0;
            while ((start_pos = content.find("\r", start_pos)) != string::npos) {
                content.replace(start_pos, 1, "\n");
                start_pos += 1;
            }
            
            string line = "::" + name + "\n" + content + "\n::end\n\n";
            myfile.write(line.c_str(), line.length());
        }
        myfile.close();
    }
}

static bool readBlockMacroLine(const string& line, string& name) {
    if (line.size() <= 2 || line[0] != ':' || line[1] != ':' || line == "::end") {
        return false;
    }
    name = line.substr(2);
    return !name.empty();
}

static bool looksLikeUtf16LE(const string& data) {
    if (data.size() >= 2 && (unsigned char)data[0] == 0xFF && (unsigned char)data[1] == 0xFE) {
        return true;
    }
    size_t nulCount = 0;
    size_t checkCount = data.size() < 200 ? data.size() : 200;
    for (size_t i = 1; i < checkCount; i += 2) {
        if (data[i] == 0) {
            nulCount++;
        }
    }
    return checkCount > 20 && nulCount > checkCount / 4;
}

static bool looksLikeUtf16BE(const string& data) {
    if (data.size() >= 2 && (unsigned char)data[0] == 0xFE && (unsigned char)data[1] == 0xFF) {
        return true;
    }
    size_t nulCount = 0;
    size_t checkCount = data.size() < 200 ? data.size() : 200;
    for (size_t i = 0; i < checkCount; i += 2) {
        if (data[i] == 0) {
            nulCount++;
        }
    }
    return checkCount > 20 && nulCount > checkCount / 4;
}

static string utf16BytesToUtf8(const string& data, const bool& bigEndian) {
    wstring text;
    size_t start = data.size() >= 2 &&
        ((unsigned char)data[0] == 0xFF && (unsigned char)data[1] == 0xFE ||
         (unsigned char)data[0] == 0xFE && (unsigned char)data[1] == 0xFF) ? 2 : 0;
    for (size_t i = start; i + 1 < data.size(); i += 2) {
        unsigned char a = (unsigned char)data[i];
        unsigned char b = (unsigned char)data[i + 1];
        wchar_t ch = (wchar_t)(bigEndian ? (a << 8 | b) : (b << 8 | a));
        text += ch;
    }
    return wideStringToUtf8(text);
}

static bool isEvKeyMarkerLine(const string& line) {
    return line.find("<<") != string::npos && line.find("Unicode") != string::npos;
}

static string trimEvKeyControlChars(const string& line) {
    size_t start = 0;
    size_t end = line.size();
    while (start < end && ((unsigned char)line[start] <= 0x20 || (unsigned char)line[start] == 0xEF)) {
        start++;
    }
    while (end > start && (unsigned char)line[end - 1] <= 0x20) {
        end--;
    }
    return line.substr(start, end - start);
}

static string readMacroTextFile(const string& path, bool& ok) {
    ok = false;
    ifstream myfile(path.c_str(), ios::in | ios::binary);
    if (!myfile.is_open()) {
        return "";
    }
    string fileData((istreambuf_iterator<char>(myfile)), istreambuf_iterator<char>());
    myfile.close();
    ok = true;

    if (looksLikeUtf16LE(fileData)) {
        return utf16BytesToUtf8(fileData, false);
    }
    if (looksLikeUtf16BE(fileData)) {
        return utf16BytesToUtf8(fileData, true);
    }
    if (fileData.size() >= 3 &&
        (unsigned char)fileData[0] == 0xEF &&
        (unsigned char)fileData[1] == 0xBB &&
        (unsigned char)fileData[2] == 0xBF) {
        fileData.erase(0, 3);
    }
    return fileData;
}

static void splitMacroLines(const string& fileData, vector<string>& lines) {
    lines.clear();
    string line;
    for (size_t i = 0; i <= fileData.size(); i++) {
        if (i < fileData.size() && fileData[i] != '\n') {
            line += fileData[i];
            continue;
        }
        while (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        line = trimEvKeyControlChars(line);
        if (!isEvKeyMarkerLine(line)) {
            lines.push_back(line);
        }
        line.clear();
    }
}

static int importEvKeyLines(const vector<string>& lines) {
    int importedCount = 0;
    for (size_t i = 0; i < lines.size(); i++) {
        string line = lines[i];
        if (line.empty() || line[0] == ';') continue;

        size_t pos = line.find("||");
        if (pos != string::npos) {
            string name = trimEvKeyControlChars(line.substr(0, pos));
            string content = line.substr(pos + 2);

            // Replace escaped \n with real CRLF
            size_t start_pos = 0;
            while ((start_pos = content.find("\\n", start_pos)) != string::npos) {
                content.replace(start_pos, 2, "\r\n");
                start_pos += 2;
            }
            start_pos = 0;
            while ((start_pos = content.find("\xEF\xBF\xBE", start_pos)) != string::npos) {
                content.replace(start_pos, 3, "\r\n");
                start_pos += 2;
            }

            if (!name.empty() && addMacro(name, content)) {
                importedCount++;
            }
        }
    }
    return importedCount;
}

static int readMacroFileCount(const string& path, const bool& append, const bool& evKeyOnly) {
    int importedCount = 0;
    map<vector<Uint32>, MacroData> oldMacroMap;
    bool clearedMacroMap = false;
    bool readOk = false;
    string fileData = readMacroTextFile(path, readOk);
    if (readOk) {
        if (!append) {
            oldMacroMap = macroMap;
            macroMap.clear();
            clearedMacroMap = true;
        }

        vector<string> lines;
        splitMacroLines(fileData, lines);

        bool hasBlockMacro = false;
        if (!evKeyOnly) {
            for (size_t i = 0; i < lines.size(); i++) {
                string blockName;
                if (readBlockMacroLine(lines[i], blockName)) {
                    hasBlockMacro = true;
                    break;
                }
            }
        }

        if (hasBlockMacro) {
            string name;
            string content;
            bool inBlock = false;
            for (size_t i = 0; i < lines.size(); i++) {
                string blockName;
                if (!inBlock && readBlockMacroLine(lines[i], blockName)) {
                    name = blockName;
                    content.clear();
                    inBlock = true;
                    continue;
                }
                if (inBlock && lines[i] == "::end") {
                    if (addMacro(name, content)) {
                        importedCount++;
                    }
                    inBlock = false;
                    continue;
                }
                if (inBlock) {
                    if (!content.empty()) {
                        content += "\n";
                    }
                    content += lines[i];
                }
            }
        } else {
            importedCount = importEvKeyLines(lines);
        }
    }
    if (importedCount == 0 && clearedMacroMap) {
        macroMap = oldMacroMap;
    }
    return importedCount;
}

int readFromFileCount(const string& path, const bool& append) {
    return readMacroFileCount(path, append, false);
}

void readFromFile(const string& path, const bool& append) {
    readFromFileCount(path, append);
}

int readEvKeyFromFileCount(const string& path, const bool& append) {
    return readMacroFileCount(path, append, true);
}
