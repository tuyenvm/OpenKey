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

struct MacroData {
    std::string macroText; //ex: "ms"
    std::string macroContent; //ex: "millisecond"
    std::vector<Uint32> macroContentCode; //converted of macroContent
};

/**
 * Call when you need to load macro data from disk
 */
void initMacroMap(const Byte* pData, const int& size);

/**
 * convert all macro data to save on disk
 */
void getMacroSaveData(std::vector<Byte>& outData);

/**
 * Use to find full text by macro
 */
bool findMacro(std::vector<Uint32>& key, std::vector<Uint32>& macroContentCode);

/**
 * check has this macro or not
 */
bool hasMacro(const std::string& macroName);

/**
 * Get all macro to show on macro table
 */
void getAllMacro(std::vector<std::vector<Uint32>>& keys, std::vector<std::string>& macroTexts, std::vector<std::string>& macroContents);

/**
 * add new macro to memory
 */
bool addMacro(const std::string& macroText, const std::string& macroContent);

/**
 * delete macro from memory
 */
bool deleteMacro(const std::string& macroText);

/**
 * When table code changed, we have to call this function to reload all macroContentCode
 */
void onTableCodeChange();

/**
 * Save all macro data to disk
 */
void saveToFile(const std::string& path);

/**
 * Load macro data from disk
 */
void readFromFile(const std::string& path, const bool& append=true);

#endif /* Macro_h */
