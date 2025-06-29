//
//  DataType.h
//  OpenKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#ifndef DataType_h
#define DataType_h

#include <vector>

//#define V_PLATFORM_MAC              1
//#define V_PLATFORM_WINDOWS          2

#define MAX_BUFF 32

enum vKeyEvent {
    Keyboard,
    Mouse
};

enum vKeyEventState {
    KeyDown,
    KeyUp,
    MouseDown,
    MouseUp
};

enum vKeyInputType {
    vTelex = 0,
    vVNI,
    vSimpleTelex1,
    vSimpleTelex2
};

typedef unsigned char Byte;
typedef signed char Int8;
typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;
typedef unsigned long int Uint64;

enum HoolCodeState {
    vDoNothing = 0, //do not do anything
    vWillProcess, //will reverse
    vBreakWord, //start new
    vRestore, //restore character to old char
    vReplaceMaro, //replace by macro
    vRestoreAndStartNewSession, //special flag: use for restore key if invalid word with break character (, . ")
};

//bytes data for main program
struct vKeyHookState {
    /*
     * 0: Do nothing
     * 1: Process
     * 2: Word break;
     * 3: Restore
     * 4: replace by macro
     */
    Byte code;
    Byte backspaceCount;
    Byte newCharCount;
    
    /**
     * 1: Word Break
     * 2: Delete key
     * 3: Normal key
     * 4: Should not send empty character
     */
    Byte extCode;
    
    Uint32 charData[MAX_BUFF]; //new character will be put in queue
    
    std::vector<Uint32> macroKey; //used for macro function; it is a key
    std::vector<Uint32> macroData; //used for macro function; it is keycode data
};

#ifdef LINUX
#include "platforms/linux.h"
#elif _WIN32
#include "platforms/win32.h"
#else
#include "platforms/mac.h"
#endif

//internal engine data
#define CAPS_MASK                               0x10000
#define TONE_MASK                               0x20000
#define TONEW_MASK                              0x40000

/*
 * MARK MASK
 * 1: Dấu Sắc - á
 * 2: Dấu Huyền - à
 * 3: Dấu Hỏi - ả
 * 4: Dấu Ngã - ã
 * 5: dấu Nặng - ạ
 */
#define MARK1_MASK                              0x80000
#define MARK2_MASK                              0x100000
#define MARK3_MASK                              0x200000
#define MARK4_MASK                              0x400000
#define MARK5_MASK                              0x800000

//for checking has mark or not
#define MARK_MASK                               0xF80000

//mark and get first 16 bytes character
#define CHAR_MASK                               0xFFFF

//Check whether the data is create by standalone key or not (W)
#define STANDALONE_MASK                         0x1000000

//Chec whether the data is keyboard code or character code
#define CHAR_CODE_MASK                          0x2000000

#define PURE_CHARACTER_MASK                     0x80000000

//for special feature
#define END_CONSONANT_MASK                      0x4000
#define CONSONANT_ALLOW_MASK                    0x8000


//Utilities macro
#define IS_CONSONANT(keyCode) !(keyCode == KEY_A || keyCode == KEY_E || keyCode == KEY_U || keyCode == KEY_Y || keyCode == KEY_I || keyCode == KEY_O)
//#define IS_MARK_KEY(keyCode) (keyCode == KEY_S || keyCode == KEY_F || keyCode == KEY_R || keyCode == KEY_J || keyCode == KEY_X)
#define CHR(index) (Uint16)TypingWord[index]
#define IS_SPECIALKEY(keyCode) \
        (vInputType == vTelex ? \
            keyCode == KEY_W || keyCode == KEY_E || keyCode == KEY_R || keyCode == KEY_O || keyCode == KEY_LEFT_BRACKET || \
            keyCode == KEY_RIGHT_BRACKET || keyCode == KEY_A || keyCode == KEY_S || keyCode == KEY_D || keyCode == KEY_F || keyCode == KEY_J || \
            keyCode == KEY_Z || keyCode == KEY_X || keyCode == KEY_W \
        : (vInputType == vVNI ? \
            keyCode == KEY_1 || keyCode == KEY_2 || keyCode == KEY_3 || keyCode == KEY_4 || \
            keyCode == KEY_5 || keyCode == KEY_6 || keyCode == KEY_7 || keyCode == KEY_8 || keyCode == KEY_9 || keyCode == KEY_0 \
        : (vInputType == vSimpleTelex1 ? \
            keyCode == KEY_W || keyCode == KEY_E || keyCode == KEY_R || keyCode == KEY_O || keyCode == KEY_A || keyCode == KEY_S || \
            keyCode == KEY_D || keyCode == KEY_F || keyCode == KEY_J ||   keyCode == KEY_Z || keyCode == KEY_X || keyCode == KEY_W \
        : (vInputType == vSimpleTelex2 ? \
            keyCode == KEY_W || keyCode == KEY_E || keyCode == KEY_R || keyCode == KEY_O || keyCode == KEY_A || keyCode == KEY_S || \
            keyCode == KEY_D || keyCode == KEY_F || keyCode == KEY_J ||   keyCode == KEY_Z || keyCode == KEY_X || keyCode == KEY_W : false))))

//is VNI or Unicode compound...
#define IS_DOUBLE_CODE(code) (code == 2 || code == 3)
#define IS_VNI_CODE(code) (code == 2)
#define IS_QUICK_TELEX_KEY(code) (_index > 0 && (code == KEY_C || code == KEY_G || code == KEY_K || code == KEY_N || code == KEY_Q || code == KEY_P || code == KEY_T) && \
                                    (Uint16)TypingWord[_index-1] == code)

#define IS_NUMBER_KEY(code) (code == KEY_1 || code == KEY_2 || code == KEY_3 || code == KEY_4 || code == KEY_5 || code == KEY_6 || code == KEY_7 || code == KEY_8 || code == KEY_9 || code == KEY_0)

#endif /* DataType_h */
