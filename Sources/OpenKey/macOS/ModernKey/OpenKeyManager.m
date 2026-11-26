//
//  OpenKeyManager.m
//  OpenKey
//
//  Created by Tuyen on 1/27/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import "OpenKeyManager.h"
#import "MJAccessibilityUtils.h"
#import <Carbon/Carbon.h>

extern void OpenKeyInit(void);
extern void ResetInputSession(void);
extern void RefreshInputContext(void);

extern CGEventRef OpenKeyCallback(CGEventTapProxy proxy,
                                  CGEventType type,
                                  CGEventRef event,
                                  void *refcon);

extern NSString* ConvertUtil(NSString* str);

@interface OpenKeyManager ()

@end

@implementation OpenKeyManager {

}
static BOOL _isInited = NO;

static CFMachPortRef      eventTap;
static CGEventMask        eventMask;
static CFRunLoopSourceRef runLoopSource;
static BOOL engineInitialized;
static BOOL userDisabled;
static NSTimer *healthTimer;

+(BOOL)isInited {
    return _isInited;
}

// The tap, timers and engine share the AppKit run loop; no cross-thread engine calls.
+(BOOL)isActive {
    return eventTap && CFMachPortIsValid(eventTap) && CGEventTapIsEnabled(eventTap);
}

+(BOOL)isPaused { return userDisabled; }

+(void)recoverEventTap {
    if (userDisabled || !MJAccessibilityIsEnabled() || IsSecureEventInputEnabled()) return;
    if (eventTap && CFMachPortIsValid(eventTap)) {
        ResetInputSession();
        CGEventTapEnable(eventTap, true);
    }
}

+(void)handleTapDisabledByUser {
    userDisabled = YES;
    ResetInputSession();
}

+(BOOL)resumeEventTap {
    return userDisabled ? NO : [self initEventTap];
}

+(void)prepareEngine {
    if (!engineInitialized) {
        OpenKeyInit();
        engineInitialized = YES;
    }
}

+(BOOL)initEventTap {
    if (!MJAccessibilityIsEnabled()) return NO;
    userDisabled = NO; // Only an explicit initialization/retry clears an intentional pause.
    if (eventTap && CFMachPortIsValid(eventTap)) {
        if (![self isActive]) [self recoverEventTap];
        return [self isActive];
    }
    [self stopEventTap];
    [self prepareEngine];
    eventMask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp) |
                CGEventMaskBit(kCGEventFlagsChanged) | CGEventMaskBit(kCGEventLeftMouseDown) |
                CGEventMaskBit(kCGEventRightMouseDown);
    // Keep the established session tap location while repairing lifecycle/recovery.
    eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
                               kCGEventTapOptionDefault, eventMask, OpenKeyCallback, NULL);
    if (!eventTap) return NO;
    runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    if (!runLoopSource) {
        CFRelease(eventTap);
        eventTap = NULL;
        return NO;
    }
    CFRunLoopAddSource(CFRunLoopGetMain(), runLoopSource, kCFRunLoopCommonModes);
    _isInited = YES;
    ResetInputSession();
    RefreshInputContext();
    CGEventTapEnable(eventTap, true);
    healthTimer = [NSTimer timerWithTimeInterval:1 repeats:YES block:^(NSTimer *timer) {
        RefreshInputContext();
        if (!MJAccessibilityIsEnabled()) {
            [self stopEventTap];
        } else if (!userDisabled && !IsSecureEventInputEnabled()) {
            if (!eventTap || !CFMachPortIsValid(eventTap)) [self initEventTap];
            else if (!CGEventTapIsEnabled(eventTap)) [self recoverEventTap];
        }
    }];
    [[NSRunLoop mainRunLoop] addTimer:healthTimer forMode:NSRunLoopCommonModes];
    return [self isActive];
}

+(BOOL)stopEventTap {
    [healthTimer invalidate];
    healthTimer = nil;
    if (runLoopSource) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(), runLoopSource, kCFRunLoopCommonModes);
        CFRelease(runLoopSource);
        runLoopSource = NULL;
    }
    if (eventTap) {
        CFMachPortInvalidate(eventTap);
        CFRelease(eventTap);
        eventTap = NULL;
    }
    _isInited = NO;
    if (engineInitialized) ResetInputSession();
    return YES;
}

+(NSArray*)getTableCodes {
    return [[NSArray alloc] initWithObjects:
            @"Unicode",
            @"TCVN3 (ABC)",
            @"VNI Windows",
            @"Unicode tổ hợp",
            @"Vietnamese Locale CP 1258", nil];
}

+(NSString*)getBuildDate {
    return [NSString stringWithUTF8String:__DATE__];
}

#pragma mark -Convert feature
+(BOOL)quickConvert {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSString *htmlString = [pasteboard stringForType:NSPasteboardTypeHTML];
    NSString *rawString = [pasteboard stringForType:NSPasteboardTypeString];
    bool converted = false;
    if (htmlString != nil) {
        htmlString = ConvertUtil(htmlString);
        converted = true;
    }
    if (rawString != nil) {
        rawString = ConvertUtil(rawString);
        converted = true;
    }
    if (converted) {
        [pasteboard clearContents];
        if (htmlString != nil)
            [pasteboard setString:htmlString forType:NSPasteboardTypeHTML];
        if (rawString != nil)
            [pasteboard setString:rawString forType:NSPasteboardTypeString];
        
        return YES;
    }
    return NO;
}

+(void)showMessage:(NSWindow*)window message:(NSString*)msg subMsg:(NSString*)subMsg {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:msg];
    [alert setInformativeText:subMsg];
    [alert addButtonWithTitle:@"OK"];
    if (window) {
        [alert beginSheetModalForWindow:window completionHandler:^(NSModalResponse returnCode) {
        }];
    } else {
        [alert runModal];
    }
}

#pragma mark -AutoUpdate feature

+(void)checkNewVersion:(NSWindow*)parent callbackFunc:(CheckNewVersionCallback) callback {
    //load new version config
    NSURLSession *aSession = [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration defaultSessionConfiguration]];
    [[aSession dataTaskWithURL:[NSURL URLWithString:@"https://raw.githubusercontent.com/tuyenvm/OpenKey/master/version.json"] completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        if (((NSHTTPURLResponse *)response).statusCode == 200) {
            if (data) {
                if(NSClassFromString(@"NSJSONSerialization")) {
                    NSError *error = nil;
                    id object = [NSJSONSerialization
                                 JSONObjectWithData:data
                                 options:0
                                 error:&error];
                    
                    if(error) {  }
                    if([object isKindOfClass:[NSDictionary class]]) {
                        NSDictionary *results = object;
                        NSDictionary *ver = [results valueForKey:@"latestVersion"];
                        NSString* versionCodeString = [ver valueForKey:@"versionCode"];
                        int versionCode = (int)[versionCodeString integerValue];
                        int currentVersionCode = (int)[((NSString*)[[NSBundle mainBundle] objectForInfoDictionaryKey: @"CFBundleVersion"]) integerValue];
                        
                        dispatch_async(dispatch_get_main_queue(), ^{
                            if (callback != nil) {
                                callback();
                            }
                            if (versionCode > currentVersionCode || callback != nil) {
                                [self showUpdateMessage:parent needUpdating:versionCode > currentVersionCode newVersion:[ver valueForKey:@"versionName"]];
                            }
                        });
                    }
                    else {
                        //oh my god
                    }
                }
                else {
                    //can not parse json
                }
            }
        }
    }] resume];
}

+(void)showUpdateMessage:(NSWindow*)parent needUpdating:(BOOL)needUpdating newVersion:(NSString*)versionString {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:(needUpdating ? [NSString stringWithFormat:@"OpenKey Có phiên bản mới (%@), bạn có muốn cập nhật không?", versionString] : @"Bạn đang dùng phiên bản mới nhất!")];
    [alert setInformativeText:(needUpdating ? @"Bấm 'Có' để cập nhật OpenKey." : @"")];
    
    if (!needUpdating) {
        [alert addButtonWithTitle:@"OK"];
    } else {
        [alert addButtonWithTitle:@"Có"];
        [alert addButtonWithTitle:@"Không"];
    }
    if (parent == nil) {
        [alert.window makeKeyAndOrderFront:nil];
        [alert.window setLevel:NSStatusWindowLevel];
        NSModalResponse res = [alert runModal];
        if (res == 1000 && needUpdating) {
            [self launchUpdateHelper];
        }
    } else {
        [alert beginSheetModalForWindow:parent completionHandler:^(NSModalResponse returnCode) {
            if (returnCode == 1000 && needUpdating) {
                [self launchUpdateHelper];
            }
        }];
    }
}

+(void)launchUpdateHelper {
    //check update app has exist or not
    NSError *copyError = nil;
    NSString* target = [NSString stringWithFormat:@"%@/OpenKeyUpdate.app", [self getApplicationSupportFolder]];
    [[NSFileManager defaultManager] removeItemAtPath:target error:&copyError];
    if (![[NSFileManager defaultManager] fileExistsAtPath:target]) {
        [[NSFileManager defaultManager] createDirectoryAtPath:[self getApplicationSupportFolder] withIntermediateDirectories:YES attributes:nil error:nil];
        
        if (![[NSFileManager defaultManager] copyItemAtPath:[self getUpdateBundlePath] toPath:target error:&copyError]) {
            NSLog(@"Error on copy");
        }
    }
    
    NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
    NSURL *url = [NSURL fileURLWithPath:[workspace fullPathForApplication:target]];
    NSError *error = nil;
    NSArray *arguments = [NSArray arrayWithObjects: @"yeah", nil];
    [workspace launchApplicationAtURL:url options:0 configuration:[NSDictionary dictionaryWithObject:arguments forKey:NSWorkspaceLaunchConfigurationArguments] error:&error];
    
    [NSApp terminate:0]; //exit main app
}

+(NSString*)getApplicationSupportFolder {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    NSString *applicationSupportDirectory = [paths firstObject];
    return [NSString stringWithFormat:@"%@/OpenKey", applicationSupportDirectory];
}

+(NSString*)getUpdateBundlePath {
    NSString *currentpath = [[NSBundle mainBundle] bundlePath];
    return [NSString stringWithFormat:@"%@/Contents/Library/LoginItems/OpenKeyUpdate.app", currentpath];
}
@end
