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
                if (event.keyCode == kVK_Space || event.keyCode == kVK_Delete || event.keyCode == kVK_ForwardDelete) {
                    [self setStringValue:@"Space"];
                    [self.Parent onSwitchKeyChange:kVK_Space character:kVK_Space];
                } else {
                    [self setStringValue:@""];
                    char t =event.characters.UTF8String[0];
                    [self.Parent onSwitchKeyChange:event.keyCode character:t];
                    return event;
                }
                return (NSEvent*)nil;
            } ];
            
        }
    }
    return okToChange;
}

-(void) textDidEndEditing:(NSNotification *)notification {
    [NSEvent removeMonitor:eventMonitor];
    eventMonitor = nil;
}

-(void)setTextByChar:(unsigned short)chr {
    if (chr == kVK_Space) {
        [self setStringValue:@"Space"];
    } else {
        NSString* str = [NSString stringWithFormat:@"%c", chr];
        [self setStringValue:str];
    }
}
@end
