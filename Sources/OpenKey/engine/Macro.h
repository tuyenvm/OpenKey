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

void initMacroMap(const Byte* pData, const int& size);

bool findMacro(const vector<Uint32>& key, string& macroText, string& macroContent);
#endif /* Macro_h */
