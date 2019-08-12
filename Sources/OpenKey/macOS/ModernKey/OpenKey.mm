//
//  OpenKey.m
//  OpenKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import "Engine.h"
#import "Macro.h"
#import "AppDelegate.h"

#import "ViewController.h"

#define FRONT_APP [[NSWorkspace sharedWorkspace] frontmostApplication].bundleIdentifier
#define OTHER_CONTROL_KEY (_flag & kCGEventFlagMaskCommand) || (_flag & kCGEventFlagMaskControl) || \
                            (_flag & kCGEventFlagMaskAlternate) || (_flag & kCGEventFlagMaskSecondaryFn) || \
                            (_flag & kCGEventFlagMaskNumericPad) || (_flag & kCGEventFlagMaskHelp)

extern ViewController* viewController;

extern AppDelegate* appDelegate;
extern int vSendKeyStepByStep;
extern "C" {
    //app which must sent special empty character
    NSArray* _niceSpaceApp = @[@"com.sublimetext.3",
                               @"com.sublimetext.2",
                             ];
    
    //app which error with unicode Compound
    NSArray* _unicodeCompoundApp = @[@"com.apple.Stickies"];
    
    CGEventSourceRef myEventSource = NULL;
    vKeyHookState* pData;
    CGEventRef eventBackSpaceDown;
    CGEventRef eventBackSpaceUp;
    UniChar _newChar, _newCharHi;
    CGEventRef _newEventDown;
    CGKeyCode _keycode;
    CGEventFlags _flag, _lastFlag = 0, _privateFlag;
    CGEventTapProxy _proxy;
    
    Uint16 _newCharString[MAX_BUFF * 2];
    Uint16 _newCharSize;
    
    Uint16 _syncKey[MAX_BUFF];
    Uint8 _syncIndex = 0;
    Uint16 _uniChar[2];
    int _i, _j;
    
    string macroText, macroContent;
    
    void OpenKeyInit() {
        memset(&_syncKey, 0, sizeof(_syncKey));
        myEventSource = CGEventSourceCreate(kCGEventSourceStatePrivate);
        pData = (vKeyHookState*)vKeyInit();

        eventBackSpaceDown = CGEventCreateKeyboardEvent (myEventSource, 51, true);
        eventBackSpaceUp = CGEventCreateKeyboardEvent (myEventSource, 51, false);
        
        NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
        NSData *data = [prefs objectForKey:@"macroData"];
        initMacroMap((Byte*)data.bytes, (int)data.length);
    }
    
    void OnTableCodeChange() {
        onTableCodeChange();
    }
    
    void InsertKeyLength(const Uint8& len) {
        if (_syncIndex >= MAX_BUFF) {
            for (_i = 1; _i < MAX_BUFF; _i++) {
                _syncKey[_i-1] = _syncKey[_i];
            }
            _syncKey[_syncIndex-1] = len;
        } else {
            _syncKey[_syncIndex++] = len;
        }
    }
    
    void SendPureCharacter(const Uint16& ch) {
        _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
        CGEventKeyboardSetUnicodeString(_newEventDown, 1, &ch);
        CGEventTapPostEvent(_proxy, _newEventDown);
        CFRelease(_newEventDown);
    }
    
    void SendKeyCode(Uint32 data) {
        _newChar = (Uint16)data;
        if (_newChar < 128) {
            if (IS_DOUBLE_CODE(vCodeTable)) //VNI
                InsertKeyLength(1);
            
            _newEventDown = CGEventCreateKeyboardEvent(myEventSource, _newChar, true);
            _privateFlag = CGEventGetFlags(_newEventDown);
            
            if (data & CAPS_MASK) {
                _privateFlag |= kCGEventFlagMaskShift;
            } else {
                _privateFlag &= ~kCGEventFlagMaskShift;
            }
            _privateFlag |= kCGEventFlagMaskNonCoalesced;
            
            CGEventSetFlags(_newEventDown, _privateFlag);
            CGEventTapPostEvent(_proxy, _newEventDown);
        } else {
            if (vCodeTable == 0) { //unicode 2 bytes code
                _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
                CGEventKeyboardSetUnicodeString(_newEventDown, 1, &_newChar);
                CGEventTapPostEvent(_proxy, _newEventDown);
            } else if (vCodeTable == 1 || vCodeTable == 2 || vCodeTable == 4) { //others such as VNI Windows, TCVN3: 1 byte code
                _newCharHi = HIBYTE(_newChar);
                _newChar = LOBYTE(_newChar);
                
                _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
                CGEventKeyboardSetUnicodeString(_newEventDown, 1, &_newChar);
                CGEventTapPostEvent(_proxy, _newEventDown);
                
                if (_newCharHi > 32) {
                    if (vCodeTable == 2) //VNI
                        InsertKeyLength(2);
                    _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
                    CGEventKeyboardSetUnicodeString(_newEventDown, 1, &_newCharHi);
                    CGEventTapPostEvent(_proxy, _newEventDown);
                } else {
                    if (vCodeTable == 2) //VNI
                        InsertKeyLength(1);
                }
            } else if (vCodeTable == 3) { //Unicode Compound
                _newCharHi = (_newChar >> 13);
                _newChar &= 0x1FFF;
                _uniChar[0] = _newChar;
                _uniChar[1] = _newCharHi > 0 ? (_unicodeCompoundMark[_newCharHi - 1]) : 0;
                InsertKeyLength(_newCharHi > 0 ? 2 : 1);
                _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
                CGEventKeyboardSetUnicodeString(_newEventDown, (_newCharHi > 0 ? 2 : 1), _uniChar);
                CGEventTapPostEvent(_proxy, _newEventDown);
            }
        }
        CFRelease(_newEventDown);
    }
    
    void SendEmptyCharacter() {
        if (IS_DOUBLE_CODE(vCodeTable)) //VNI or Unicode Compound
            InsertKeyLength(1);
        
        _newChar = 0x202F; //empty char
        if ([_niceSpaceApp containsObject:FRONT_APP]) {
            _newChar = 0x200C; //Unicode character with empty space
        }
        
        _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
        CGEventKeyboardSetUnicodeString(_newEventDown, 1, &_newChar);
        CGEventTapPostEvent(_proxy, _newEventDown);
        CFRelease(_newEventDown);
    }
    
    void SendVirtualKey(const Byte& vKey) {
        CGEventRef eventVkeyDown = CGEventCreateKeyboardEvent (myEventSource, vKey, true);
        CGEventRef eventVkeyUp = CGEventCreateKeyboardEvent (myEventSource, vKey, false);
        
        CGEventTapPostEvent(_proxy, eventVkeyDown);
        CGEventTapPostEvent(_proxy, eventVkeyUp);
        
        CFRelease(eventVkeyDown);
        CFRelease(eventVkeyUp);
    }

    void SendBackspace() {
        CGEventTapPostEvent(_proxy, eventBackSpaceDown);
        CGEventTapPostEvent(_proxy, eventBackSpaceUp);
        
        if (IS_DOUBLE_CODE(vCodeTable)) { //VNI or Unicode Compound
            _syncIndex--;
            if (_syncKey[_syncIndex] > 1) {
                if (!(vCodeTable == 3 && [_unicodeCompoundApp containsObject:FRONT_APP])) {
                    CGEventTapPostEvent(_proxy, eventBackSpaceDown);
                    CGEventTapPostEvent(_proxy, eventBackSpaceUp);
                }
            }
        }
    }
    
    void SendNewCharString() {
        int j = 0;
        _newCharSize = pData->newCharCount;
        if (pData->newCharCount > 0 && pData->newCharCount <= MAX_BUFF) {
            for (int i = pData->newCharCount - 1; i >= 0; i--) {
                if (pData->charData[i] < 128) {
                    if (IS_DOUBLE_CODE(vCodeTable)) //VNI
                        InsertKeyLength(1);
                    _newCharString[j++] = keyCodeToCharacter(pData->charData[i]);
                } else {
                    if (vCodeTable == 0) {  //unicode 2 bytes code
                        _newCharString[j++] = pData->charData[i];
                    } else if (vCodeTable == 1 || vCodeTable == 2 || vCodeTable == 4) { //others such as VNI Windows, TCVN3: 1 byte code
                        _newChar = pData->charData[i];
                        _newCharHi = HIBYTE(_newChar);
                        _newChar = LOBYTE(_newChar);
                        _newCharString[j++] = _newChar;
                        
                        if (_newCharHi > 32) {
                            if (vCodeTable == 2) //VNI
                                InsertKeyLength(2);
                            _newCharString[j++] = _newCharHi;
                            _newCharSize++;
                        } else {
                            if (vCodeTable == 2) //VNI
                                InsertKeyLength(1);
                        }
                        //_j++;
                    } else if (vCodeTable == 3) { //Unicode Compound
                        _newChar = pData->charData[i];
                        _newCharHi = (_newChar >> 13);
                        _newChar &= 0x1FFF;
                        
                        InsertKeyLength(_newCharHi > 0 ? 2 : 1);
                        _newCharString[j++] = _newChar;
                        if (_newCharHi > 0) {
                            _newCharSize++;
                            _newCharString[j++] = _unicodeCompoundMark[_newCharHi - 1];
                        }
                        
                    }
                }
            }
        }
        
        if (pData->code == 3) { //if is restore
            _newCharSize++;
            _newCharString[j++] = keyCodeToCharacter(_keycode | ((_flag & kCGEventFlagMaskAlphaShift) || (_flag & kCGEventFlagMaskShift) ? CAPS_MASK : 0));
        }
        
        _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
        CGEventKeyboardSetUnicodeString(_newEventDown, _newCharSize, _newCharString);
        CGEventTapPostEvent(_proxy, _newEventDown);
        CFRelease(_newEventDown);
    }
            
    bool handleSwitchKey(bool checkKeyCode=true) {
        if (HAS_CONTROL(vSwitchKeyStatus) ^ GET_BOOL(_lastFlag & kCGEventFlagMaskControl))
            return false;
        if (HAS_OPTION(vSwitchKeyStatus) ^ GET_BOOL(_lastFlag & kCGEventFlagMaskAlternate))
            return false;
        if (HAS_COMMAND(vSwitchKeyStatus) ^ GET_BOOL(_lastFlag & kCGEventFlagMaskCommand))
            return false;
        if (HAS_SHIFT(vSwitchKeyStatus) ^ GET_BOOL(_lastFlag & kCGEventFlagMaskShift))
            return false;
        if (checkKeyCode) {
            if (GET_SWITCH_KEY(vSwitchKeyStatus) != _keycode)
                return false;
        }
        if (vLanguage == 0)
            vLanguage = 1;
        else
            vLanguage = 0;
        if (HAS_BEEP(vSwitchKeyStatus))
            NSBeep();
        [appDelegate onInputMethodSelected];

        startNewSession();
        return true;
    }
    
    void handleMacro() {
        //fix autocomplete
        if (vFixRecommendBrowser) {
            SendEmptyCharacter();
            pData->backspaceCount++;
        }
        
        //send backspace
        if (pData->backspaceCount > 0) {
            for (int i = 0; i < pData->backspaceCount; i++) {
                SendBackspace();
            }
        }
        //send real data
        for (int i = 0; i < pData->macroData.size(); i++) {
            if (pData->macroData[i] & PURE_CHARACTER_MASK) {
                SendPureCharacter(pData->macroData[i]);
            } else {
                SendKeyCode(pData->macroData[i]);
            }
        }
        SendPureCharacter(' ');
    }

    /*MAIN Callback*/
    CGEventRef OpenKeyCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
        //dont handle my event
        if (CGEventGetIntegerValueField(event, kCGEventSourceStateID) == CGEventSourceGetSourceStateID(myEventSource)) {
            return event;
        }
        
        _flag = CGEventGetFlags(event);
        _keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        
        //switch language shortcut
        if (type == kCGEventKeyDown) {
            if (GET_SWITCH_KEY(vSwitchKeyStatus) != _keycode) {
                _lastFlag = 0;
            } else {
                if (handleSwitchKey(GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE)){
                    return NULL;
                }
            }
        } else if (type == kCGEventFlagsChanged) {
            if (_lastFlag == 0 || _lastFlag < _flag) {
                _lastFlag = _flag;
            } else if (_lastFlag > _flag)  {
                //check swith
                if (handleSwitchKey(GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE)) {
                    _lastFlag = 0;
                    return NULL;
                }
                _lastFlag = 0;
            }
        }

        // Also check correct event hooked
        if ((type != kCGEventKeyDown) && (type != kCGEventKeyUp) &&
            (type != kCGEventLeftMouseDown) && (type != kCGEventRightMouseDown) &&
            (type != kCGEventLeftMouseDragged) && (type != kCGEventRightMouseDragged))
            return event;
        
        _proxy = proxy;
        
        //If is in english mode
        if (vLanguage == 0) {
            if (vUseMacro && vUseMacroInEnglishMode && type == kCGEventKeyDown) {
                vEnglishMode((type == kCGEventKeyDown ? vKeyEventState::KeyDown : vKeyEventState::MouseDown),
                             _keycode,
                             (_flag & kCGEventFlagMaskShift) || (_flag & kCGEventFlagMaskAlphaShift),
                             OTHER_CONTROL_KEY);
                
                if (pData->code == vReplaceMaro) { //handle macro in english mode
                    handleMacro();
                    return NULL;
                }
            }
            return event;
        }
        
        //handle mouse
        if (type == kCGEventLeftMouseDown || type == kCGEventRightMouseDown || type == kCGEventLeftMouseDragged || type == kCGEventRightMouseDragged) {
            //send event signal to Engine
            vKeyHandleEvent(vKeyEvent::Mouse, vKeyEventState::MouseDown, 0);
            
            if (IS_DOUBLE_CODE(vCodeTable)) { //VNI
                _syncIndex = 0;
            }
            return event;
        }

        //handle keyboard
        if (type == kCGEventKeyDown) {
            //send event signal to Engine
            vKeyHandleEvent(vKeyEvent::Keyboard,
                            vKeyEventState::KeyDown,
                            _keycode,
                            _flag & kCGEventFlagMaskShift ? 1 : (_flag & kCGEventFlagMaskAlphaShift ? 2 : 0),
                            OTHER_CONTROL_KEY);

            if (pData->code == 0) { //do nothing
                if (IS_DOUBLE_CODE(vCodeTable)) { //VNI
                    if (pData->extCode == 1) {
                        _syncIndex = 0;
                    } else if (pData->extCode == 2) {
                        if (_syncIndex > 0) _syncIndex--;
                    } else if (pData->extCode == 3) {
                        InsertKeyLength(1);
                    }
                }
                return event;
            } else if (pData->code == 1 || pData->code == 3) { //handle result signal
                
                //fix autocomplete
                if (vFixRecommendBrowser) {
                    SendEmptyCharacter();
                    pData->backspaceCount++;
                }
                
                //send backspace
                if (pData->backspaceCount > 0 && pData->backspaceCount < MAX_BUFF) {
                    for (_i = 0; _i < pData->backspaceCount; _i++) {
                        SendBackspace();
                    }
                }
                
                //send new character
                if (!vSendKeyStepByStep) {
                    SendNewCharString();
                } else {
                    if (pData->newCharCount > 0 && pData->newCharCount <= MAX_BUFF) {
                        for (int i = pData->newCharCount - 1; i >= 0; i--) {
                            SendKeyCode(pData->charData[i]);
                        }
                    }
                    if (pData->code == 3) {
                        SendKeyCode(_keycode | ((_flag & kCGEventFlagMaskAlphaShift) || (_flag & kCGEventFlagMaskShift) ? CAPS_MASK : 0));
                    }
                }
            } else if (pData->code == 4) { //MACRO
                handleMacro();
            }
            
            return NULL;
        }
        
        return event;
    }
}
