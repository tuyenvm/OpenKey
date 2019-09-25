//
//  Vietnamese.h
//  OpenKey
//
//  Created by Tuyen on 1/19/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#ifndef Vietnamese_h
#define Vietnamese_h
#include "DataType.h"
#include <vector>
#include <map>

using namespace std;

extern Uint16 douKey[2][2];

extern map<Uint16, vector<vector<Uint16>>> _vowel;
extern map<Uint16, vector<vector<Uint32>>> _vowelCombine;
extern map<Uint16, vector<vector<Uint16>>> _vowelForMark;
extern vector<vector<Uint16>> _consonantD;
extern vector<vector<Uint16>> _consonantTable;
extern vector<vector<Uint16>> _endConsonantTable;
extern vector<Uint16> _standaloneWbad;
extern vector<vector<Uint16>> _doubleWAllowed;

extern map<Uint32, vector<Uint16>> _codeTable[];
extern Uint16 _unicodeCompoundMark[];

extern map<Uint32, vector<Uint16>> _quickTelex;
extern map<Uint16, vector<Uint16>> _quickStartConsonant;
extern map<Uint16, vector<Uint16>> _quickEndConsonant;
extern map<Uint32, Uint32> _characterMap;

extern Uint16 keyCodeToCharacter(const Uint32& keyCode);
#endif /* Vietnamese_h */
