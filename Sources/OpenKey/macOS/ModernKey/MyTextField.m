//
//  MyTextField.m
//  OpenKey
//
//  Created by Tuyen on 7/30/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import "MyTextField.h"
#include <Carbon/Carbon.h>

@implementation MyTextField {
    id eventMonitor;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    // Drawing code here.
}

- (BOOL)becomeFirstResponder {
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
