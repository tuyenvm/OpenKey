//
//  Engine.cpp
//  OpenKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
#include <iostream>
#include <algorithm>
#include "Engine.h"
#include <string.h>
#include <list>
#include "Macro.h"

#ifndef VK_LSHIFT
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#endif

static vector<Uint8> _charKeyCode = {
    KEY_BACKQUOTE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUALS,
    KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET, KEY_BACK_SLASH,
    KEY_SEMICOLON, KEY_QUOTE, KEY_COMMA, KEY_DOT, KEY_SLASH
};

static vector<Uint8> _breakCode = {
    KEY_ESC, KEY_TAB, KEY_ENTER, KEY_RETURN, KEY_LEFT, KEY_RIGHT, KEY_DOWN, KEY_UP, KEY_COMMA, KEY_DOT,
    KEY_SLASH, KEY_SEMICOLON, KEY_QUOTE, KEY_BACK_SLASH, KEY_MINUS, KEY_EQUALS, KEY_BACKQUOTE, KEY_TAB
#if _WIN32
	, VK_INSERT, VK_HOME, VK_END, VK_DELETE, VK_PRIOR, VK_NEXT, VK_SNAPSHOT, VK_PRINT, VK_SELECT, VK_HELP,
	VK_EXECUTE, VK_NUMLOCK, VK_SCROLL
#endif
};

static vector<Uint8> _macroBreakCode = {
    KEY_RETURN, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_SEMICOLON, KEY_QUOTE, KEY_BACK_SLASH, KEY_MINUS, KEY_EQUALS
};

static Uint16 ProcessingChar[][11] = {
    {KEY_S, KEY_F, KEY_R, KEY_X, KEY_J, KEY_A, KEY_O, KEY_E, KEY_W, KEY_D, KEY_Z}, //Telex
    {KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0}, //VNI
    {KEY_S, KEY_F, KEY_R, KEY_X, KEY_J, KEY_A, KEY_O, KEY_E, KEY_W, KEY_D, KEY_Z}, //Simple Telex 1
    {KEY_S, KEY_F, KEY_R, KEY_X, KEY_J, KEY_A, KEY_O, KEY_E, KEY_W, KEY_D, KEY_Z}, //Simple Telex 2
    {KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_8, KEY_7, KEY_9, KEY_D, KEY_0}, //Tu Binh Tran Don Gian
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}                                              //Tu Dinh Nghia
};

#define IS_KEY_Z(key) (ProcessingChar[vInputType][10] == key || (vInputType == vTuBinhTranDonGian && key == KEY_Z))
#define IS_KEY_D(key) (ProcessingChar[vInputType][9] == key)
#define IS_KEY_W(key) (vInputType == vTuBinhTranDonGian ? (key == KEY_9) : \
                       ((vInputType != vVNI) ? ProcessingChar[vInputType][8] == key : \
                                               (vInputType == vVNI ? (ProcessingChar[vInputType][8] == key || ProcessingChar[vInputType][7] == key) : false)))
#define IS_KEY_DOUBLE(key) (vInputType == vTuBinhTranDonGian ? (key == KEY_6 || key == KEY_7 || key == KEY_8) : \
                            ((vInputType != vVNI) ? (ProcessingChar[vInputType][5] == key || ProcessingChar[vInputType][6] == key || ProcessingChar[vInputType][7] == key) : \
                                                    (vInputType == vVNI ? ProcessingChar[vInputType][6] == key : false)))
#define IS_KEY_S(key) (ProcessingChar[vInputType][0] == key)
#define IS_KEY_F(key) (ProcessingChar[vInputType][1] == key)
#define IS_KEY_R(key) (ProcessingChar[vInputType][2] == key)
#define IS_KEY_X(key) (ProcessingChar[vInputType][3] == key)
#define IS_KEY_J(key) (ProcessingChar[vInputType][4] == key)

#define IS_MARK_KEY(keyCode) ((vInputType == vTuBinhTranDonGian || vInputType == vVNI) ? \
                                (keyCode == KEY_1 || keyCode == KEY_2 || keyCode == KEY_3 || keyCode == KEY_4 || keyCode == KEY_5) : \
                                (keyCode == KEY_S || keyCode == KEY_F || keyCode == KEY_R || keyCode == KEY_J || keyCode == KEY_X))
#define IS_BRACKET_KEY(key) (key == KEY_LEFT_BRACKET || key == KEY_RIGHT_BRACKET)
#define IS_TBT_NUMBER_STANDALONE_KEY(key) (key == KEY_6 || key == KEY_7 || key == KEY_8 || key == KEY_9)
#define IS_TBT_STANDALONE_KEY(key) (IS_TBT_NUMBER_STANDALONE_KEY(key) || IS_BRACKET_KEY(key))
#define IS_TBT_CONSONANT_CONTEXT(key) (isLetterKey(key) && IS_CONSONANT(key))

#define VSI vowelStartIndex
#define VEI vowelEndIndex
#define VWSM vowelWillSetMark
#define hBPC HookState.backspaceCount
#define hNCC HookState.newCharCount
#define hCode HookState.code
#define hExt HookState.extCode
#define hData HookState.charData
#define GET getCharacterCode
#define hMacroKey HookState.macroKey
#define hMacroData HookState.macroData

//Data to sendback to main program
vKeyHookState HookState;
vector<CustomRule> customRules;

//private data
/**
 * data structure of each element in TypingWord (Uint64)
 * first 2 byte is character code or key code.
 * bit 16: has caps or not
 * bit 17: has tone ^ or not
 * bit 18: has tone w or not
 * bit 19 - > 23: has mark or not (Sắc, huyền, hỏi, ngã, nặng)
 * bit 24: is standalone key? (w, [, ])
 * bit 25: is character code or keyboard code; 1: character code; 0: keycode
 */
static Uint32 TypingWord[MAX_BUFF];
static Byte _index = 0;
static vector<Uint32> _longWordHelper; //save the word when _index >= MAX_BUFF
static list<vector<Uint32>> _typingStates; //Aug 28th, 2019: typing helper, save long state of Typing word, can go back and modify the word
vector<Uint32> _typingStatesData;

/**
 * Use for restore key if invalid word
 */
static Uint32 KeyStates[MAX_BUFF];
static Byte _stateIndex = 0;

static bool tempDisableKey = false;
static int capsElem;
static int key;
static int markElem;
static bool isCorect = false;
static bool isChanged = false;
static Byte vowelCount = 0;
static Byte vowelStartIndex = 0;
static Byte vowelEndIndex = 0;
static Byte vowelWillSetMark = 0;
static int i, ii, iii;
static int j;
static int k, kk;
static int l;
static bool isRestoredW;
static Uint16 keyForAEO;
static bool isCheckedGrammar;
static bool _isCaps = false;
static int _spaceCount = 0; //add: July 30th, 2019
static bool _hasHandledMacro = false; //for macro flag August 9th, 2019
static Byte _upperCaseStatus = 0; //for Write upper case for the first letter; 2: will upper case
static bool _isCharKeyCode;
static vector<Uint32> _specialChar;
static bool _useSpellCheckingBefore;
static bool _hasHandleQuickConsonant;
static bool _willTempOffEngine = false;

//function prototype
void findAndCalculateVowel(const bool& forGrammar=false);
void insertMark(const Uint32& markMask, const bool& canModifyFlag=true);
void insertKey(const Uint16& keyCode, const bool& isCaps, const bool& isCheckSpelling=true);
void saveWord();
bool canApplyTBTStandaloneInVowel(const Uint16& data);
void reverseLastStandaloneChar(const Uint32& keyCode, const bool& isCaps);
void insertW(const Uint16& data, const bool& isCaps);
void checkForStandaloneChar(const Uint16& data, const bool& isCaps, const Uint32& keyWillReverse);

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
wstring utf8ToWideString(const string& str) {
    return converter.from_bytes(str.c_str());
}

string wideStringToUtf8(const wstring& str) {
    return converter.to_bytes(str.c_str());
}

void* vKeyInit() {
    _index = 0;
    _stateIndex = 0;
    _useSpellCheckingBefore = vCheckSpelling;
    _typingStatesData.clear();
    _typingStates.clear();
    _longWordHelper.clear();
    return &HookState;
}

bool isWordBreak(const vKeyEvent& event, const vKeyEventState& state, const Uint16& data) {
    if (event == vKeyEvent::Mouse)
        return true;
    for (i = 0; i < _breakCode.size(); i++) {
        if (_breakCode[i] == data) {
            return true;
        }
    }
    return false;
}

bool isMacroBreakCode(const int& data) {
    for (i = 0; i < _macroBreakCode.size(); i++) {
        if (_macroBreakCode[i] == data) {
            return true;
        }
    }
    return false;
}

static bool isMacroTriggerKey(const Uint16& data) {
    if (data == KEY_SPACE && (vMacroTriggerMask & 0x01)) return true;
    if ((data == KEY_RETURN || data == KEY_ENTER) && (vMacroTriggerMask & 0x02)) return true;
    if (data == VK_LSHIFT && (vMacroTriggerMask & 0x04)) return true;
    if (data == VK_RSHIFT && (vMacroTriggerMask & 0x08)) return true;
    if (data == VK_LCONTROL && (vMacroTriggerMask & 0x10)) return true;
    if (data == VK_RCONTROL && (vMacroTriggerMask & 0x20)) return true;
    return false;
}

static bool canAppendCurrentKeyToMacro(const Uint16& data, const bool& isCaps) {
    return data != KEY_SPACE && keyCodeToCharacter(data | (isCaps ? CAPS_MASK : 0)) != 0;
}

static bool findMacroMatch(vector<Uint32> key, vector<Uint32>& macroData, Byte& backspaceCount) {
    if (findMacro(key, macroData)) {
        backspaceCount = (Byte)key.size();
        return true;
    }

    vector<Uint32> collapsedKey;
    for (size_t index = 0; index < key.size(); index++) {
        if (collapsedKey.empty() || collapsedKey.back() != key[index]) {
            collapsedKey.push_back(key[index]);
        }
    }
    if (collapsedKey.size() != key.size() && findMacro(collapsedKey, macroData)) {
        backspaceCount = (Byte)collapsedKey.size();
        return true;
    }
    return false;
}

bool isLetterKey(const Uint16& data) {
    switch (data) {
        case KEY_A: case KEY_B: case KEY_C: case KEY_D: case KEY_E:
        case KEY_F: case KEY_G: case KEY_H: case KEY_I: case KEY_J:
        case KEY_K: case KEY_L: case KEY_M: case KEY_N: case KEY_O:
        case KEY_P: case KEY_Q: case KEY_R: case KEY_S: case KEY_T:
        case KEY_U: case KEY_V: case KEY_W: case KEY_X: case KEY_Y:
        case KEY_Z:
            return true;
        default:
            return false;
    }
}

bool isTBTConsonantContextAt(const int& index) {
    if (index < 0)
        return false;
    Uint16 key = CHR(index);
    if (IS_TBT_CONSONANT_CONTEXT(key))
        return true;
    return index > 0 && ((key == KEY_I && CHR(index - 1) == KEY_G) || (key == KEY_U && CHR(index - 1) == KEY_Q));
}

bool shouldKeepTBTStandaloneKeyRaw(const Uint16& data) {
    if (vInputType != vTuBinhTranDonGian || !IS_TBT_STANDALONE_KEY(data))
        return false;
    if (_specialChar.size() > 0)
        return true;
    return _index > 0 && !isTBTConsonantContextAt(_index - 1) && !canApplyTBTStandaloneInVowel(data);
}

bool isSpellingConsonant(const Uint16& data) {
    if (vInputType == vTuBinhTranDonGian)
        return IS_TBT_CONSONANT_CONTEXT(data);
    return IS_CONSONANT(data);
}

bool getTBTStandaloneRawKey(const Uint32& value, Uint16& rawKey) {
    if (!(value & STANDALONE_MASK))
        return false;

    Uint16 key = (Uint16)(value & CHAR_MASK);
    Uint32 mark = value & (TONE_MASK | TONEW_MASK);
    if (key == KEY_A && mark == TONE_MASK) {
        rawKey = KEY_6;
        return true;
    }
    if (key == KEY_E && mark == TONE_MASK) {
        rawKey = KEY_7;
        return true;
    }
    if (key == KEY_O && mark == TONE_MASK) {
        rawKey = KEY_8;
        return true;
    }
    if (key == KEY_A && mark == TONEW_MASK) {
        rawKey = KEY_9;
        return true;
    }
    if (key == KEY_U && mark == TONEW_MASK) {
        rawKey = KEY_LEFT_BRACKET;
        return true;
    }
    if (key == KEY_O && mark == TONEW_MASK) {
        rawKey = KEY_RIGHT_BRACKET;
        return true;
    }
    return false;
}

bool getTBTStandaloneValue(const Uint16& data, Uint32& value) {
    switch (data) {
        case KEY_6:
            value = KEY_A | TONE_MASK;
            return true;
        case KEY_7:
            value = KEY_E | TONE_MASK;
            return true;
        case KEY_8:
            value = KEY_O | TONE_MASK;
            return true;
        case KEY_9:
            value = KEY_A | TONEW_MASK;
            return true;
        case KEY_LEFT_BRACKET:
            value = KEY_U | TONEW_MASK;
            return true;
        case KEY_RIGHT_BRACKET:
            value = KEY_O | TONEW_MASK;
            return true;
        default:
            return false;
    }
}

bool canApplyTBTStandaloneInVowel(const Uint16& data) {
    Uint32 target;
    if (!getTBTStandaloneValue(data, target) || _index == 0)
        return false;

    vector<Uint32> seq;
    for (int pos = _index - 1; pos >= 0; pos--) {
        Uint16 key = CHR(pos);
        if (!isLetterKey(key) || isSpellingConsonant(key))
            break;
        seq.insert(seq.begin(), key | (TypingWord[pos] & (TONE_MASK | TONEW_MASK)));
    }
    seq.push_back(target);
    if (seq.size() < 2)
        return false;

    vector<vector<Uint32>>& vowelSet = _vowelCombine[(Uint16)(seq[0] & CHAR_MASK)];
    for (const auto& vowel : vowelSet) {
        if (seq.size() > vowel.size() - 1)
            continue;
        bool matches = true;
        for (size_t pos = 0; pos < seq.size(); pos++) {
            if (vowel[pos + 1] != seq[pos]) {
                matches = false;
                break;
            }
        }
        if (matches)
            return true;
    }
    return false;
}

bool canTBTStandaloneStartVowel(const Uint32& value) {
    if (!(value & STANDALONE_MASK))
        return false;
    Uint32 key = (value & CHAR_MASK) | (value & (TONE_MASK | TONEW_MASK));
    for (const auto& vowelEntry : _vowelCombine) {
        for (const auto& vowel : vowelEntry.second) {
            if (vowel.size() > 2 && vowel[1] == key)
                return true;
        }
    }
    return false;
}

void insertCustomW(const Uint16& data, const bool& isCaps, const bool& allowStandaloneAtStart) {
    findAndCalculateVowel();
    if (vowelCount > 0) {
        insertW(KEY_W, isCaps);
    } else if (_index == 0 && allowStandaloneAtStart) {
        insertKey(data, isCaps, false);
        reverseLastStandaloneChar(KEY_U, isCaps);
    } else if (_index > 0) {
        checkForStandaloneChar(data, isCaps, KEY_U);
    } else {
        insertKey(data, isCaps);
    }
}

bool restoreTBTStandaloneThenInsertRaw(const Uint16& data, const bool& isCaps) {
    Uint16 previousRawKey;
    if (_index == 0 || !getTBTStandaloneRawKey(TypingWord[_index - 1], previousRawKey))
        return false;

    hCode = vWillProcess;
    hBPC = 1;
    hExt = 3;
    TypingWord[_index - 1] = previousRawKey | (TypingWord[_index - 1] & CAPS_MASK);
    if (previousRawKey == data) {
        hNCC = 1;
        hData[0] = TypingWord[_index - 1];
        saveWord();
        return true;
    }

    hNCC = 2;
    hData[1] = TypingWord[_index - 1];
    insertKey(data, isCaps);
    hData[0] = TypingWord[_index - 1];
    saveWord();
    return true;
}

void setKeyData(const Byte& index, const Uint16& keyCode, const bool& isCaps) {
    if (index < 0 || index >= MAX_BUFF)
        return;
    TypingWord[index] = keyCode | (isCaps ? CAPS_MASK : 0);
}

bool _spellingOK = false;
bool _spellingFlag = false;
bool _spellingVowelOK = false;
Byte _spellingEndIndex = 0;

void checkSpelling(const bool& forceCheckVowel=false) {
    _spellingOK = false;
    _spellingVowelOK = true;
    _spellingEndIndex = _index;
    
    if (_index > 0 && CHR(_index-1) == KEY_RIGHT_BRACKET) {
        _spellingEndIndex = _index-1;
    }
    
    if (_spellingEndIndex > 0) {
        j = 0;
        vector<vector<Uint16>>& consonantTable = (vInputType == vTuBinhTranDonGian) ? _consonantTableTBT : _consonantTable;
        //Check first consonant
        if (isSpellingConsonant(CHR(0))) {
            for (i = 0; i < consonantTable.size(); i++) {
                _spellingFlag = false;
                if (_spellingEndIndex < consonantTable[i].size())
                    _spellingFlag = true;
                for (j = 0; j < consonantTable[i].size(); j++) {
                    if (_spellingEndIndex > j &&
                        (consonantTable[i][j] & ~(vQuickStartConsonant ? END_CONSONANT_MASK : 0)) != CHR(j) &&
                        (consonantTable[i][j] & ~(vAllowConsonantZFWJ ? CONSONANT_ALLOW_MASK : 0)) != CHR(j)) {
                        _spellingFlag = true;
                        break;
                    }
                }
                if (_spellingFlag)
                    continue;
                
                break;
            }
        }
        
        if (j == _spellingEndIndex){ //for "d" case
            _spellingOK = true;
        }
        
        //check next vowel
        k = j;
        VSI = k;
        //August 23rd, 2019: fix case "que't"
        if (CHR(VSI) == KEY_U && k > 0 && k < _spellingEndIndex-1 && CHR(VSI-1) == KEY_Q) {
            k = k + 1;
            j = k;
            VSI = k;
        } else if (_index >= 2 && CHR(0) == KEY_G && CHR(1) == KEY_I && isSpellingConsonant(CHR(2))) {
            VSI = k = j = 1; //Sep 28th: fix gìn
        }
        for (l = 0; l < 3; l++) {
            if (k < _spellingEndIndex && !isSpellingConsonant(CHR(k))) {
                k++;
                VEI = k;
            }
        }
        if (k > j) { //has vowel,
            _spellingVowelOK = false;
            //check correct combined vowel
            if (k - j > 1 && forceCheckVowel) {
                vector<vector<Uint32>>& vowelSet = _vowelCombine[CHR(j)];
                for (l = 0; l < vowelSet.size(); l++) {
                    _spellingFlag = false;
                    for (ii = 1; ii < vowelSet[l].size(); ii++) {
                        if (j + ii - 1 < _spellingEndIndex && vowelSet[l][ii] != ((CHR(j + ii - 1) | (TypingWord[j + ii - 1] & TONEW_MASK) | (TypingWord[j + ii - 1] & TONE_MASK)))) {
                            _spellingFlag = true;
                            break;
                        }
                    }
                    if (_spellingFlag || (k < _spellingEndIndex && !vowelSet[l][0]) || (j + ii - 1 < _spellingEndIndex && !isSpellingConsonant(CHR(j + ii - 1))))
                        continue;
                    
                    _spellingVowelOK = true;
                    break;
                }
            } else if (!isSpellingConsonant(CHR(j))) {
                _spellingVowelOK = true;
            }
            
            //continue check last consonant
            for (ii = 0; ii < _endConsonantTable.size(); ii++) {
                _spellingFlag = false;
   
                for (j = 0; j < _endConsonantTable[ii].size(); j++) {
                    if (_spellingEndIndex > k+j &&
                        (_endConsonantTable[ii][j] & ~(vQuickEndConsonant ? END_CONSONANT_MASK : 0)) != CHR(k + j)) {
                        _spellingFlag = true;
                        break;
                    }
                }
                if (_spellingFlag)
                    continue;
                
                if (k + j >= _spellingEndIndex) {
                    _spellingOK = true;
                    break;
                }
            }
            
            //limit: end consonant "ch", "t" can not use with "~", "`", "?"
            if (_spellingOK) {
                if (_index >= 3 && CHR(_index-1) == KEY_H && CHR(_index-2) == KEY_C && !((TypingWord[_index-3] & MARK1_MASK) || (TypingWord[_index-3] & MARK5_MASK) || !(TypingWord[_index-3] & MARK_MASK))) {
                    _spellingOK = false;
                } else if (_index >= 2 && CHR(_index-1) == KEY_T && !((TypingWord[_index-2] & MARK1_MASK) || (TypingWord[_index-2] & MARK5_MASK) || !(TypingWord[_index-2] & MARK_MASK))) {
                    _spellingOK = false;
                }
            }
        }
    } else {
        _spellingOK = true;
    }
    tempDisableKey = !(_spellingOK && _spellingVowelOK);
    
    //cout<<"spelling vowel: "<<(_spellingVowelOK ? "OK": "Err")<<endl;
    //cout<<"spelling: "<<(_spellingOK ? "OK": "Err")<<endl<<endl;
}

void checkGrammar(const int& deltaBackSpace) {
    if (_index <= 1 || _index >= MAX_BUFF)
        return;
    
    findAndCalculateVowel(true);
    if (vowelCount == 0)
        return;
    
    isCheckedGrammar = false;
    
    l = VSI;
    
    //if N key for case: "thuơn", "ưoi", "ưom", "ưoc"
    if (_index >= 3) {
        for (i = _index-1; i >= 0; i--) {
            if (CHR(i) == KEY_N || CHR(i) == KEY_C || CHR(i) == KEY_I ||
                CHR(i) == KEY_M || CHR(i) == KEY_P || CHR(i) == KEY_T) {
                if (i - 2 >= 0 && CHR(i - 1) == KEY_O && CHR(i - 2) == KEY_U) {
                    if ((TypingWord[i-1] & TONEW_MASK) ^ (TypingWord[i-2] & TONEW_MASK)) {
                        TypingWord[i - 2] |= TONEW_MASK;
                        TypingWord[i - 1] |= TONEW_MASK;
                        isCheckedGrammar = true;
                        break;
                    }
                }
            }
        }
    }
    
    //check mark
    if (_index >= 2) {
        for (i = l; i <= VEI; i++) {
            if (TypingWord[i] & MARK_MASK) {
                Uint32 mark = TypingWord[i] & MARK_MASK;
                TypingWord[i] &= ~MARK_MASK;
                insertMark(mark, false);
                if (i != vowelWillSetMark)
                    isCheckedGrammar = true;
                break;
            }
        }
    }
    
    //re-arrange data to sendback
    if (isCheckedGrammar) {
        if (hCode ==vDoNothing)
            hCode = vWillProcess;
        hBPC = 0;
        
        for (i = _index - 1; i >= l; i--) {
            hBPC++;
            hData[_index - 1 - i] = GET(TypingWord[i]);
        }
        hNCC = hBPC;
        hBPC += deltaBackSpace;
        hExt = 4;
    }
}

void insertKey(const Uint16& keyCode, const bool& isCaps, const bool& isCheckSpelling) {
    if (_index >= MAX_BUFF) {
        _longWordHelper.push_back(TypingWord[0]); //save long word
        //left shift
        for (iii = 0; iii < MAX_BUFF - 1; iii++) {
            TypingWord[iii] = TypingWord[iii + 1];
        }
        setKeyData(_index-1, keyCode, isCaps);
    } else {
        setKeyData(_index++, keyCode, isCaps);
    }
    
    if (vCheckSpelling && isCheckSpelling)
        checkSpelling();
    
    //allow d after consonant
    if (keyCode == KEY_D && _index - 2 >= 0 && IS_CONSONANT(CHR(_index - 2)))
        tempDisableKey = false;
}

void insertState(const Uint16& keyCode, const bool& isCaps) {
    if (_stateIndex >= MAX_BUFF) {
        //left shift
        for (iii = 0; iii < MAX_BUFF - 1; iii++) {
            KeyStates[iii] = KeyStates[iii + 1];
        }
        KeyStates[_stateIndex-1] = keyCode | (isCaps ? CAPS_MASK : 0);
    } else {
        KeyStates[_stateIndex++] = keyCode | (isCaps ? CAPS_MASK : 0);
    }
}

void saveWord() {
    //save word history
    if (hCode != vReplaceMaro) {
        if (_index > 0) {
            if (_longWordHelper.size() > 0) { //save long word first
                _typingStatesData.clear();
                for (i = 0; i < _longWordHelper.size(); i++) {
                    if (i != 0 && i % MAX_BUFF == 0) { //save if overflow
                        _typingStates.push_back(_typingStatesData);
                        _typingStatesData.clear();
                    }
                    _typingStatesData.push_back(_longWordHelper[i]);
                }
                _typingStates.push_back(_typingStatesData);
                _longWordHelper.clear();
            }
            
            //save current word
            _typingStatesData.clear();
            for (i = 0; i < _index; i++) {
                _typingStatesData.push_back(TypingWord[i]);
            }
            _typingStates.push_back(_typingStatesData);
        }
    } else { //save macro words
        _typingStatesData.clear();
        for (i = 0; i < hMacroData.size(); i++) {
            if (i != 0 && i % MAX_BUFF == 0) { //break if overflow
                _typingStates.push_back(_typingStatesData);
                _typingStatesData.clear();
            }
            _typingStatesData.push_back(hMacroData[i]);
        }
        _typingStates.push_back(_typingStatesData);
    }
}

void saveWord(const Uint32& keyCode, const int& count) {
    _typingStatesData.clear();
    for (i = 0; i < count; i++) {
        _typingStatesData.push_back(keyCode);
    }
    _typingStates.push_back(_typingStatesData);
}

void saveSpecialChar() {
    _typingStatesData.clear();
    for (i = 0; i < _specialChar.size(); i++) {
        _typingStatesData.push_back(_specialChar[i]);
    }
    _typingStates.push_back(_typingStatesData);
    _specialChar.clear();
}

void restoreLastTypingState() {
    if (_typingStates.size() > 0) {
        _typingStatesData = _typingStates.back();
        _typingStates.pop_back();
        if (_typingStatesData.size() > 0){
            if (_typingStatesData[0] == KEY_SPACE) {
                _spaceCount = (int)_typingStatesData.size();
                _index = 0;
            } else if (std::find(_charKeyCode.begin(), _charKeyCode.end(), (Uint16)_typingStatesData[0]) != _charKeyCode.end()) {
                _index = 0;
                _specialChar = _typingStatesData;
                checkSpelling();
            } else {
                for (i = 0; i < _typingStatesData.size(); i++) {
                    TypingWord[i] = _typingStatesData[i];
                }
                _index = (Byte)_typingStatesData.size();
            }
        }
    }
}

void startNewSession() {
    _index = 0;
    hBPC = 0;
    hNCC = 0;
    tempDisableKey = false;
    _stateIndex = 0;
    _hasHandledMacro = false;
    _hasHandleQuickConsonant = false;
    _longWordHelper.clear();
}

void checkCorrectVowel(vector<vector<Uint16>>& charset, int& i, int& k, const Uint16& markKey) {
    //ignore "qu" case
    if (_index >= 2 && CHR(_index-1) == KEY_U && CHR(_index-2) == KEY_Q) {
        isCorect = false;
        return;
    }
    k = _index - 1;
    for (j = (int)charset[i].size() - 1; j >= 0; j--) {
        if ((charset[i][j] & ~(vQuickEndConsonant ? END_CONSONANT_MASK : 0)) != CHR(k)) {
            isCorect = false;
            return;
        }
        k--;
        if (k < 0)
            break;
    }
    
    //limit mark for end consonant: "C", "T"
    if (isCorect && charset[i].size() > 1 && (IS_KEY_F(markKey) || IS_KEY_X(markKey) || IS_KEY_R(markKey))) {
        if (charset[i][1] == KEY_C || charset[i][1] == KEY_T) {
            isCorect = false;
        } else if (charset[i].size() > 2 && (charset[i][2] == KEY_T)) {
            isCorect = false;
        }
    }
    
    if (isCorect && k >= 0) {
        if (CHR(k) == CHR(k+1)) {
            isCorect = false;
        }
    }
}

Uint32 getCharacterCode(const Uint32& data) {
    capsElem = (data & CAPS_MASK) ? 0 : 1;
    key = data & CHAR_MASK;
    if (data & MARK_MASK) { //has mark
        markElem = -2;
        switch (data & MARK_MASK) {
            case MARK1_MASK:
                markElem = 0;
                break;
            case MARK2_MASK:
                markElem = 2;
                break;
            case MARK3_MASK:
                markElem = 4;
                break;
            case MARK4_MASK:
                markElem = 6;
                break;
            case MARK5_MASK:
                markElem = 8;
                break;
        }
        markElem += capsElem;
        
        switch (key) {
            case KEY_A:
            case KEY_O:
            case KEY_U:
            case KEY_E:
                if ((data & TONE_MASK) == 0 && (data & TONEW_MASK) == 0)
                    markElem += 4;
                break;
        }
        
        if (data & TONE_MASK) {
            key |= TONE_MASK;
        } else if (data & TONEW_MASK) {
            key |= TONEW_MASK;
        }
        if (_codeTable[vCodeTable].find(key) == _codeTable[vCodeTable].end())
            return data; //not found
        
        return _codeTable[vCodeTable][key][markElem] | CHAR_CODE_MASK;
    } else { //doesn't has mark
        if (_codeTable[vCodeTable].find(key) == _codeTable[vCodeTable].end())
            return data; //not found
        
        if (data & TONE_MASK) {
            return _codeTable[vCodeTable][key][capsElem] | CHAR_CODE_MASK;
        } else if (data & TONEW_MASK) {
            return _codeTable[vCodeTable][key][capsElem + 2] | CHAR_CODE_MASK;
        } else {
            return data; //not found
        }
    }
    
    return 0;
}

void findAndCalculateVowel(const bool& forGrammar) {
    vowelCount = 0;
    VSI = VEI = 0;
    for (iii = _index - 1; iii >= 0; iii--) {
        if (IS_CONSONANT(CHR(iii))) {
            if (vowelCount > 0)
                break;
        } else {  //is vowel
            if (vowelCount == 0)
                VEI = iii;
            if (!forGrammar) {
                if ((iii-1 >= 0 && (CHR(iii) == KEY_I && CHR(iii-1) == KEY_G)) ||
                    (iii-1 >= 0 && (CHR(iii) == KEY_U && CHR(iii-1) == KEY_Q))) {
                    break;
                }
            }
            VSI = iii;
            vowelCount++;
        }
    }
    //August 26th, 2019: don't count "u" at "q u" as a vowel
    if (VSI - 1 >= 0 && CHR(VSI) == KEY_U && CHR(VSI-1) == KEY_Q) {
        VSI++;
        vowelCount--;
    }
}

void removeMark() {
    findAndCalculateVowel(true);
    isChanged = false;
    if (_index > 0) {
        for (i = VSI; i <= VEI; i++) {
            if (TypingWord[i] & MARK_MASK) {
                TypingWord[i] &= ~MARK_MASK;
                isChanged = true;
            }
        }
    }
    if (isChanged) {
        hCode = vWillProcess;
        hBPC = 0;
        
        for (i = _index - 1; i >= VSI; i--) {
            hBPC++;
            hData[_index - 1 - i] = GET(TypingWord[i]);
        }
        hNCC = hBPC;
    } else {
        hCode = vDoNothing;
    }
}

bool canHasEndConsonant() {
    vector<vector<Uint32>>& vo = _vowelCombine[CHR(VSI)];
    for (ii = 0; ii < vo.size(); ii++) {
        kk = VSI;
        for (iii = 1; iii < vo[ii].size(); iii++) {
            if (kk > VEI || ((CHR(kk) | (TypingWord[kk] & TONE_MASK) | (TypingWord[kk] & TONEW_MASK)) != vo[ii][iii])) {
                break;
            }
            kk++;
        }
        if (iii >= vo[ii].size()) {
            return vo[ii][0] == 1;
        }
    }
    return false;
}

void handleModernMark() {
    //default
    VWSM = VEI;
    hBPC = (_index - VEI);
    
    //rule 2
    if (vowelCount == 3 && ((CHR(VSI) == KEY_O && CHR(VSI+1) == KEY_A && CHR(VSI+2) == KEY_I) ||
                            (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_Y && CHR(VSI+2) == KEY_U) ||
                            (CHR(VSI) == KEY_O && CHR(VSI+1) == KEY_E && CHR(VSI+2) == KEY_O) ||
                            (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_Y && CHR(VSI+2) == KEY_A))) {
        VWSM = VSI + 1;
        hBPC = _index - VWSM;
    } else if ((CHR(VSI) == KEY_O && CHR(VSI+1) == KEY_I) ||
               (CHR(VSI) == KEY_A && CHR(VSI+1) == KEY_I) ||
               (CHR(VSI)== KEY_U && CHR(VSI+1) == KEY_I) ) {
        
        VWSM = VSI;
        hBPC = _index - VWSM;
    } else if (CHR(VEI-1) == KEY_A && CHR(VEI) == KEY_Y) {
        VWSM = VEI - 1;
        hBPC = (_index - VEI) + 1;
    } else if (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_O) {
        VWSM = VSI + 1;
        hBPC = _index - VWSM;
    } else if (CHR(VSI+1) == KEY_O || CHR(VSI+1) == KEY_U) {
        VWSM = VEI - 1;
        hBPC = (_index - VEI) + 1;
    } else if (CHR(VSI) == KEY_O || CHR(VSI) == KEY_U) {
        VWSM = VEI;
        hBPC = (_index - VEI);
    }
    
    //rule 3.1
    if ((CHR(VSI) == KEY_I && (TypingWord[VSI+1] & (KEY_E | TONE_MASK))) ||
        (CHR(VSI) == KEY_Y && (TypingWord[VSI+1] & (KEY_E | TONE_MASK))) ||
        (CHR(VSI) == KEY_U && (TypingWord[VSI+1] == (KEY_O | TONE_MASK))) ||
        ((TypingWord[VSI] == (KEY_U | TONEW_MASK)) && (TypingWord[VSI+1] == (KEY_O | TONEW_MASK)))){
        
        if (VSI+2 < _index) {
            if (CHR(VSI+2) == KEY_P || CHR(VSI+2) == KEY_T ||
                CHR(VSI+2) == KEY_M || CHR(VSI+2) == KEY_N ||
                CHR(VSI+2) == KEY_O || CHR(VSI+2) == KEY_U ||
                CHR(VSI+2) == KEY_I || CHR(VSI+2) == KEY_C ||
                (VSI+3 < _index && CHR(VSI+2) == KEY_C && CHR(VSI+2) == KEY_H) ||
                (VSI+3 < _index && CHR(VSI+2) == KEY_N && CHR(VSI+2) == KEY_H) ||
                (VSI+3 < _index && CHR(VSI+2) == KEY_N && CHR(VSI+2) == KEY_G)) {
                
                VWSM = VSI + 1;
                hBPC = _index - VWSM;
            } else {
                VWSM = VSI;
                hBPC = _index - VWSM;
            }
        } else {
            VWSM = VSI;
            hBPC = _index - VWSM;
        }
    }
    //rule 3.2
    else if ((CHR(VSI) == KEY_I && (CHR(VSI) == KEY_A)) ||
             (CHR(VSI) == KEY_Y && (CHR(VSI) == KEY_A)) ||
             (CHR(VSI) == KEY_U && (CHR(VSI) == KEY_A)) ||
             (CHR(VSI) == KEY_U && (TypingWord[VSI+1] == (KEY_U | TONEW_MASK)))){
        
        VWSM = VSI;
        hBPC = _index - VWSM;
    }
    
    //rule 4
    if (vowelCount == 2) {
        if (((CHR(VSI) == KEY_I) && (CHR(VSI+1) == KEY_A)) ||
            ((CHR(VSI) == KEY_I) && (CHR(VSI+1) == KEY_U)) ||
            ((CHR(VSI) == KEY_I) && (CHR(VSI+1) == KEY_O))) {
            
            if (VSI == 0 || (CHR(VSI-1) != KEY_G)) { //dont have G
                VWSM = VSI;
                hBPC = _index - VWSM;
            } else {
                VWSM = VSI + 1;
                hBPC = _index - VWSM;
            }
        } else if ((CHR(VSI) == KEY_U) && (CHR(VSI+1) == KEY_A)) {
            if (VSI == 0 || (CHR(VSI-1) != KEY_Q)) { //dont have Q
                if (VEI + 1 >= _index || !canHasEndConsonant()) {
                    VWSM = VSI;
                    hBPC = _index - VWSM;
                }
            } else {
                VWSM = VSI + 1;
                hBPC = _index - VWSM;
            }
        } else if ((CHR(VSI) == KEY_O) && (CHR(VSI+1) == KEY_O)) { //thoong
            VWSM = VEI;
            hBPC = _index - VWSM;
        }
    }
}

void handleOldMark() {
    //default
    if (vowelCount == 0 && CHR(VEI) == KEY_I)
        VWSM = VEI;
    else
        VWSM = VSI;
    hBPC = (_index - VWSM);
    
    //rule 2
    if (vowelCount == 3 || (VEI + 1 < _index && IS_CONSONANT(CHR(VEI + 1)) && canHasEndConsonant())) {
        VWSM = VSI + 1;
        hBPC = _index - VWSM;
    }
    
    //rule 3
    for (ii = VSI; ii <= VEI; ii++) {
        if ((CHR(ii) == KEY_E && TypingWord[ii] & TONE_MASK) || (CHR(ii) == KEY_O && TypingWord[ii] & TONEW_MASK)) {
            VWSM = ii;
            hBPC = _index - VWSM;
            break;
        }
    }
    
    hNCC = hBPC;
}

void insertMark(const Uint32& markMask, const bool& canModifyFlag) {
    vowelCount = 0;
    
    if (canModifyFlag)
        hCode = vWillProcess;
    hBPC = hNCC = 0;
    
    findAndCalculateVowel();
    VWSM = 0;
    
    //detect mark position
    if (vowelCount == 1) {
        VWSM = VEI;
        hBPC = (_index - VEI);
    } else { //vowel = 2 or 3
        if (vUseModernOrthography == 0)
            handleOldMark();
        else
            handleModernMark();
        if (TypingWord[VEI] & TONE_MASK || TypingWord[VEI] & TONEW_MASK)
            vowelWillSetMark = VEI;
    }
    
    //send data
    kk = _index - 1 - VSI;
    //if duplicate same mark -> restore
    if (TypingWord[VWSM] & markMask) {
        
        TypingWord[VWSM] &= ~MARK_MASK;
        if (canModifyFlag)
            hCode = vRestore;
        for (ii = VSI; ii < _index; ii++) {
            TypingWord[ii] &= ~MARK_MASK;
            hData[kk--] = GET(TypingWord[ii]);
        }
        //_index = 0;
        tempDisableKey = true;
    } else {
        //remove other mark
        TypingWord[VWSM] &= ~MARK_MASK;
        
        //add mark
        TypingWord[VWSM] |= markMask;
        for (ii = VSI; ii < _index; ii++) {
            if (ii != VWSM) { //remove mark for other vowel
                TypingWord[ii] &= ~MARK_MASK;
            }
            hData[kk--] = GET(TypingWord[ii]);
        }
        
        hBPC = _index - VSI;
    }
    hNCC = hBPC;
}

void insertD(const Uint16& data, const bool& isCaps) {
    hCode = vWillProcess;
    hBPC = 0;
    for (ii = _index - 1; ii >= 0; ii--) {
        hBPC++;
        if (CHR(ii) == KEY_D) { //reverse unicode char
            if (TypingWord[ii] & TONE_MASK) {
                //restore and disable temporary
                hCode = vRestore;
                TypingWord[ii] &= ~TONE_MASK;
                hData[_index - 1 - ii] = TypingWord[ii];
                tempDisableKey = true;
                break;
            } else {
                TypingWord[ii] |= TONE_MASK;
                hData[_index - 1 - ii] = GET(TypingWord[ii]);
            }
            break;
        } else { //preresent old char
            hData[_index - 1 - ii] = GET(TypingWord[ii]);
        }
    }
    hNCC = hBPC;
}

void insertAOE(const Uint16& data, const bool& isCaps) {
    findAndCalculateVowel();
    
    //remove W tone
    for (ii = VSI; ii <= VEI; ii++) {
        TypingWord[ii] &= ~TONEW_MASK;
    }
    
    hCode = vWillProcess;
    hBPC = 0;
    
    for (ii = _index - 1; ii >= 0; ii--) {
        hBPC++;
        if (CHR(ii) == data) { //reverse unicode char
            if (TypingWord[ii] & TONE_MASK) {
                //restore and disable temporary
                hCode = vRestore;
                TypingWord[ii] &= ~TONE_MASK;
                hData[_index - 1 - ii] = TypingWord[ii];
                //_index = 0;
                if (data != KEY_O) //case thoòng
                    tempDisableKey = true;
                break;
            } else {
                TypingWord[ii] |= TONE_MASK;
                if (!IS_KEY_D(data))
                    TypingWord[ii] &= ~TONEW_MASK;
                hData[_index - 1 - ii] = GET(TypingWord[ii]);
                
            }
            break;
        } else { //preresent old char
            hData[_index - 1 - ii] = GET(TypingWord[ii]);
        }
    }
    hNCC = hBPC;
}

void insertW(const Uint16& data, const bool& isCaps) {
    isRestoredW = false;
    
    findAndCalculateVowel();
    
    //remove ^ tone
    for (ii = VSI; ii <= VEI; ii++) {
        TypingWord[ii] &= ~TONE_MASK;
    }
    
    if (vowelCount > 1) {
        hBPC = _index - VSI;
        hNCC = hBPC;
        
        if (((TypingWord[VSI] & TONEW_MASK) && (TypingWord[VSI+1] & TONEW_MASK)) ||
            ((TypingWord[VSI] & TONEW_MASK) && CHR(VSI+1) == KEY_I) ||
            ((TypingWord[VSI] & TONEW_MASK) && CHR(VSI+1) == KEY_A)){
            //restore and disable temporary
            hCode = vRestore;
            
            for (ii = VSI; ii < _index; ii++) {
                TypingWord[ii] &= ~TONEW_MASK;
                hData[_index - 1 - ii] = GET(TypingWord[ii]) & ~STANDALONE_MASK;
            }
            isRestoredW = true;
            tempDisableKey = true;
        } else {
            hCode = vWillProcess;
            
            if ((CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_O)) {
                if (VSI - 2 >= 0 && TypingWord[VSI - 2] == KEY_T && TypingWord[VSI - 1] == KEY_H) {
                    TypingWord[VSI+1] |= TONEW_MASK;
                    if (VSI + 2 < _index && CHR(VSI+2) == KEY_N) {
                        TypingWord[VSI] |= TONEW_MASK;
                    }
                } else if (VSI - 1 >= 0 && TypingWord[VSI - 1] == KEY_Q) {
                    TypingWord[VSI+1] |= TONEW_MASK;
                } else {
                    TypingWord[VSI] |= TONEW_MASK;
                    TypingWord[VSI+1] |= TONEW_MASK;
                }
            } else if ((CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_A) ||
                       (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_I) ||
                       (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_U) ||
                       (CHR(VSI) == KEY_O && CHR(VSI+1) == KEY_I)) {
                TypingWord[VSI] |= TONEW_MASK;
            } else if ((CHR(VSI) == KEY_I && CHR(VSI+1) == KEY_O) ||
                       (CHR(VSI) == KEY_O && CHR(VSI+1) == KEY_A)) {
                TypingWord[VSI+1] |= TONEW_MASK;
            } else {
                //don't do anything
                tempDisableKey = true;
                isChanged = false;
                hCode = vDoNothing;
            }
            
            for (ii = VSI; ii < _index; ii++) {
                hData[_index - 1 - ii] = GET(TypingWord[ii]);
            }
        }
        
        return;
    }
    
    hCode = vWillProcess;
    hBPC = 0;
    
    for (ii = _index - 1; ii >= 0; ii--) {
        if (ii < VSI)
            break;
        hBPC++;
        switch (CHR(ii)) {
            case KEY_A:
            case KEY_U:
            case KEY_O:
                if (TypingWord[ii] & TONEW_MASK) {
                    //restore and disable temporary
                    if (TypingWord[ii] & STANDALONE_MASK) {
                        hCode = vWillProcess;
                        if (CHR(ii) == KEY_U){
                            TypingWord[ii] = KEY_W | ((TypingWord[ii] & CAPS_MASK) ? CAPS_MASK : 0);
                        } else if (CHR(ii) == KEY_O) {
                            hCode = vRestore;
                            TypingWord[ii] = KEY_O | ((TypingWord[ii] & CAPS_MASK) ? CAPS_MASK : 0);
                            isRestoredW = true;
                        }
                        hData[_index - 1 - ii] = TypingWord[ii];
                    } else {
                        hCode = vRestore;
                        TypingWord[ii] &= ~TONEW_MASK;
                        hData[_index - 1 - ii] = TypingWord[ii];
                        isRestoredW = true;
                        //_index++;
                    }
                    
                    tempDisableKey = true;
                } else {
                    TypingWord[ii] |= TONEW_MASK;
                    TypingWord[ii] &= ~TONE_MASK;
                    hData[_index - 1 - ii] = GET(TypingWord[ii]);
                }
                break;
                
            default:
                hData[_index - 1 - ii] = GET(TypingWord[ii]);
                break;
        }
    }
    hNCC = hBPC;
    
    if (isRestoredW) {
        //_index = 0;
    }
}

void reverseLastStandaloneChar(const Uint32& keyCode, const bool& isCaps) {
    hCode = vWillProcess;
    hBPC = 0;
    hNCC = 1;
    hExt = 4;
    TypingWord[_index - 1] = (keyCode | TONEW_MASK | STANDALONE_MASK | (isCaps ? CAPS_MASK : 0));
    hData[0] = GET(TypingWord[_index - 1]);
}

void checkForStandaloneChar(const Uint16& data, const bool& isCaps, const Uint32& keyWillReverse) {
    if (CHR(_index - 1) == keyWillReverse && TypingWord[_index - 1] & TONEW_MASK) {
        hCode = vWillProcess;
        hBPC = 1;
        hNCC = 1;
        TypingWord[_index - 1] = data | (isCaps ? CAPS_MASK : 0);
        hData[0] = GET(TypingWord[_index - 1]);
        return;
    }
    
    //check standalone w -> ư
    
    if (_index > 0 && CHR(_index-1) == KEY_U && keyWillReverse == KEY_O) {
        insertKey(keyWillReverse, isCaps);
        reverseLastStandaloneChar(keyWillReverse, isCaps);
        return;
    }
    
    if (_index == 0) { //zero char
        insertKey(data, isCaps, false);
        reverseLastStandaloneChar(keyWillReverse, isCaps);
        return;
    } else if (_index == 1) { //1 char
        for (i = 0; i < _standaloneWbad.size(); i++) {
            if (CHR(0) == _standaloneWbad[i]) {
                insertKey(data, isCaps);
                return;
            }
        }
        insertKey(data, isCaps, false);
        reverseLastStandaloneChar(keyWillReverse, isCaps);
        return;
    } else if (_index == 2) {
        for (i = 0; i < _doubleWAllowed.size(); i++) {
            if (CHR(0) == _doubleWAllowed[i][0] && CHR(1) == _doubleWAllowed[i][1]) {
                insertKey(data, isCaps, false);
                reverseLastStandaloneChar(keyWillReverse, isCaps);
                return;
            }
        }
        insertKey(data, isCaps);
        return;
    }
    
    insertKey(data, isCaps);
}

bool isSpecialKeyCustom(Uint16 keyCode) {
    for (const auto& rule : customRules) {
        if ((rule.key & CHAR_MASK) == keyCode) {
            return true;
        }
    }
    return false;
}

void checkForStandaloneCharTBT(const Uint16& data, const bool& isCaps, const Uint32& keyWillReverse, const Uint32& maskToAdd) {
    if (shouldKeepTBTStandaloneKeyRaw(data) && restoreTBTStandaloneThenInsertRaw(data, isCaps)) {
        return;
    }

    if (_index > 0 && CHR(_index - 1) == keyWillReverse && (TypingWord[_index - 1] & maskToAdd) && (TypingWord[_index - 1] & STANDALONE_MASK)) {
        hCode = vWillProcess;
        hBPC = 1;
        hNCC = 1;
        hExt = 4;
        TypingWord[_index - 1] = data | (isCaps ? CAPS_MASK : 0); // No STANDALONE_MASK
        hData[0] = GET(TypingWord[_index - 1]);
        saveWord();
        return;
    }
    if (shouldKeepTBTStandaloneKeyRaw(data)) {
        hCode = vDoNothing;
        hBPC = 0;
        hNCC = 0;
        hExt = 3;
        insertKey(data, isCaps);
        return;
    }
    hCode = vWillProcess;
    hBPC = 0;
    hNCC = 1;
    hExt = 4;
    TypingWord[_index] = (keyWillReverse | maskToAdd | STANDALONE_MASK | (isCaps ? CAPS_MASK : 0));
    hData[0] = GET(TypingWord[_index]);
    _index++;
    saveWord();
}

void upperCaseFirstCharacter() {
    if (!(TypingWord[0] & CAPS_MASK)) {
        hCode = vWillProcess;
        hBPC = 0;
        hNCC = 1;
        TypingWord[0] |= CAPS_MASK;
        hData[0] = GET(TypingWord[0]);
        _upperCaseStatus = 0;
        if (vUseMacro)
            hMacroKey[0] |= CAPS_MASK;
    }
}

void handleMainKey(const Uint16& data, const bool& isCaps) {
    //if is Z key, remove mark
    if (IS_KEY_Z(data)) {
        removeMark();
        if (!isChanged) {
            insertKey(data, isCaps);
        }
        return;
    }
    
    // Tư Bình Trần và Tự Định Nghĩa standalone keys
    if (vInputType == vTuBinhTranDonGian) {
        if (data == KEY_6) {
            checkForStandaloneCharTBT(data, isCaps, KEY_A, TONE_MASK);
            return;
        }
        if (data == KEY_7) {
            checkForStandaloneCharTBT(data, isCaps, KEY_E, TONE_MASK);
            return;
        }
        if (data == KEY_8) {
            checkForStandaloneCharTBT(data, isCaps, KEY_O, TONE_MASK);
            return;
        }
        if (data == KEY_9) {
            checkForStandaloneCharTBT(data, isCaps, KEY_A, TONEW_MASK);
            return;
        }
    }
    
    if (vInputType == vTuDinhNghia) {
        Uint32 lookupKey = data | (isCaps ? CAPS_MASK : 0);
        for (const auto& rule : customRules) {
            if (rule.key == lookupKey) {
                int action = rule.action;
                
                auto hasChar = [](Uint16 targetKey) -> bool {
                    for (int ii = _index - 1; ii >= 0; ii--) {
                        if (CHR(ii) == targetKey) return true;
                    }
                    return false;
                };

                auto hasAnyChar = [&](const vector<Uint16>& targets) -> bool {
                    for (Uint16 t : targets) {
                        if (hasChar(t)) return true;
                    }
                    return false;
                };

                bool applicable = false;
                if (action >= 1 && action <= 5) {
                    vowelCount = 0;
                    findAndCalculateVowel();
                    applicable = (vowelCount > 0);
                } else if (action == 6) {
                    vowelCount = 0;
                    findAndCalculateVowel();
                    applicable = (vowelCount > 0 && (CHR(VEI) == KEY_A || CHR(VEI) == KEY_E || CHR(VEI) == KEY_O));
                } else if (action == 7) {
                    applicable = hasChar(KEY_A);
                } else if (action == 8) {
                    applicable = hasChar(KEY_E);
                } else if (action == 9) {
                    applicable = hasChar(KEY_O);
                } else if (action == 10) {
                    applicable = hasAnyChar({KEY_A, KEY_U, KEY_O});
                } else if (action == 11) {
                    applicable = hasChar(KEY_U) && hasChar(KEY_O);
                } else if (action == 12) {
                    applicable = hasChar(KEY_U);
                } else if (action == 13) {
                    applicable = hasChar(KEY_O);
                } else if (action == 14) {
                    applicable = hasChar(KEY_A);
                } else if (action == 15) {
                    applicable = hasChar(KEY_D);
                } else {
                    applicable = true;
                }

                if (applicable) {
                    if (action == 0) {
                        removeMark();
                        if (!isChanged) insertKey(data, isCaps);
                        return;
                    }
                    if (action >= 1 && action <= 5) {
                        Uint32 masks[] = { MARK1_MASK, MARK2_MASK, MARK3_MASK, MARK4_MASK, MARK5_MASK };
                        insertMark(masks[action - 1]);
                        return;
                    }
                    if (action == 6) {
                        findAndCalculateVowel();
                        if (vowelCount > 0) {
                            Uint32 lastVowel = CHR(VEI);
                            if (lastVowel == KEY_A || lastVowel == KEY_E || lastVowel == KEY_O) {
                                insertAOE(lastVowel, isCaps);
                            }
                        }
                        return;
                    }
                    if (action == 7) { insertAOE(KEY_A, isCaps); return; }
                    if (action == 8) { insertAOE(KEY_E, isCaps); return; }
                    if (action == 9) { insertAOE(KEY_O, isCaps); return; }
                    if (action == 10) { insertCustomW(data, isCaps, false); return; }
                    if (action == 11) { insertCustomW(data, isCaps, false); return; }
                    if (action == 12) {
                        findAndCalculateVowel();
                        if (vowelCount > 0 && CHR(VEI) == KEY_U) {
                            insertW(KEY_U, isCaps);
                        }
                        return;
                    }
                    if (action == 13) {
                        findAndCalculateVowel();
                        if (vowelCount > 0 && CHR(VEI) == KEY_O) {
                            insertW(KEY_O, isCaps);
                        }
                        return;
                    }
                    if (action == 14) {
                        findAndCalculateVowel();
                        if (vowelCount > 0 && CHR(VEI) == KEY_A) {
                            insertW(KEY_A, isCaps);
                        }
                        return;
                    }
                    if (action == 15) {
                        insertD(data, isCaps);
                        return;
                    }
                    if (action == 16 || action == 17) {
                        insertCustomW(data, isCaps, action == 16);
                        return;
                    }
                    if (action == 18) {
                        insertKey(data, isCaps);
                        return;
                    }
                    if (action == 19) { checkForStandaloneCharTBT(data, false, KEY_A, TONEW_MASK); return; }
                    if (action == 20) { checkForStandaloneCharTBT(data, true, KEY_A, TONEW_MASK | CAPS_MASK); return; }
                    if (action == 21) { checkForStandaloneCharTBT(data, false, KEY_A, TONE_MASK); return; }
                    if (action == 22) { checkForStandaloneCharTBT(data, true, KEY_A, TONE_MASK | CAPS_MASK); return; }
                    if (action == 23) { checkForStandaloneCharTBT(data, false, KEY_D, TONE_MASK); return; }
                    if (action == 24) { checkForStandaloneCharTBT(data, true, KEY_D, TONE_MASK | CAPS_MASK); return; }
                    if (action == 25) { checkForStandaloneCharTBT(data, false, KEY_E, TONE_MASK); return; }
                    if (action == 26) { checkForStandaloneCharTBT(data, true, KEY_E, TONE_MASK | CAPS_MASK); return; }
                    if (action == 27) { checkForStandaloneCharTBT(data, false, KEY_O, TONE_MASK); return; }
                    if (action == 28) { checkForStandaloneCharTBT(data, true, KEY_O, TONE_MASK | CAPS_MASK); return; }
                    if (action == 29) { checkForStandaloneCharTBT(data, false, KEY_O, TONEW_MASK); return; }
                    if (action == 30) { checkForStandaloneCharTBT(data, true, KEY_O, TONEW_MASK | CAPS_MASK); return; }
                    if (action == 31) { checkForStandaloneCharTBT(data, false, KEY_U, TONEW_MASK); return; }
                    if (action == 32) { checkForStandaloneCharTBT(data, true, KEY_U, TONEW_MASK | CAPS_MASK); return; }
                }
            }
        }
    }
    
    if (data == KEY_LEFT_BRACKET) { //standalone key [
        if (shouldKeepTBTStandaloneKeyRaw(data) && restoreTBTStandaloneThenInsertRaw(data, isCaps))
            return;
        checkForStandaloneChar(data, isCaps, (vInputType == vTuBinhTranDonGian) ? KEY_U : KEY_O);
        return;
    }
    
    if (data == KEY_RIGHT_BRACKET) { //standalone key ]
        if (shouldKeepTBTStandaloneKeyRaw(data) && restoreTBTStandaloneThenInsertRaw(data, isCaps))
            return;
        checkForStandaloneChar(data, isCaps, (vInputType == vTuBinhTranDonGian) ? KEY_O : KEY_U);
        return;
    }
    
    //if is D key
    if (IS_KEY_D(data)) {
        isCorect = false;
        isChanged = false;
        k = _index;
        for (i = 0; i < _consonantD.size(); i++) {
            if (_index < _consonantD[i].size())
                continue;
            isCorect = true;
            checkCorrectVowel(_consonantD, i, k, data);
            
            //allow d after consonant
            if (!isCorect && _index - 2 >= 0 && CHR(_index-1) == KEY_D && IS_CONSONANT(CHR(_index-2))) {
                isCorect = true;
            }
            if (isCorect) {
                isChanged = true;
                insertD(data, isCaps);
                break;
            }
        }
    
        if (!isChanged) {
            insertKey(data, isCaps);
        }
        return;
    }
    
    //if is mark key
    if (IS_MARK_KEY(data)) {
        for (auto& vowelEntry : _vowelForMark) {
            vector<vector<Uint16>>& charset = vowelEntry.second;
            isCorect = false;
            isChanged = false;
            k = _index;
            for (l = 0; l < charset.size(); l++) {
                if (_index < charset[l].size())
                    continue;
                isCorect = true;
                checkCorrectVowel(charset, l, k, data);

                if (isCorect) {
                    isChanged = true;
                    if (IS_KEY_S(data))
                        insertMark(MARK1_MASK);
                    else if (IS_KEY_F(data))
                        insertMark(MARK2_MASK);
                    else if (IS_KEY_R(data))
                        insertMark(MARK3_MASK);
                    else if (IS_KEY_X(data))
                        insertMark(MARK4_MASK);
                    else if (IS_KEY_J(data))
                        insertMark(MARK5_MASK);
                    break;
                }
            }

            if (isCorect) {
                break;
            }
        }

        if (!isChanged) {
            insertKey(data, isCaps);
        }

        return;
    }
    
    //check Vowel
    if (vInputType == vVNI) {
        for (i = _index-1; i >= 0; i--) {
            if (CHR(i) == KEY_O || CHR(i) == KEY_A || CHR(i) == KEY_E) {
                VEI = i;
                break;
            }
        }
    }
    
    keyForAEO = (vInputType == vTuBinhTranDonGian) ? 
                    (data == KEY_6 ? KEY_A : 
                     (data == KEY_7 ? KEY_E : 
                      (data == KEY_8 ? KEY_O : 
                       (data == KEY_9 ? KEY_W : data)))) :
                ((vInputType != vVNI) ? data : 
                    ((data == KEY_7 || data == KEY_8 ? KEY_W : 
                      (data == KEY_6 ? TypingWord[VEI] : data))));
    vector<vector<Uint16>>& charset = _vowel[keyForAEO];
    isCorect = false;
    isChanged = false;
    k = _index;
    for (i = 0; i < charset.size(); i++) {
        if (_index < charset[i].size())
            continue;
        isCorect = true;
        checkCorrectVowel(charset, i, k, data);
        
        if (isCorect) {
            isChanged = true;
            if (IS_KEY_DOUBLE(data)) {
                insertAOE(keyForAEO, isCaps);
            } else if (IS_KEY_W(data)) {
                if (vInputType == vVNI) {
                    for (j = _index-1; j >= 0; j--) {
                        if (CHR(j) == KEY_O || CHR(j) == KEY_U ||CHR(j) == KEY_A || CHR(j) == KEY_E) {
                            VEI = j;
                            break;
                        }
                    }
                    if ((data == KEY_7 && CHR(VEI) == KEY_A && (VEI-1>=0 ? CHR(VEI-1) != KEY_U : true)) || (data == KEY_8 && (CHR(VEI) == KEY_O || CHR(VEI) == KEY_U)))
                        break;
                }
                insertW(keyForAEO, isCaps);
            }
            break;
        }
    }
    
    if (!isChanged) {
        if (data == KEY_W && vInputType != vSimpleTelex1) {
            checkForStandaloneChar(data, isCaps, KEY_U);
        } else {
            insertKey(data, isCaps);
        }
    }
}

void handleQuickTelex(const Uint16& data, const bool& isCaps) {
    hCode = vWillProcess;
    hBPC = 1;
    hNCC = 2;
    hData[1] = _quickTelex[data][0] | (isCaps ? CAPS_MASK : 0);
    hData[0] = _quickTelex[data][1] | (isCaps ? CAPS_MASK : 0);
    insertKey(_quickTelex[data][1], isCaps, false);
}

bool checkRestoreIfWrongSpelling(const int& handleCode) {
    for (ii = 0; ii < _index; ii++) {
        if (!IS_CONSONANT(CHR(ii)) &&
            (TypingWord[ii] & MARK_MASK || TypingWord[ii] & TONE_MASK || TypingWord[ii] & TONEW_MASK)) {
            
            hCode = handleCode;
            hBPC = _index;
            hNCC = _stateIndex;
            for (i = 0; i < _stateIndex; i++) {
                TypingWord[i] = KeyStates[i];
                hData[_stateIndex - 1 - i] = TypingWord[i];
            }
            _index = _stateIndex;
            return true;
        }
    }
    return false;
}

void vTempOffSpellChecking() {
    if (_useSpellCheckingBefore) {
        vCheckSpelling = vCheckSpelling ? 0 : 1;
    }
}

void vSetCheckSpelling() {
    _useSpellCheckingBefore = vCheckSpelling;
}

void vTempOffEngine(const bool& off) {
    _willTempOffEngine = off;
}

bool checkQuickConsonant() {
    if (_index <= 1) return false;
    l = 0;
    if (_index > 0) {
        if (vQuickStartConsonant && _quickStartConsonant.find(CHR(0)) != _quickStartConsonant.end()) {
            hCode = vRestore;
            hBPC = _index;
            hNCC = _index + 1;
            if (_index < MAX_BUFF-1)
                _index++;
            //right shift
            for (i = _index-1; i >= 2; i--) {
                TypingWord[i] = TypingWord[i-1];
            }
            TypingWord[1] = _quickStartConsonant[CHR(0)][1] | ((TypingWord[0] & CAPS_MASK) && (TypingWord[2] & CAPS_MASK) ? CAPS_MASK : 0);
            TypingWord[0] = _quickStartConsonant[CHR(0)][0] | (TypingWord[0] & CAPS_MASK ? CAPS_MASK : 0);
            l = 1;;
        }
        if (vQuickEndConsonant &&
            (_index-2 >= 0 && !IS_CONSONANT(CHR(_index-2))) &&
            _quickEndConsonant.find(CHR(_index-1)) != _quickEndConsonant.end()) {
            hCode = vRestore;
            if (l == 1) {
                hNCC++;
            } else {
                hBPC = 1;
                hNCC = 2;
            }
            if (_index < MAX_BUFF-1)
                _index++;
            TypingWord[_index-1] = _quickEndConsonant[CHR(_index-2)][1] | (TypingWord[_index-2] & CAPS_MASK ? CAPS_MASK : 0);
            TypingWord[_index-2] = _quickEndConsonant[CHR(_index-2)][0] | (TypingWord[_index-2] & CAPS_MASK ? CAPS_MASK : 0);
            
            l = 1;
        }
        if (l == 1) {
            _hasHandleQuickConsonant = true;
            for (i = _index - 1; i >= 0; i--) {
                hData[_index - 1 - i] = GET(TypingWord[i]);
            }
            return true;
        }
    }
    return false;
}
/*==========================================================================================================*/

void vEnglishMode(const vKeyEventState& state, const Uint16& data, const bool& isCaps, const bool& otherControlKey) {
    hCode = vDoNothing;
    hExt = 0;
    if (state == vKeyEventState::MouseDown || (otherControlKey && !isCaps)) {
        hMacroKey.clear();
        _willTempOffEngine = false;
    } else if (data == KEY_SPACE || data == KEY_RETURN || data == KEY_ENTER || data == VK_LSHIFT || data == VK_RSHIFT || data == VK_LCONTROL || data == VK_RCONTROL) {
        Byte macroBackspace = 0;
        if (isMacroTriggerKey(data) && !_hasHandledMacro && findMacroMatch(hMacroKey, hMacroData, macroBackspace)) {
            hCode = vReplaceMaro;
            hBPC = macroBackspace;
        }
        hMacroKey.clear();
        _willTempOffEngine = false;
    } else if (isMacroBreakCode(data)) {
        if (canAppendCurrentKeyToMacro(data, isCaps)) {
            hMacroKey.push_back(data | (isCaps ? CAPS_MASK : 0));
        }
    } else if (data == KEY_DELETE) {
        if (hMacroKey.size() > 0) {
            hMacroKey.pop_back();
        } else {
            _willTempOffEngine = false;
        }
    } else {
        if (isWordBreak(vKeyEvent::Keyboard, state, data) &&
            std::find(_charKeyCode.begin(), _charKeyCode.end(), data) == _charKeyCode.end()) {
            hMacroKey.clear();
            _willTempOffEngine = false;
        } else {
            if (!_willTempOffEngine)
                hMacroKey.push_back(data | (isCaps ? CAPS_MASK : 0));
        }
    }
}

void vKeyHandleEvent(const vKeyEvent& event,
                     const vKeyEventState& state,
                     const Uint16& data,
                     const Uint8& capsStatus,
                     const bool& otherControlKey) {
    _isCaps = (capsStatus == 1 || //shift
               capsStatus == 2); //caps lock
               
    bool isTriggerModifier = (data == VK_LSHIFT && (vMacroTriggerMask & 0x04)) ||
                             (data == VK_RSHIFT && (vMacroTriggerMask & 0x08)) ||
                             (data == VK_LCONTROL && (vMacroTriggerMask & 0x10)) ||
                             (data == VK_RCONTROL && (vMacroTriggerMask & 0x20));

    bool isTBTOrCustomNumberAction = (vInputType == vTuBinhTranDonGian || vInputType == vTuDinhNghia) &&
                                     IS_TBT_STANDALONE_KEY(data);
    bool isNumberBreak = (IS_NUMBER_KEY(data) && capsStatus == 1) && !isTBTOrCustomNumberAction;
    bool isNumberAtStartBreak = (_index == 0 && IS_NUMBER_KEY(data)) && !isTBTOrCustomNumberAction;

    if (isNumberBreak
        || otherControlKey || isWordBreak(event, state, data) || isNumberAtStartBreak || isTriggerModifier) {
        hCode = vDoNothing;
        hBPC = 0;
        hNCC = 0;
        hExt = 1; //word break
        
        //check macro feature
        if (vUseMacro && isMacroTriggerKey(data) && !_hasHandledMacro) {
            Byte macroBackspace = 0;
            if (findMacroMatch(hMacroKey, hMacroData, macroBackspace)) {
                hCode = vReplaceMaro;
                hBPC = macroBackspace;
                hExt = 5;
                _hasHandledMacro = true;
            }
        } else if ((vQuickStartConsonant || vQuickEndConsonant) && !tempDisableKey && isMacroBreakCode(data)) {
            checkQuickConsonant();
        } else if (vRestoreIfWrongSpelling && isWordBreak(event, state, data)) { //restore key if wrong spelling with break-key
            if (!tempDisableKey && vCheckSpelling) {
                checkSpelling(true); //force check spelling
            }
            if (tempDisableKey && !checkRestoreIfWrongSpelling(vRestoreAndStartNewSession)) {
                hCode = vDoNothing;
            }
        }
        
        _isCharKeyCode = state == KeyDown && std::find(_charKeyCode.begin(), _charKeyCode.end(), data) != _charKeyCode.end();
        if (!_isCharKeyCode) { //clear all line cache
            _specialChar.clear();
            _typingStates.clear();
        } else { //check and save current word
            if (_spaceCount > 0) {
                saveWord(KEY_SPACE, _spaceCount);
                _spaceCount = 0;
            } else {
                saveWord();
            }
            _specialChar.push_back(data | (_isCaps ? CAPS_MASK : 0));
            if (hExt != 5) {
                hExt = 3;//normal word
            }
        }
        
        if (hCode == vDoNothing) {
            startNewSession();
            vCheckSpelling = _useSpellCheckingBefore;
            _willTempOffEngine = false;
        } else if (hCode == vReplaceMaro || _hasHandleQuickConsonant) {
            _index = 0;
        }
        
        //insert key for macro function
        if (vUseMacro) {
            if (_isCharKeyCode && hExt != 5) {
                hMacroKey.push_back(data | (_isCaps ? CAPS_MASK : 0));
            } else {
                hMacroKey.clear();
            }
        }
        
        if (vUpperCaseFirstChar) {
            if (data == KEY_DOT)
                _upperCaseStatus = 1;
            else if (data == KEY_ENTER || data == KEY_RETURN)
                _upperCaseStatus = 2;
            else
                _upperCaseStatus = 0;
        }
    } else if (data == KEY_SPACE) {
        if (!tempDisableKey && vCheckSpelling) {
            checkSpelling(true); //force check spelling
        }
        Byte macroBackspace = 0;
        if (vUseMacro && (vMacroTriggerMask & 0x01) && !_hasHandledMacro && findMacroMatch(hMacroKey, hMacroData, macroBackspace)) { //macro
            hCode = vReplaceMaro;
            hBPC = macroBackspace;
            hExt = 6;
            _spaceCount = 0;
            _hasHandledMacro = true;
        } else if ((vQuickStartConsonant || vQuickEndConsonant) && !tempDisableKey && checkQuickConsonant()) {
            _spaceCount++;
        } else if (vRestoreIfWrongSpelling && tempDisableKey && !_hasHandledMacro) { //restore key if wrong spelling
            if (!checkRestoreIfWrongSpelling(vRestore)) {
                hCode = vDoNothing;
            }
            _spaceCount++;
        } else { //do nothing with SPACE KEY
            hCode = vDoNothing;
            _spaceCount++;
        }
        if (vUseMacro) {
            hMacroKey.clear();
        }
        if (vUpperCaseFirstChar && _upperCaseStatus == 1) {
            _upperCaseStatus = 2;
        }
        //save word
        if (_spaceCount == 1) {
            if (_specialChar.size() > 0) {
                saveSpecialChar();
            } else {
                saveWord();
            }
        }
        vCheckSpelling = _useSpellCheckingBefore;
        _willTempOffEngine = false;
    } else if (data == KEY_DELETE) {
        hCode = vDoNothing;
        hExt = 2; //delete
        if (_specialChar.size() > 0) {
            _specialChar.pop_back();
            if (_specialChar.size() == 0) {
                restoreLastTypingState();
            }
        } else if (_spaceCount > 0) { //previous char is space
            _spaceCount--;
            if (_spaceCount == 0) { //restore word
                restoreLastTypingState();
            }
        } else {
            if (_stateIndex > 0) {
                _stateIndex--;
            }
            if (_index > 0){
                _index--;
                if (_longWordHelper.size() > 0) {
                    //right shift
                    for (i = MAX_BUFF - 1; i > 0; i--) {
                        TypingWord[i] = TypingWord[i-1];
                    }
                    TypingWord[0] = _longWordHelper.back();
                    _longWordHelper.pop_back();
                    _index++;
                }
                if (vCheckSpelling)
                    checkSpelling();
            }
            if (vUseMacro && hMacroKey.size() > 0) {
                hMacroKey.pop_back();
            }
            
            hBPC = 0;
            hNCC = 0;
            hExt = 2; //delete key
            if (_index == 0) {
                startNewSession();
                _specialChar.clear();
                restoreLastTypingState();
            } else { //August 23rd continue check grammar
                checkGrammar(1);
            }
        }
    } else { //START AND CHECK KEY
        if (_willTempOffEngine) {
            hCode = vDoNothing;
            hExt = 3;
            return;
        }
        if (_spaceCount > 0) {
            hBPC = 0;
            hNCC = 0;
            hExt = 0;
            startNewSession();
            //continute save space
            saveWord(KEY_SPACE, _spaceCount);
            _spaceCount = 0;
        } else if (_specialChar.size() > 0) {
            saveSpecialChar();
        }

        insertState(data, _isCaps); //save state
        
        if (!IS_SPECIALKEY(data) || tempDisableKey) { //do nothing
            if (vQuickTelex && IS_QUICK_TELEX_KEY(data)) {
                handleQuickTelex(data, _isCaps);
                return;
            } else {
                hCode = vDoNothing;
                hBPC = 0;
                hNCC = 0;
                hExt = 3; //normal key
                insertKey(data, _isCaps);
            }
        } else { //check and update key
            //restore state
            hCode = vDoNothing;
            hExt = 3; //normal key
            handleMainKey(data, _isCaps);
        }

        if (!vFreeMark && !IS_KEY_D(data)) {
            if (hCode == vDoNothing) {
                checkGrammar(-1);
            } else {
                checkGrammar(0);
            }
        }
        
        if (hCode == vRestore) {
            insertKey(data, _isCaps);
            _stateIndex--;
        }
        
        //insert or replace key for macro feature
        if (vUseMacro) {
            hMacroKey.push_back(data | (_isCaps ? CAPS_MASK : 0));
        }
        
        if (vUpperCaseFirstChar) {
            if (_index == 1 && _upperCaseStatus == 2) {
                upperCaseFirstCharacter();
            }
            _upperCaseStatus = 0;
        }
        
        //case [ ]
        bool isCustomStandalone = false;
        if (vInputType == vTuDinhNghia) {
            Uint32 lookupKey = data | (_isCaps ? CAPS_MASK : 0);
            for (const auto& r : customRules) {
                if (r.key == lookupKey && r.action >= 19) {
                    isCustomStandalone = true;
                    break;
                }
            }
        }
        bool isTBTStandalone = (vInputType == vTuBinhTranDonGian) && IS_TBT_STANDALONE_KEY(data) && !shouldKeepTBTStandaloneKeyRaw(data) && !canTBTStandaloneStartVowel(TypingWord[_index - 1]);
        if ((IS_BRACKET_KEY(data) && (( IS_BRACKET_KEY( (((Uint16)hData[0] & ~CAPS_MASK)) )) || vInputType == vSimpleTelex1 || vInputType == vSimpleTelex2)) ||
            ((isTBTStandalone || isCustomStandalone) && ((((Uint16)hData[0] & ~CAPS_MASK)) == data))) {
            if (_index - (hCode == vWillProcess ? hBPC : 0) > 0) {
                _index--;
                saveWord();
            }
            _index = 0;
            tempDisableKey = false;
            _stateIndex = 0;
            hExt = 3;
            _specialChar.push_back(data | (_isCaps ? CAPS_MASK : 0));
        }
    }
    
    //Debug
    //cout<<"index "<<(int)_index<< ", stateIndex "<<(int)_stateIndex<<", word "<<_typingStates.size()<<", long word "<<_longWordHelper.size()<< endl;
    //cout<<"backspace "<<(int)hBPC<<endl;
    //cout<<"new char "<<(int)hNCC<<endl<<endl;
}
