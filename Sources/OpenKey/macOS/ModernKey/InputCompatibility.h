#pragma once
#import <Foundation/Foundation.h>

static inline BOOL OKIsTerminalApp(NSString *bundleID) {
    return [@[@"com.apple.Terminal", @"com.googlecode.iterm2", @"com.mitchellh.ghostty",
              @"dev.warp.Warp-Stable"] containsObject:bundleID];
}

static inline BOOL OKTargetsSpotlight(pid_t targetPID, pid_t spotlightPID, NSString *frontApp) {
    // A fading window is not evidence that Spotlight owns the keystroke.
    return targetPID > 0 ? spotlightPID > 0 && targetPID == spotlightPID :
                          [frontApp isEqualToString:@"com.apple.Spotlight"];
}
