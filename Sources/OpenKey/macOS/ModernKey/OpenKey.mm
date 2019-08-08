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

extern ViewController* viewController;

extern AppDelegate* appDelegate;

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
    
    Uint16 _syncKey[MAX_BUFF];
    Uint8 _syncIndex = 0, _i = 0;
    Uint16 _uniChar[2];
    
    void OpenKeyInit() {
        memset(&_syncKey, 0, sizeof(_syncKey));
        myEventSource = CGEventSourceCreate(kCGEventSourceStatePrivate);
        pData = (vKeyHookState*)vKeyInit();

        eventBackSpaceDown = CGEventCreateKeyboardEvent (myEventSource, 51, true);
        eventBackSpaceUp = CGEventCreateKeyboardEvent (myEventSource, 51, false);
        
        //init macro feature
        //test
       /* Byte test[] = {1, 0, 2, KEY_K, KEY_O, 2, 'k', 'o', 5, 0, 'k', 'h', 'o', 'n', 'g'};
        NSData* data = [NSData dataWithBytes:test length:15];
        NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
        [prefs setObject:data forKey:@"macroData"];
        
        NSData *data2 = [prefs objectForKey:@"macroData"];
        //memcpy(&playfield, data.bytes, data.length);
        initMacroMap((Byte*)data2.bytes, data2.length);*/
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
            CGEventSetFlags(_newEventDown, _privateFlag);
            CGEventPost(kCGSessionEventTap, _newEventDown);
        } else {
            if (vCodeTable == 0) { //unicode 2 bytes code
                _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
                CGEventKeyboardSetUnicodeString(_newEventDown, 1, &_newChar);
                CGEventPost(kCGHIDEventTap, _newEventDown);
            } else if (vCodeTable == 1 || vCodeTable == 2 || vCodeTable == 4) { //others such as VNI Windows, TCVN3: 1 byte code
                _newCharHi = HIBYTE(_newChar);
                _newChar = LOBYTE(_newChar);
                
                _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
                CGEventKeyboardSetUnicodeString(_newEventDown, 1, &_newChar);
                CGEventPost(kCGHIDEventTap, _newEventDown);
                
                if (_newCharHi > 32) {
                    if (vCodeTable == 2) //VNI
                        InsertKeyLength(2);
                    _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
                    CGEventKeyboardSetUnicodeString(_newEventDown, 1, &_newCharHi);
                    CGEventPost(kCGHIDEventTap, _newEventDown);
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
                CGEventPost(kCGHIDEventTap, _newEventDown);
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
        CGEventPost(kCGHIDEventTap, _newEventDown);
        CFRelease(_newEventDown);
    }
    
    void SendVirtualKey(const Byte& vKey) {
        CGEventRef eventVkeyDown = CGEventCreateKeyboardEvent (myEventSource, vKey, true);
        CGEventRef eventVkeyUp = CGEventCreateKeyboardEvent (myEventSource, vKey, false);
        
        CGEventPost(kCGSessionEventTap, eventVkeyDown);
        CGEventPost(kCGSessionEventTap, eventVkeyUp);
        
        CFRelease(eventVkeyDown);
        CFRelease(eventVkeyUp);
    }

    void SendBackspace() {
        CGEventPost(kCGSessionEventTap, eventBackSpaceDown);
        CGEventPost(kCGSessionEventTap, eventBackSpaceUp);
        
        if (IS_DOUBLE_CODE(vCodeTable)) { //VNI or Unicode Compound
            _syncIndex--;
            if (_syncKey[_syncIndex] > 1) {
                if (!(vCodeTable == 3 && [_unicodeCompoundApp containsObject:FRONT_APP])) {
                    CGEventPost(kCGSessionEventTap, eventBackSpaceDown);
                    CGEventPost(kCGSessionEventTap, eventBackSpaceUp);
                }
            }
        }
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

    /*MAIN Callback*/
    CGEventRef OpenKeyCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
        //NSLog(@"%@", FRONT_APP);
        _flag = CGEventGetFlags(event);
        _keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        
        //switch language shortcut
        if (type == kCGEventKeyDown) {
            if (GET_SWITCH_KEY(vSwitchKeyStatus) != _keycode) {
                _lastFlag = 0;
            } else {
                if (handleSwitchKey(GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE))
                    return NULL;
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
        
        if (vLanguage == 0) //ignore if is english
            return event;
        
        // Also check correct event hooked
        if ((type != kCGEventKeyDown) && (type != kCGEventKeyUp) &&
            (type != kCGEventLeftMouseDown) && (type != kCGEventRightMouseDown) &&
            (type != kCGEventLeftMouseDragged) && (type != kCGEventRightMouseDragged))
            return event;
        
        //handle mouse
        if (type == kCGEventLeftMouseDown || type == kCGEventRightMouseDown || type == kCGEventLeftMouseDragged || type == kCGEventRightMouseDragged) {
            //send event signal to Engine
            vKeyHandleEvent(vKeyEvent::Mouse, vKeyEventState::MouseDown, 0);
            
            if (IS_DOUBLE_CODE(vCodeTable)) { //VNI
                _syncIndex = 0;
            }
            return event;
        }
        
        //dont handle my event
        if (CGEventGetIntegerValueField(event, kCGEventSourceStateID) == CGEventSourceGetSourceStateID(myEventSource)) {
            return event;
        }

        //handle keyboard
        if (type == kCGEventKeyDown) {
            //send event signal to Engine
            vKeyHandleEvent(vKeyEvent::Keyboard,
                            vKeyEventState::KeyDown,
                            _keycode,
                            _flag & kCGEventFlagMaskShift ? 1 : (_flag & kCGEventFlagMaskAlphaShift ? 2 : 0),
                                (_flag & kCGEventFlagMaskCommand) || (_flag & kCGEventFlagMaskControl) ||
                                (_flag & kCGEventFlagMaskAlternate) || (_flag & kCGEventFlagMaskSecondaryFn) ||
                                (_flag & kCGEventFlagMaskNumericPad) || (_flag & kCGEventFlagMaskHelp));

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
                    for (int i = 0; i < pData->backspaceCount; i++) {
                        SendBackspace();
                    }
                }
                
                //send new character
                if (pData->newCharCount > 0 && pData->newCharCount <= MAX_BUFF) {
                    for (int i = pData->newCharCount - 1; i >= 0; i--) {
                        SendKeyCode(pData->charData[i]);
                    }
                }
                if (pData->code == 3) {
                    SendKeyCode(_keycode | ((_flag & kCGEventFlagMaskAlphaShift) || (_flag & kCGEventFlagMaskShift) ? CAPS_MASK : 0));
                }
            }
            
            return NULL;
        }
        
        return event;
    }
}
