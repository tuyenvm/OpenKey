//
//  MJAccessibilityUtils.m
//  OpenKey
//
//  Created by Nguyen Tan Thong on 18/9/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
//  Source: https://github.com/Hammerspoon/hammerspoon/blob/master/Hammerspoon/MJAccessibilityUtils.m
//  License: MIT


#import "MJAccessibilityUtils.h"
#import <AppKit/AppKit.h>
// #import "HSLogger.h"

extern Boolean AXAPIEnabled(void);
extern Boolean AXIsProcessTrustedWithOptions(CFDictionaryRef options) __attribute__((weak_import));
extern CFStringRef kAXTrustedCheckOptionPrompt __attribute__((weak_import));


BOOL MJAccessibilityIsEnabled(void) {
    BOOL isEnabled = NO;
    if (AXIsProcessTrustedWithOptions != NULL)
        isEnabled = AXIsProcessTrustedWithOptions(NULL);
    else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        isEnabled = AXAPIEnabled();
#pragma clang diagnostic pop

//    HSNSLOG(@"Accessibility is: %@", isEnabled ? @"ENABLED" : @"DISABLED");
    return isEnabled;
}

void MJAccessibilityOpenPanel(void) {
    AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)@{(__bridge id)kAXTrustedCheckOptionPrompt: @YES});
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"]];
}
