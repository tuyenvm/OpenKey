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
#import "AppDelegate.h"

#import "ViewController.h"
extern ViewController* viewController;

extern AppDelegate* appDelegate;

extern "C" {
    NSArray* _specialApp = @[@"com.google.Chrome",
                             @"com.apple.Safari",
                             @"org.mozilla.firefox",
                             @"com.jetbrains.rider",
                             @"com.barebones.textwrangler"];
    
    CGEventSourceRef myEventSource = NULL;
    vKeyHookState* pData;
    CGEventRef eventBackSpaceDown;
    CGEventRef eventBackSpaceUp;
    UniChar _newChar, _newCharHi;
    CGEventRef _newEventDown;
    CGKeyCode _keycode;
    CGEventFlags _flag, _privateFlag;
    
    Uint16 _syncKey[MAX_BUFF];
    Uint8 _syncIndex = 0, _i = 0;
    Uint16 _uniChar[2];
    
    void OpenKeyInit() {
        memset(&_syncKey, 0, sizeof(_syncKey));
        myEventSource = CGEventSourceCreate(kCGEventSourceStatePrivate);
        pData = (vKeyHookState*)vKeyInit();

        eventBackSpaceDown = CGEventCreateKeyboardEvent (myEventSource, 51, true);
        eventBackSpaceUp = CGEventCreateKeyboardEvent (myEventSource, 51, false);
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
        //sleep(1);
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
        //sleep(1);
        if (IS_DOUBLE_CODE(vCodeTable)) //VNI or Unicode Compound
            InsertKeyLength(1);
        if ([_specialApp containsObject:[[NSWorkspace sharedWorkspace] frontmostApplication].bundleIdentifier]) {
            _newChar = 0x202F;
        } else {
            _newChar = 0x200C; //Unicode character with empty space
        }
        //_newChar = 0x2060;
        
        _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
        CGEventKeyboardSetUnicodeString(_newEventDown, 1, &_newChar);
        CGEventPost(kCGHIDEventTap, _newEventDown);
        CFRelease(_newEventDown);
    }

    void SendBackspace() {
        //sleep(1);
        CGEventPost(kCGSessionEventTap, eventBackSpaceDown);
        CGEventPost(kCGSessionEventTap, eventBackSpaceUp);
        
        if (IS_DOUBLE_CODE(vCodeTable)) { //VNI or Unicode Compound
            _syncIndex--;
            if (_syncKey[_syncIndex] > 1) {
                CGEventPost(kCGSessionEventTap, eventBackSpaceDown);
                CGEventPost(kCGSessionEventTap, eventBackSpaceUp);
            }
        }
    }
    
    bool canSwitchKey() {
        if (HAS_CONTROL(vSwitchKeyStatus) && !(_flag & kCGEventFlagMaskControl))
            return false;
        if (HAS_OPTION(vSwitchKeyStatus) && !(_flag & kCGEventFlagMaskAlternate))
            return false;
        if (HAS_COMMAND(vSwitchKeyStatus) && !(_flag & kCGEventFlagMaskCommand))
            return false;
        if (GET_SWITCH_KEY(vSwitchKeyStatus) != _keycode)
            return false;
        return true;
    }
    
    /*MAIN Callback*/
    CGEventRef OpenKeyCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
        //NSLog(@"%@", [[NSWorkspace sharedWorkspace] frontmostApplication].bundleIdentifier);
        _flag = CGEventGetFlags(event);
        _keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        
        //switch language shortcut
        if ((type == kCGEventKeyDown) && canSwitchKey()) {
            if (vLanguage == 0)
                vLanguage = 1;
            else
                vLanguage = 0;
            NSBeep();
            [appDelegate onInputMethodSelected];
            
            startNewSession();
            
            return NULL;
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
                SendEmptyCharacter();
                pData->backspaceCount++;
   
                //send backspace
                if (pData->backspaceCount > 0 && pData->backspaceCount <= 7) {
                    for (int i = 0; i < pData->backspaceCount; i++) {
                        SendBackspace();
                    }
                }
                
                //send new character
                if (pData->newCharCount > 0 && pData->newCharCount <= 8) {
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
