//
//  Engine.cpp
//  OpenKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
#include <iostream>

#include "Engine.h"
#include <string.h>

static vector<Uint8> _breakCode = {
    KEY_ESC, KEY_TAB, KEY_ENTER, KEY_RETURN, KEY_LEFT, KEY_RIGHT, KEY_DOWN, KEY_UP,
    43, 47, 44, 41, 39, 42, 27, 24, 50, 48
};

static Uint16 ProcessingChar[][11] = {
    {KEY_S, KEY_F, KEY_R, KEY_X, KEY_J, KEY_A, KEY_O, KEY_E, KEY_W, KEY_D, KEY_Z}, //Telex
    {KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0}, //VNI
    {KEY_S, KEY_F, KEY_R, KEY_X, KEY_J, KEY_A, KEY_O, KEY_E, KEY_W, KEY_D, KEY_Z} //Simple Telex
};

#define IS_KEY_Z(key) (ProcessingChar[vInputType][10] == key)
#define IS_KEY_D(key) (ProcessingChar[vInputType][9] == key)
#define IS_KEY_W(key) ((vInputType == vTelex || vInputType == vSimpleTelex) ? ProcessingChar[vInputType][8] == key : \
                                    (vInputType == vVNI ? (ProcessingChar[vInputType][8] == key || ProcessingChar[vInputType][7] == key) : false))
#define IS_KEY_DOUBLE(key) ((vInputType == vTelex || vInputType == vSimpleTelex) ? (ProcessingChar[vInputType][5] == key || ProcessingChar[vInputType][6] == key || ProcessingChar[vInputType][7] == key) :\
                                        (vInputType == vVNI ? ProcessingChar[vInputType][6] == key : false))
#define IS_KEY_S(key) (ProcessingChar[vInputType][0] == key)
#define IS_KEY_F(key) (ProcessingChar[vInputType][1] == key)
#define IS_KEY_R(key) (ProcessingChar[vInputType][2] == key)
#define IS_KEY_X(key) (ProcessingChar[vInputType][3] == key)
#define IS_KEY_J(key) (ProcessingChar[vInputType][4] == key)

#define IS_MARK_KEY(keyCode) (((vInputType == vTelex || vInputType == vSimpleTelex) && (keyCode == KEY_S || keyCode == KEY_F || keyCode == KEY_R || keyCode == KEY_J || keyCode == KEY_X)) || \
                                        (vInputType == vVNI && (keyCode == KEY_1 || keyCode == KEY_2 || keyCode == KEY_3 || keyCode == KEY_5 || keyCode == KEY_4)))

#define VSI vowelStartIndex
#define VEI vowelEndIndex
#define VWSM vowelWillSetMark
#define hBPC HookState.backspaceCount
#define hNCC HookState.newCharCount
#define hCode HookState.code
#define hExt HookState.extCode
#define hData HookState.charData
#define GET getCharacterCode


//Data to sendback to main program
vKeyHookState HookState;

//private data
static Byte _index = 0;
static Uint32 TypingWord[MAX_BUFF];
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

//function prototype
void findAndCalculateVowel(const bool& forGrammar=false);
void insertMark(const Uint32& markMask, const bool& canModifyFlag=true);
Uint32 getCharacterCode(const Uint32& data);


extern void _convert();
void* vKeyInit() {
    _index = 0;
    memset(&HookState, sizeof(vKeyEventState), 0);
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

void setKeyData(const Byte& index, const Uint16& keyCode, const bool& isCaps) {
    if (index < 0 || index >= MAX_BUFF)
        return;
    TypingWord[index] = keyCode | (isCaps ? CAPS_MASK : 0);
}

bool _spellingOK = false;
bool _spellingFlag = false;

void checkSpelling() {
    _spellingOK = false;
    if (_index > 0) {
        j = 0;
        //Check first consonant
        if (IS_CONSONANT(CHR(0))) {
            for (i = 0; i < _consonantTable.size(); i++) {
                _spellingFlag = false;
                if (_index < _consonantTable[i].size())
                    _spellingFlag = true;
                for (j = 0; j < _consonantTable[i].size(); j++) {
                    if (_index > j && _consonantTable[i][j] != CHR(j)) {
                        _spellingFlag = true;
                        break;
                    }
                }
                if (_spellingFlag)
                    continue;
                
                break;
            }
        }
        
        if (j == _index){ //for "d" case
            _spellingOK = true;
        }
        
        //check next vowel
        k = j;
        for (l = 0; l < 3; l++) {
            if (k < _index && !IS_CONSONANT(CHR(k)))
                k++;
        }
        
        if (k > j) { //has vowel, continue check last consonant
            for (ii = 0; ii < _endConsonantTable.size(); ii++) {
                _spellingFlag = false;
   
                for (j = 0; j < _endConsonantTable[ii].size(); j++) {
                    if (_index > k+j && _endConsonantTable[ii][j] != CHR(k + j)) {
                        _spellingFlag = true;
                        break;
                    }
                }
                if (_spellingFlag)
                    continue;
                
                if (k + j >= _index)
                    _spellingOK = true;
            }
            
        }
    } else {
        _spellingOK = true;
    }
    tempDisableKey = !_spellingOK;
    
    //cout<<"spelling: "<<(_spellingOK ? "OK": "Err")<<endl;
}

void checkGrammar(const int& deltaBackSpace) {
    if (_index <= 1 || _index >= MAX_BUFF)
        return;
    
    findAndCalculateVowel(true);
    if (vowelCount == 0)
        return;
    
    isCheckedGrammar = false;
    
    l = VSI;
    
    //check mark
    if (_index >= 2) {
        for (i = l; i <= VEI; i++) {
            if (TypingWord[i] & MARK_MASK) {
                Uint32 mark = TypingWord[i] & MARK_MASK;
                TypingWord[i] &= ~MARK_MASK;
                insertMark(mark, false);
                isCheckedGrammar = true;
                break;
            }
        }
    }
    
    //if N key for case: "thuơn", "ưoi", "ưom", "ưoc"
    if (_index >= 3) {
        for (i = _index-1; i >= 0; i--) {
            if (CHR(i) == KEY_N || CHR(i) == KEY_C || CHR(i) == KEY_I ||
                CHR(i) == KEY_M || CHR(i) == KEY_P || CHR(i) == KEY_T) {
                if (i - 2 >= 0 && CHR(i - 1) == KEY_O && CHR(i - 2) == KEY_U) {
                    if ((TypingWord[i-1] & TONEW_MASK) || (TypingWord[i-2] & TONEW_MASK)) {
                        TypingWord[i - 2] |= TONEW_MASK;
                        TypingWord[i - 1] |= TONEW_MASK;
                        isCheckedGrammar = true;
                        break;
                    }
                }
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
    }
}

void insertKey(const Uint16& keyCode, const bool& isCaps, const bool& isCheckSpelling=true) {
    if (_index >= MAX_BUFF) {
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
}

void startNewSession() {
    _index = 0;
    hBPC = 0;
    hNCC = 0;
    tempDisableKey = false;
}

void checkCorrectVowel(vector<vector<Uint16>>& charset, int& i, int& k, const Uint16& markKey) {
    k = _index - 1;
    for (j = (int)charset[i].size() - 1; j >= 0; j--) {
        if (charset[i][j] != CHR(k)) {
            isCorect = false;
            break;
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
            return data;
        
        return _codeTable[vCodeTable][key][markElem];
    } else { //doesn't has mark
        if (_codeTable[vCodeTable].find(key) == _codeTable[vCodeTable].end())
            return data;
        
        if (data & TONE_MASK) {
            return _codeTable[vCodeTable][key][capsElem];
        } else if (data & TONEW_MASK) {
            return _codeTable[vCodeTable][key][capsElem + 2];
        } else {
            return data;
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
}

void removeMark() {
    findAndCalculateVowel(true);
    isChanged = false;

    for (i = VSI; i <= VEI; i++) {
        if (TypingWord[i] & MARK_MASK) {
            TypingWord[i] &= ~MARK_MASK;
            isChanged = true;
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
    } else if ((CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_O) /*||
                                                           (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_A)*/) {
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
                if (VEI + 1 >= _index) {
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
    if (vowelCount == 3 || (VEI + 1 < _index && IS_CONSONANT(CHR(VEI + 1)))) {
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
                hData[_index - 1 - ii] = GET(TypingWord[ii]);
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
                        if ((TypingWord[ii] & CHAR_MASK) == KEY_U){
                            TypingWord[ii] = KEY_W | ((TypingWord[ii] & CAPS_MASK) ? CAPS_MASK : 0);
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
        insertKey(data, isCaps);
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

void handleMainKey(const Uint16& data, const bool& isCaps) {
    
    //if is Z key, remove mark
    if (IS_KEY_Z(data)) {
        removeMark();
        
        if (!isChanged) {
            insertKey(data, isCaps);
        }
        return;
    }
    
    if (data == KEY_LEFT_BRACKET) { //standalone key [
        checkForStandaloneChar(data, isCaps, KEY_O);
        return;
    }
    
    if (data == KEY_RIGHT_BRACKET) { //standalone key }
        checkForStandaloneChar(data, isCaps, KEY_U);
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
        for (i = 0; i < _vowelForMark.size(); i++) {
            vector<vector<Uint16>>& charset = _vowelForMark[i];
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
            if (CHR(i) == KEY_O || CHR(i) == KEY_A || CHR(i) == KEY_E)
                VEI = i;
        }
    }
    keyForAEO = ((vInputType == vTelex || vInputType == vSimpleTelex) ? data : ((data == KEY_7 || data == KEY_8 ? KEY_W : (data == KEY_6 ? TypingWord[VEI] : data))));
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
            } else if (IS_KEY_W(data) && !(vInputType == vVNI && data == KEY_8 && CHR(VEI) != KEY_A)) {
                insertW(keyForAEO, isCaps);
            }
            break;
        }
    }
    
    if (!isChanged) {
        if (data == KEY_W && vInputType != vSimpleTelex) {
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
    hData[1] = _quickTelex[data][0] | isCaps;
    hData[0] = _quickTelex[data][1] | isCaps;
    insertKey(_quickTelex[data][1], isCaps, false);
}
/*==========================================================================================================*/

void vKeyHandleEvent(const vKeyEvent& event,
                     const vKeyEventState& state,
                     const Uint16& data,
                     const Uint8& capsStatus,
                     const bool& otherControlKey) {
    _isCaps = (capsStatus == 1 || //shift
               capsStatus == 2); //caps lock
    if ((IS_NUMBER_KEY(data) && capsStatus == 1)
        || otherControlKey || isWordBreak(event, state, data) || (_index == 0 && IS_NUMBER_KEY(data))) {
        hCode = vDoNothing;
        hBPC = 0;
        hNCC = 0;
        hExt = 1; //word break
        startNewSession();
    } else if (data == KEY_SPACE) {
        hCode = vDoNothing;
        _spaceCount++;
    } else if (data == KEY_DELETE) {
        if (_spaceCount > 0) { //previous char is space
            _spaceCount--;
        } else {
            if (_index > 0){
                _index--;
                if (vCheckSpelling)
                    checkSpelling();
            }
            hCode = vDoNothing;
            hBPC = 0;
            hNCC = 0;
            hExt = 2; //delete key
            if (_index == 0)
                startNewSession();
        }
    } else {
        if (_spaceCount > 0) {
            _spaceCount = 0;
            hBPC = 0;
            hNCC = 0;
            hExt = 0;
            startNewSession();
        }
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
        }
    }
    
    //Debug
    //cout<<(int)_index<<endl;
    //cout<<(int)hBPC<<endl;
    //cout<<(int)hNCC<<endl<<endl;
}
