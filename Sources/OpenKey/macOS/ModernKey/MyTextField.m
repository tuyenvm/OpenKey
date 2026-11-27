//
//  MyTextField.m
//  OpenKey
//
//  Created by Tuyen on 7/30/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import "MyTextField.h"
#include <Carbon/Carbon.h>

extern void SetShortcutRecording(BOOL recording);

@implementation MyTextField {
    int _displayedShortcut;
    id eventMonitor;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    // Drawing code here.
}

- (BOOL)becomeFirstResponder {
    if (self.recordsShortcut) {
        self.stringValue=@"Nhấn tổ hợp phím…"; SetShortcutRecording(YES); return YES;
    }
    BOOL okToChange = [super becomeFirstResponder];
    if (okToChange) {
        [self setKeyboardFocusRingNeedsDisplayInRect: [self bounds]];
        
        if (!eventMonitor) {
            eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSKeyDownMask handler:^(NSEvent *event) {
                self.LastKeyCode = event.keyCode;
                self.LastKeyChar = event.characters.UTF8String[0];
                return event;
            } ];
            
        }
    }
    return okToChange;
}

- (BOOL)accessibilityPerformPress {
    if (self.recordsShortcut) return [self.window makeFirstResponder:self];
    return [super accessibilityPerformPress];
}
- (BOOL)acceptsFirstResponder { return self.recordsShortcut || [super acceptsFirstResponder]; }
- (void)mouseDown:(NSEvent *)event {
    if (self.recordsShortcut) [self.window makeFirstResponder:self]; else [super mouseDown:event];
}
- (BOOL)resignFirstResponder {
    if (self.recordsShortcut) { SetShortcutRecording(NO); [self displayShortcut:_displayedShortcut]; return YES; }
    return [super resignFirstResponder];
}
- (BOOL)performKeyEquivalent:(NSEvent *)event {
    if (self.recordsShortcut && self.window.firstResponder==self) { [self keyDown:event]; return YES; }
    return [super performKeyEquivalent:event];
}
- (void)displayShortcut:(int)shortcut {
    _displayedShortcut=shortcut;
    if ((shortcut & ~0x8000)==0xFE0000FE) { self.stringValue=@"Không đặt"; return; }
    NSMutableString *label=[NSMutableString new];
    for (NSInteger i=0;i<4;i++) if (shortcut & (1<<(8+i))) [label appendString:@[@"⌃",@"⌥",@"⌘",@"⇧"][i]];
    unsigned int character=((unsigned int)shortcut>>24)&0xFF;
    [label appendString:(shortcut&0xFF)==kVK_Space ? @"Space" : [[NSString stringWithFormat:@"%c",character] uppercaseString]];
    self.stringValue=label;
}
- (void)keyDown:(NSEvent *)event {
    if (!self.recordsShortcut) { [super keyDown:event]; return; }
    if (event.keyCode==kVK_Escape) { [self.window makeFirstResponder:nil]; return; }
    int shortcut=0xFE0000FE;
    if (event.keyCode!=kVK_Delete && event.keyCode!=kVK_ForwardDelete) {
        NSEventModifierFlags flags=event.modifierFlags;
        if (!(flags & (NSEventModifierFlagControl|NSEventModifierFlagOption|NSEventModifierFlagCommand))) { NSBeep(); return; }
        NSString *characters=event.charactersIgnoringModifiers;
        if (!characters.length || [characters characterAtIndex:0]>127) { NSBeep(); return; }
        shortcut=event.keyCode | ((unsigned int)[characters characterAtIndex:0]<<24);
        if (flags & NSEventModifierFlagControl) shortcut|=1<<8;
        if (flags & NSEventModifierFlagOption) shortcut|=1<<9;
        if (flags & NSEventModifierFlagCommand) shortcut|=1<<10;
        if (flags & NSEventModifierFlagShift) shortcut|=1<<11;
    }
    [self.Parent onShortcutRecorded:shortcut]; [self.window makeFirstResponder:nil]; [self displayShortcut:shortcut];
}

-(void) textDidEndEditing:(NSNotification *)notification {
    [NSEvent removeMonitor:eventMonitor];
    eventMonitor = nil;
}

- (void)textDidChange:(NSNotification *)notification {
    if (self.LastKeyCode == kVK_Space) {
        [self setStringValue:@"Space"];
        [self.Parent onMyTextFieldKeyChange:kVK_Space character:kVK_Space];
    } else if (self.LastKeyCode == kVK_Delete || self.LastKeyCode == kVK_ForwardDelete) {
        [self setStringValue:@""];
        [self.Parent onMyTextFieldKeyChange:0xFE character:0xFE];
    } else {
        [self setStringValue:@""];
        [self.Parent onMyTextFieldKeyChange:self.LastKeyCode character:self.LastKeyChar];
        [self setStringValue:[NSString stringWithFormat:@"%c", self.LastKeyChar]];
    }
}

-(void)setTextByChar:(unsigned short)chr {
    if (chr == kVK_Space) {
        [self setStringValue:@"Space"];
    } else if (chr == 0xFE) {
        [self setStringValue:@""];
    } else {
        NSString* str = [NSString stringWithFormat:@"%c", chr];
        [self setStringValue:str];
    }
}
@end
