//
//  Macro.h
//  OpenKey
//
//  Created by Tuyen on 8/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#ifndef Macro_h
#define Macro_h

#include <vector>
#include <map>
#include <string>
#include "DataType.h"

using namespace std;

struct MacroData {
    string macroText; //ex: "ms"
    string macroContent; //ex: "millisecond"
    vector<Uint32> macroContentCode; //converted of macroContent
};

/**
 * Call when you need to load macro data from disk
 */
void initMacroMap(const Byte* pData, const int& size);

/**
 * convert all macro data to save on disk
 */
void getMacroSaveData(vector<Byte>& outData);

/**
 * Use to find full text by macro
 */
bool findMacro(vector<Uint32>& key, vector<Uint32>& macroContentCode);

/**
 * check has this macro or not
 */
bool hasMacro(const string& macroName);

/**
 * Get all macro to show on macro table
 */
void getAllMacro(vector<vector<Uint32>>& keys, vector<string>& macroTexts, vector<string>& macroContents);

/**
 * add new macro to memory
 */
bool addMacro(const string& macroText, const string& macroContent);

/**
 * delete macro from memory
 */
bool deleteMacro(const string& macroText);

/**
 * When table code changed, we have to call this function to reload all macroContentCode
 */
void onTableCodeChange();

/**
 * Save all macro data to disk
 */
void saveToFile(const string& path);

/**
 * Load macro data from disk
 */
void readFromFile(const string& path, const bool& append=true);

#endif /* Macro_h */
