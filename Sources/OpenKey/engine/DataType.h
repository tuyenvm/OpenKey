//
//  DataType.h
//  OpenKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#ifndef DataType_h
#define DataType_h

//#define V_PLATFORM_MAC              1
//#define V_PLATFORM_WINDOWS          2

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
    vVNI
};

typedef unsigned char Byte;
typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;
typedef unsigned long int Uint64;

enum HoolCodeState {
    vDoNothing = 0, //do not do anything
    vWillProcess, //will reverse
    vBreakWord, //start new
    vRestore //restore character to old char
};

//bytes data for main program
struct vKeyHookState {
    /*
     * 0: Do nothing
     * 1: Process
     * 2: Word break;
     * 3: Restore
     */
    Byte code;
    Byte backspaceCount;
    Byte newCharCount;
    
    /**
     * 1: Word Break
     * 2: Delete key
     * 3: Normal key
     */
    Byte extCode;
    
    Uint32 charData[8]; //new character will be put in queue
};

//define Key code for mac keyboard
#define KEY_ESC                                 53
#define KEY_DELETE                              51
#define KEY_TAB                                 48
#define KEY_ENTER                               76
#define KEY_RETURN                              36
#define KEY_SPACE                               49
#define KEY_LEFT                                123
#define KEY_RIGHT                               124
#define KEY_DOWN                                125
#define KEY_UP                                  126

#define KEY_EMPTY                               256
#define KEY_A                                   0
#define KEY_B                                   11
#define KEY_C                                   8
#define KEY_D                                   2
#define KEY_E                                   14
#define KEY_F                                   3
#define KEY_G                                   5
#define KEY_H                                   4
#define KEY_I                                   34
#define KEY_J                                   38
#define KEY_K                                   40
#define KEY_L                                   37
#define KEY_M                                   46
#define KEY_N                                   45
#define KEY_O                                   31
#define KEY_P                                   35
#define KEY_Q                                   12
#define KEY_R                                   15
#define KEY_S                                   1
#define KEY_T                                   17
#define KEY_U                                   32
#define KEY_V                                   9
#define KEY_W                                   13
#define KEY_X                                   7
#define KEY_Y                                   16
#define KEY_Z                                   6

#define KEY_1                                   18
#define KEY_2                                   19
#define KEY_3                                   20
#define KEY_4                                   21
#define KEY_5                                   23
#define KEY_6                                   22
#define KEY_7                                   26
#define KEY_8                                   28
#define KEY_9                                   25
#define KEY_0                                   29

#define KEY_LEFT_BRACKET                        33
#define KEY_RIGHT_BRACKET                       30

#define KEY_LEFT_SHIFT                          57
#define KEY_RIGHT_SHIFT                         60

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

#define MAX_BUFF                                8

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
            keyCode == KEY_5 || keyCode == KEY_6 || keyCode == KEY_7 || keyCode == KEY_8 || keyCode == KEY_9 || keyCode == KEY_0 : false))

//is VNI or Unicode compound...
#define IS_DOUBLE_CODE(code) (code == 2 || code == 3)

#define IS_QUICK_TELEX_KEY(code) (_index > 0 && (code == KEY_C || code == KEY_G || code == KEY_K || code == KEY_N || code == KEY_Q || code == KEY_P || code == KEY_T) && \
                                    (Uint16)TypingWord[_index-1] == code)

#define IS_NUMBER_KEY(code) (code == KEY_1 || code == KEY_2 || code == KEY_3 || code == KEY_4 || code == KEY_5 || code == KEY_6 || code == KEY_7 || code == KEY_8 || code == KEY_9 || code == KEY_0)

#endif /* DataType_h */
