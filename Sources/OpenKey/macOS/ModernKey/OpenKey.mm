//
//  OpenKey.m
//  OpenKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <Foundation/Foundation.h>
#import <ApplicationServices/ApplicationServices.h>
#import <mach/mach_time.h>
#import <unistd.h>
#import <unordered_map>
#import <vector>
#if __has_include("Engine.h")
#import "Engine.h"
#elif __has_include("../../engine/Engine.h")
#import "../../engine/Engine.h"
#endif
#import "AppDelegate.h"
#import "ViewController.h"

#define FRONT_APP [[NSWorkspace sharedWorkspace] frontmostApplication].bundleIdentifier
#define OTHER_CONTROL_KEY (_flag & kCGEventFlagMaskCommand) || (_flag & kCGEventFlagMaskControl) || \
                            (_flag & kCGEventFlagMaskAlternate) || (_flag & kCGEventFlagMaskSecondaryFn) || \
                            (_flag & kCGEventFlagMaskNumericPad) || (_flag & kCGEventFlagMaskHelp)

#define DYNA_DATA(macro, pos) (macro ? pData->macroData[pos] : pData->charData[pos])
#define MAX_UNICODE_STRING  20
#define EMPTY_HOTKEY 0xFE0000FE
#define LOAD_DATA(VAR, KEY) VAR = (int)[[NSUserDefaults standardUserDefaults] integerForKey:@#KEY]

// Ignore code for Modifier keys and numpad
// Reference: https://eastmanreference.com/complete-list-of-applescript-key-codes
NSDictionary *keyStringToKeyCodeMap = @{
    // Characters from number row
    @"`": @50, @"~": @50, @"1": @18, @"!": @18, @"2": @19, @"@": @19, @"3": @20, @"#": @20, @"4": @21, @"$": @21,
    @"5": @23, @"%": @23, @"6": @22, @"^": @22, @"7": @26, @"&": @26, @"8": @28, @"*": @28, @"9": @25, @"(": @25,
    @"0": @29, @")": @29, @"-": @27, @"_": @27, @"=": @24, @"+": @24,
    // Characters from first keyboard row
    @"q": @12, @"w": @13, @"e": @14, @"r": @15, @"t": @17, @"y": @16, @"u": @32, @"i": @34, @"o": @31, @"p": @35,
    @"[": @33, @"{": @33, @"]": @30, @"}": @30, @"\\": @42, @"|": @42,
    // Characters from second keyboard row
    @"a": @0, @"s": @1, @"d": @2, @"f": @3, @"g": @5, @"h": @4, @"j": @38, @"k": @40, @"l": @37,
    @";": @41, @":": @41, @"'": @39, @"\"": @39,
    // Characters from second third row
    @"z": @6, @"x": @7, @"c": @8, @"v": @9, @"b": @11, @"n": @45, @"m": @46,
    @",": @43, @"<": @43, @".": @47, @">": @47, @"/": @44, @"?": @44
};

extern ViewController* viewController;

extern AppDelegate* appDelegate;
extern int vSendKeyStepByStep;
extern int vFixChromiumBrowser;
extern int vPerformLayoutCompat;

extern "C" {
    //app which must sent special empty character
    NSArray* _niceSpaceApp = @[@"com.sublimetext.3",
                               @"com.sublimetext.2",
                             ];
    
    //app which error with unicode Compound
    NSArray* _unicodeCompoundApp = @[@"com.apple.",
                                     @"com.google.Chrome", @"com.brave.Browser",
                                     @"com.microsoft.edgemac.Dev", @"com.microsoft.edgemac.Beta", @"com.microsoft.Edge.Dev", @"com.microsoft.Edge"];
    NSArray* _recommendWorkaroundDisabledApp = @[@"com.apple.Spotlight"];
    
    CGEventSourceRef myEventSource = NULL;
    vKeyHookState* pData;
    CGEventRef eventBackSpaceDown;
    CGEventRef eventBackSpaceUp;
    UniChar _newChar, _newCharHi;
    CGEventRef _newEventDown, _newEventUp;
    CGKeyCode _keycode;
    CGEventFlags _flag, _lastFlag = 0, _privateFlag;
    CGEventTapProxy _proxy;
    
    Uint16 _newCharString[MAX_UNICODE_STRING];
    Uint16 _newCharSize;
    bool _willContinuteSending = false;
    bool _willSendControlKey = false;
    
    vector<Uint16> _syncKey;
    
    Uint16 _uniChar[2];
    int _i, _j, _k;
    Uint32 _tempChar;
    bool _hasJustUsedHotKey = false;

    int _languageTemp = 0; //use for smart switch key
    vector<Byte> savedSmartSwitchKeyData; ////use for smart switch key
    
    NSString* _frontMostApp = @"UnknownApp";
    void initReverseCharMap(void);
    
    void OpenKeyInit() {
        //load saved data
        vFreeMark = 0;//(int)[[NSUserDefaults standardUserDefaults] integerForKey:@"FreeMark"];
        LOAD_DATA(vCodeTable, CodeTable); if (vCodeTable < 0) vCodeTable = 0;
        LOAD_DATA(vCheckSpelling, Spelling);
        LOAD_DATA(vQuickTelex, QuickTelex);
        LOAD_DATA(vUseModernOrthography, ModernOrthography);
        LOAD_DATA(vRestoreIfWrongSpelling, RestoreIfInvalidWord);
        LOAD_DATA(vFixRecommendBrowser, FixRecommendBrowser);
        LOAD_DATA(vUseMacro, UseMacro);
        LOAD_DATA(vUseMacroInEnglishMode, UseMacroInEnglishMode);
        LOAD_DATA(vAutoCapsMacro, vAutoCapsMacro);
        LOAD_DATA(vSendKeyStepByStep, SendKeyStepByStep);
        LOAD_DATA(vUseSmartSwitchKey, UseSmartSwitchKey);
        LOAD_DATA(vUpperCaseFirstChar, UpperCaseFirstChar);
        
        LOAD_DATA(vTempOffSpelling, vTempOffSpelling);
        LOAD_DATA(vAllowConsonantZFWJ, vAllowConsonantZFWJ);
        LOAD_DATA(vQuickEndConsonant, vQuickEndConsonant);
        LOAD_DATA(vQuickStartConsonant, vQuickStartConsonant);
        LOAD_DATA(vRememberCode, vRememberCode);
        LOAD_DATA(vOtherLanguage, vOtherLanguage);
        LOAD_DATA(vTempOffOpenKey, vTempOffOpenKey);
        
        LOAD_DATA(vFixChromiumBrowser, vFixChromiumBrowser);
        
        LOAD_DATA(vPerformLayoutCompat, vPerformLayoutCompat);
        
        myEventSource = CGEventSourceCreate(kCGEventSourceStatePrivate);
        pData = (vKeyHookState*)vKeyInit();

        eventBackSpaceDown = CGEventCreateKeyboardEvent (myEventSource, 51, true);
        eventBackSpaceUp = CGEventCreateKeyboardEvent (myEventSource, 51, false);
        
        //init and load macro data
        NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
        NSData *data = [prefs objectForKey:@"macroData"];
        initMacroMap((Byte*)data.bytes, (int)data.length);
        
        //init and load smart switch key data
        data = [prefs objectForKey:@"smartSwitchKey"];
        initSmartSwitchKey((Byte*)data.bytes, (int)data.length);
        
        //init convert tool
        convertToolDontAlertWhenCompleted = ![prefs boolForKey:@"convertToolDontAlertWhenCompleted"];
        convertToolToAllCaps = [prefs boolForKey:@"convertToolToAllCaps"];
        convertToolToAllNonCaps = [prefs boolForKey:@"convertToolToAllNonCaps"];
        convertToolToCapsFirstLetter = [prefs boolForKey:@"convertToolToCapsFirstLetter"];
        convertToolToCapsEachWord = [prefs boolForKey:@"convertToolToCapsEachWord"];
        convertToolRemoveMark = [prefs boolForKey:@"convertToolRemoveMark"];
        convertToolFromCode = [prefs integerForKey:@"convertToolFromCode"];
        convertToolToCode = [prefs integerForKey:@"convertToolToCode"];
        convertToolHotKey = (int)[prefs integerForKey:@"convertToolHotKey"];
        if (convertToolHotKey == 0) {
            convertToolHotKey = EMPTY_HOTKEY;
        }

        initReverseCharMap();
    }
    
    void RequestNewSession() {
        //send event signal to Engine
        vKeyHandleEvent(vKeyEvent::Mouse, vKeyEventState::MouseDown, 0);
        
        if (IS_DOUBLE_CODE(vCodeTable)) { //VNI
            _syncKey.clear();
        }
    }
    
    void queryFrontMostApp() {
        if ([[[NSWorkspace sharedWorkspace] frontmostApplication].bundleIdentifier compare:OPENKEY_BUNDLE] != 0) {
            _frontMostApp = [[NSWorkspace sharedWorkspace] frontmostApplication].bundleIdentifier;
            if (_frontMostApp == nil)
                _frontMostApp = [[NSWorkspace sharedWorkspace] frontmostApplication].localizedName != nil ?
                [[NSWorkspace sharedWorkspace] frontmostApplication].localizedName : @"UnknownApp";
        }
    }
    
    NSString* ConvertUtil(NSString* str) {
        return [NSString stringWithUTF8String:convertUtil([str UTF8String]).c_str()];
    }
    
    BOOL containUnicodeCompoundApp(NSString* topApp) {
        if (topApp == nil) return false;
        for (_j = 0; _j < [_unicodeCompoundApp count]; _j++) {
            if ([topApp hasPrefix:[_unicodeCompoundApp objectAtIndex:_j]] || [[_unicodeCompoundApp objectAtIndex:_j] isEqualToString:topApp])
                return true;
        }
        return false;
    }

    BOOL isSpotlightVisible() {
        NSArray *windows = CFBridgingRelease(CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
                                                                        kCGNullWindowID));
        for (NSDictionary *window in windows) {
            if ([[window objectForKey:(__bridge NSString *)kCGWindowOwnerName] isEqualToString:@"Spotlight"]) {
                return true;
            }
        }
        return false;
    }

    BOOL isTerminalApp(NSString* topApp) {
        if (!topApp || topApp.length == 0) return NO;
        static NSSet *terminalApps = nil;
        if (!terminalApps) {
            terminalApps = [[NSSet alloc] initWithArray:@[
                @"com.apple.Terminal",
                @"com.googlecode.iterm2",
                @"net.kovidgoyal.kitty",
                @"alacritty",
                @"io.alacritty",
                @"org.alacritty",
                @"dev.warp.Warp-Stable",
                @"dev.warp.Warp",
                @"com.mitchellh.ghostty",
                @"com.github.wez.wezterm",
                @"co.zeit.hyper",
                @"org.tabby"
            ]];
        }
        if ([terminalApps containsObject:topApp]) return YES;
        NSString *lower = [topApp lowercaseString];
        if ([lower containsString:@"terminal"] || 
            [lower containsString:@"iterm"] || 
            [lower containsString:@"ghostty"] || 
            [lower containsString:@"alacritty"] || 
            [lower containsString:@"kitty"] || 
            [lower containsString:@"wezterm"] || 
            [lower containsString:@"warp"]) {
            return YES;
        }
        return NO;
    }

    BOOL isBrowserApp(NSString* topApp) {
        if (!topApp || topApp.length == 0) return NO;
        static NSSet *browserApps = nil;
        if (!browserApps) {
            browserApps = [[NSSet alloc] initWithArray:@[
                @"com.google.Chrome",
                @"com.google.Chrome.canary",
                @"com.apple.Safari",
                @"com.apple.SafariTechnologyPreview",
                @"com.brave.Browser",
                @"com.microsoft.edgemac",
                @"com.microsoft.edgemac.Dev",
                @"com.microsoft.edgemac.Beta",
                @"com.microsoft.Edge",
                @"org.mozilla.firefox",
                @"com.operasoftware.Opera",
                @"com.vivaldi.Vivaldi",
                @"company.thebrowser.Browser"
            ]];
        }
        if ([browserApps containsObject:topApp]) return YES;
        NSString *lower = [topApp lowercaseString];
        return [lower containsString:@"chrome"] || 
               [lower containsString:@"browser"] || 
               [lower containsString:@"safari"] || 
               [lower containsString:@"firefox"] ||
               [lower containsString:@"edge"];
    }

    BOOL shouldUseRecommendWorkaround(NSString* topApp) {
        if (!vFixRecommendBrowser) return false;
        if (isSpotlightVisible()) return false;
        if (topApp == nil) return false;
        if (isTerminalApp(topApp)) return false;
        if (!isBrowserApp(topApp)) return false;
        return ![_recommendWorkaroundDisabledApp containsObject:topApp];
    }

    BOOL shouldUseSelectionReplacement(NSString* topApp) {
        if (isTerminalApp(topApp)) return false;
        return isSpotlightVisible() || [_recommendWorkaroundDisabledApp containsObject:topApp];
    }
    
    std::unordered_map<unichar, Uint32> _reverseCharMap;

    void initReverseCharMap() {
        _reverseCharMap.clear();
        Uint16 keyMap[26] = {
            KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I,
            KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R,
            KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z
        };
        for (int i = 0; i < 26; i++) {
            _reverseCharMap['a' + i] = keyMap[i];
            _reverseCharMap['A' + i] = keyMap[i] | CAPS_MASK;
        }

        _reverseCharMap[0x0110] = KEY_D | TONE_MASK | CAPS_MASK; // Đ
        _reverseCharMap[0x0111] = KEY_D | TONE_MASK;             // đ

        auto reg = [](unichar ch, Uint32 val) {
            if (ch != 0) _reverseCharMap[ch] = val;
        };

        Uint32 markMasks[5] = {MARK1_MASK, MARK2_MASK, MARK3_MASK, MARK4_MASK, MARK5_MASK};

        // A
        unichar aBasic[14] = {0x00C2, 0x00E2, 0x0102, 0x0103, 0x00C1, 0x00E1, 0x00C0, 0x00E0, 0x1EA2, 0x1EA3, 0x00C3, 0x00E3, 0x1EA0, 0x1EA1};
        reg(aBasic[0], KEY_A | TONE_MASK | CAPS_MASK); reg(aBasic[1], KEY_A | TONE_MASK);
        reg(aBasic[2], KEY_A | TONEW_MASK | CAPS_MASK); reg(aBasic[3], KEY_A | TONEW_MASK);
        for (int m = 0; m < 5; m++) {
            reg(aBasic[4 + m*2], KEY_A | markMasks[m] | CAPS_MASK);
            reg(aBasic[5 + m*2], KEY_A | markMasks[m]);
        }

        // A with TONE_MASK (Â: Ấ, ấ, Ầ, ầ, Ẩ, ẩ, Ẫ, ẫ, Ậ, ậ)
        unichar aTone[10] = {0x1EA4, 0x1EA5, 0x1EA6, 0x1EA7, 0x1EA8, 0x1EA9, 0x1EAA, 0x1EAB, 0x1EAC, 0x1EAD};
        for (int m = 0; m < 5; m++) {
            reg(aTone[m*2], KEY_A | TONE_MASK | markMasks[m] | CAPS_MASK);
            reg(aTone[m*2 + 1], KEY_A | TONE_MASK | markMasks[m]);
        }

        // A with TONEW_MASK (Ă: Ắ, ắ, Ằ, ằ, Ẳ, ẳ, Ẵ, ẵ, Ặ, ặ)
        unichar aToneW[10] = {0x1EAE, 0x1EAF, 0x1EB0, 0x1EB1, 0x1EB2, 0x1EB3, 0x1EB4, 0x1EB5, 0x1EB6, 0x1EB7};
        for (int m = 0; m < 5; m++) {
            reg(aToneW[m*2], KEY_A | TONEW_MASK | markMasks[m] | CAPS_MASK);
            reg(aToneW[m*2 + 1], KEY_A | TONEW_MASK | markMasks[m]);
        }

        // E
        unichar eBasic[14] = {0x00CA, 0x00EA, 0, 0, 0x00C9, 0x00E9, 0x00C8, 0x00E8, 0x1EBA, 0x1EBB, 0x1EBC, 0x1EBD, 0x1EB8, 0x1EB9};
        reg(eBasic[0], KEY_E | TONE_MASK | CAPS_MASK); reg(eBasic[1], KEY_E | TONE_MASK);
        for (int m = 0; m < 5; m++) {
            reg(eBasic[4 + m*2], KEY_E | markMasks[m] | CAPS_MASK);
            reg(eBasic[5 + m*2], KEY_E | markMasks[m]);
        }
        // E with TONE_MASK (Ê: Ế, ế, Ề, ề, Ể, ể, Ễ, ễ, Ệ, ệ)
        unichar eTone[10] = {0x1EBE, 0x1EBF, 0x1EC0, 0x1EC1, 0x1EC2, 0x1EC3, 0x1EC4, 0x1EC5, 0x1EC6, 0x1EC7};
        for (int m = 0; m < 5; m++) {
            reg(eTone[m*2], KEY_E | TONE_MASK | markMasks[m] | CAPS_MASK);
            reg(eTone[m*2 + 1], KEY_E | TONE_MASK | markMasks[m]);
        }

        // O
        unichar oBasic[14] = {0x00D4, 0x00F4, 0x01A0, 0x01A1, 0x00D3, 0x00F3, 0x00D2, 0x00F2, 0x1ECE, 0x1ECF, 0x00D5, 0x00F5, 0x1ECC, 0x1ECD};
        reg(oBasic[0], KEY_O | TONE_MASK | CAPS_MASK); reg(oBasic[1], KEY_O | TONE_MASK);
        reg(oBasic[2], KEY_O | TONEW_MASK | CAPS_MASK); reg(oBasic[3], KEY_O | TONEW_MASK);
        for (int m = 0; m < 5; m++) {
            reg(oBasic[4 + m*2], KEY_O | markMasks[m] | CAPS_MASK);
            reg(oBasic[5 + m*2], KEY_O | markMasks[m]);
        }
        // O with TONE_MASK (Ô: Ố, ố, Ồ, ồ, Ổ, ổ, Ỗ, ỗ, Ộ, ộ)
        unichar oTone[10] = {0x1ED0, 0x1ED1, 0x1ED2, 0x1ED3, 0x1ED4, 0x1ED5, 0x1ED6, 0x1ED7, 0x1ED8, 0x1ED9};
        for (int m = 0; m < 5; m++) {
            reg(oTone[m*2], KEY_O | TONE_MASK | markMasks[m] | CAPS_MASK);
            reg(oTone[m*2 + 1], KEY_O | TONE_MASK | markMasks[m]);
        }
        // O with TONEW_MASK (Ơ: Ớ, ớ, Ờ, ờ, Ở, ở, Ỡ, ỡ, Ợ, ợ)
        unichar oToneW[10] = {0x1EDA, 0x1EDB, 0x1EDC, 0x1EDD, 0x1EDE, 0x1EDF, 0x1EE0, 0x1EE1, 0x1EE2, 0x1EE3};
        for (int m = 0; m < 5; m++) {
            reg(oToneW[m*2], KEY_O | TONEW_MASK | markMasks[m] | CAPS_MASK);
            reg(oToneW[m*2 + 1], KEY_O | TONEW_MASK | markMasks[m]);
        }

        // U
        unichar uBasic[14] = {0, 0, 0x01AF, 0x01B0, 0x00DA, 0x00FA, 0x00D9, 0x00F9, 0x1EE6, 0x1EE7, 0x0168, 0x0169, 0x1EE4, 0x1EE5};
        reg(uBasic[2], KEY_U | TONEW_MASK | CAPS_MASK); reg(uBasic[3], KEY_U | TONEW_MASK);
        for (int m = 0; m < 5; m++) {
            reg(uBasic[4 + m*2], KEY_U | markMasks[m] | CAPS_MASK);
            reg(uBasic[5 + m*2], KEY_U | markMasks[m]);
        }
        // U with TONEW_MASK (Ư: Ứ, ứ, Ừ, ừ, Sử, ử, Ữ, ữ, Ự, ự)
        unichar uToneW[10] = {0x1EE8, 0x1EE9, 0x1EEA, 0x1EEB, 0x1EEC, 0x1EED, 0x1EEE, 0x1EEF, 0x1EF0, 0x1EF1};
        for (int m = 0; m < 5; m++) {
            reg(uToneW[m*2], KEY_U | TONEW_MASK | markMasks[m] | CAPS_MASK);
            reg(uToneW[m*2 + 1], KEY_U | TONEW_MASK | markMasks[m]);
        }

        // I (Í, í, Ì, ì, Ỉ, ỉ, Ĩ, ĩ, Ị, ị)
        unichar iBasic[10] = {0x00CD, 0x00ED, 0x00CC, 0x00EC, 0x1EC8, 0x1EC9, 0x0128, 0x0129, 0x1ECA, 0x1ECB};
        for (int m = 0; m < 5; m++) {
            reg(iBasic[m*2], KEY_I | markMasks[m] | CAPS_MASK);
            reg(iBasic[m*2 + 1], KEY_I | markMasks[m]);
        }

        // Y (Ý, ý, Kỳ, ỳ, Ỷ, ỷ, Ỹ, ỹ, Ỵ, ỵ)
        unichar yBasic[10] = {0x00DD, 0x00FD, 0x1EF2, 0x1EF3, 0x1EF6, 0x1EF7, 0x1EF8, 0x1EF9, 0x1EF4, 0x1EF5};
        for (int m = 0; m < 5; m++) {
            reg(yBasic[m*2], KEY_Y | markMasks[m] | CAPS_MASK);
            reg(yBasic[m*2 + 1], KEY_Y | markMasks[m]);
        }
    }

    static inline Uint32 lookupReverseChar(unichar ch) {
        if (_reverseCharMap.empty()) initReverseCharMap();
        auto it = _reverseCharMap.find(ch);
        if (it != _reverseCharMap.end()) return it->second;
        return 0xFFFFFFFF;
    }

    static inline BOOL isVietnameseWordChar(unichar ch) {
        return lookupReverseChar(ch) != 0xFFFFFFFF;
    }

    static inline BOOL isAlphanumericKeyCode(CGKeyCode code) {
        switch (code) {
            case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
            case 8: case 9: case 11: case 12: case 13: case 14: case 15: case 16:
            case 17: case 31: case 32: case 34: case 35: case 37: case 38: case 40:
            case 45: case 46:
            case 18: case 19: case 20: case 21: case 22: case 23: case 25: case 26:
            case 28: case 29:
            case 30: case 33:
                return YES;
            default:
                return NO;
        }
    }

    enum class CaretRestoreResult {
        RestoredWord,
        NoWordAtCaret,
        Unsupported
    };

    CaretRestoreResult tryRestoreWordFromCaretContext() {
        if (vLanguage == 0 || getCurrentWordIndex() > 0) return CaretRestoreResult::Unsupported;
        if (_reverseCharMap.empty()) initReverseCharMap();
        
        NSRunningApplication *frontApp = [[NSWorkspace sharedWorkspace] frontmostApplication];
        if (!frontApp) return CaretRestoreResult::Unsupported;
        if (isTerminalApp(frontApp.bundleIdentifier)) {
            return CaretRestoreResult::Unsupported;
        }
        
        AXUIElementRef focusedElem = NULL;
        
        // Strategy 1: System-Wide Focus (direct from WindowServer)
        AXUIElementRef sysWide = AXUIElementCreateSystemWide();
        if (sysWide) {
            AXError sysErr = AXUIElementCopyAttributeValue(sysWide, kAXFocusedUIElementAttribute, (CFTypeRef *)&focusedElem);
            CFRelease(sysWide);
            if (sysErr == kAXErrorSuccess && focusedElem) {
                pid_t elemPid = 0;
                if (AXUIElementGetPid(focusedElem, &elemPid) != kAXErrorSuccess || elemPid != frontApp.processIdentifier) {
                    CFRelease(focusedElem);
                    focusedElem = NULL;
                }
            }
        }
        
        // Strategy 2: App-level Focus fallback
        if (!focusedElem) {
            AXUIElementRef appElem = AXUIElementCreateApplication(frontApp.processIdentifier);
            if (appElem) {
                AXError appErr = AXUIElementCopyAttributeValue(appElem, kAXFocusedUIElementAttribute, (CFTypeRef *)&focusedElem);
                if (appErr != kAXErrorSuccess || !focusedElem) {
                    // Strategy 3: Focused Window's focused element fallback
                    AXUIElementRef focusedWin = NULL;
                    if (AXUIElementCopyAttributeValue(appElem, kAXFocusedWindowAttribute, (CFTypeRef *)&focusedWin) == kAXErrorSuccess && focusedWin) {
                        AXUIElementCopyAttributeValue(focusedWin, kAXFocusedUIElementAttribute, (CFTypeRef *)&focusedElem);
                        CFRelease(focusedWin);
                    }
                }
                CFRelease(appElem);
            }
        }
        
        if (!focusedElem) return CaretRestoreResult::Unsupported;
        
        // Check role of focusedElem
        CFTypeRef roleVal = NULL;
        AXUIElementCopyAttributeValue(focusedElem, kAXRoleAttribute, &roleVal);
        NSString *role = roleVal ? (__bridge NSString *)roleVal : @"";
        if (roleVal) CFRelease(roleVal);
        
        // If role is AXWebArea, Chrome/Electron web content has not exposed inner elements
        if ([role isEqualToString:@"AXWebArea"]) {
            CFRelease(focusedElem);
            return CaretRestoreResult::Unsupported;
        }

        AXValueRef selectedRangeVal = NULL;
        AXError err = AXUIElementCopyAttributeValue(focusedElem, kAXSelectedTextRangeAttribute, (CFTypeRef *)&selectedRangeVal);
        if (err != kAXErrorSuccess || !selectedRangeVal) {
            // Check if focusedElem has an inner focused child (e.g. if container was returned)
            AXUIElementRef innerFocused = NULL;
            if (AXUIElementCopyAttributeValue(focusedElem, kAXFocusedUIElementAttribute, (CFTypeRef *)&innerFocused) == kAXErrorSuccess && innerFocused) {
                CFRelease(focusedElem);
                focusedElem = innerFocused;
                err = AXUIElementCopyAttributeValue(focusedElem, kAXSelectedTextRangeAttribute, (CFTypeRef *)&selectedRangeVal);
            }
        }
        if (err != kAXErrorSuccess || !selectedRangeVal) {
            CFRelease(focusedElem);
            return CaretRestoreResult::Unsupported;
        }
        
        CFRange selectedRange;
        BOOL gotRange = AXValueGetValue(selectedRangeVal, kAXValueTypeCFRange, &selectedRange);
        CFRelease(selectedRangeVal);
        if (!gotRange || selectedRange.length > 0) {
            CFRelease(focusedElem);
            return CaretRestoreResult::Unsupported;
        }
        if (selectedRange.location <= 0) {
            CFRelease(focusedElem);
            return CaretRestoreResult::NoWordAtCaret;
        }
        
        // Scan backwards up to 30 characters before caret
        CFIndex maxScan = 30;
        CFIndex start = (selectedRange.location > maxScan) ? (selectedRange.location - maxScan) : 0;
        CFIndex length = selectedRange.location - start;
        CFRange queryRange = CFRangeMake(start, length);
        
        NSString *textBefore = nil;
        AXValueRef queryRangeVal = AXValueCreate(kAXValueTypeCFRange, &queryRange);
        if (queryRangeVal) {
            CFTypeRef stringVal = NULL;
            err = AXUIElementCopyParameterizedAttributeValue(focusedElem, kAXStringForRangeParameterizedAttribute, queryRangeVal, &stringVal);
            CFRelease(queryRangeVal);
            if (err == kAXErrorSuccess && stringVal) {
                if (CFGetTypeID(stringVal) == CFStringGetTypeID()) {
                    textBefore = [NSString stringWithString:(__bridge NSString *)stringVal];
                } else if (CFGetTypeID(stringVal) == CFAttributedStringGetTypeID()) {
                    textBefore = [NSString stringWithString:(__bridge NSString *)CFAttributedStringGetString((CFAttributedStringRef)stringVal)];
                }
                CFRelease(stringVal);
            }
        }
        
        if (!textBefore || textBefore.length == 0) {
            CFTypeRef fullVal = NULL;
            err = AXUIElementCopyAttributeValue(focusedElem, kAXValueAttribute, &fullVal);
            if (err == kAXErrorSuccess && fullVal) {
                NSString *fullText = nil;
                if (CFGetTypeID(fullVal) == CFStringGetTypeID()) {
                    fullText = (__bridge NSString *)fullVal;
                } else if (CFGetTypeID(fullVal) == CFAttributedStringGetTypeID()) {
                    fullText = (__bridge NSString *)CFAttributedStringGetString((CFAttributedStringRef)fullVal);
                }
                if (fullText && fullText.length > 0) {
                    NSInteger subStart = (NSInteger)start;
                    NSInteger subLen = (NSInteger)length;
                    if (subStart < 0) subStart = 0;
                    if (subStart + subLen > (NSInteger)fullText.length) {
                        subLen = (NSInteger)fullText.length - subStart;
                    }
                    if (subStart < (NSInteger)fullText.length && subLen > 0) {
                        textBefore = [fullText substringWithRange:NSMakeRange((NSUInteger)subStart, (NSUInteger)subLen)];
                    }
                }
                CFRelease(fullVal);
            }
        }
        CFRelease(focusedElem);
        
        if (!textBefore || textBefore.length == 0) {
            return CaretRestoreResult::Unsupported;
        }
        
        // Normalize to NFC (precomposed) so accents are combined with base letters
        textBefore = [textBefore precomposedStringWithCanonicalMapping];
        
        // Extract the word immediately preceding the caret
        NSInteger endIdx = (NSInteger)textBefore.length - 1;
        NSInteger startIdx = endIdx;
        
        while (startIdx >= 0) {
            unichar ch = [textBefore characterAtIndex:(NSUInteger)startIdx];
            if (isVietnameseWordChar(ch)) {
                startIdx--;
            } else {
                break;
            }
        }
        
        // If the character right before caret was not a word char (e.g. space), no word to restore
        if (startIdx == endIdx) {
            return CaretRestoreResult::NoWordAtCaret;
        }
        
        NSString *wordStr = [textBefore substringWithRange:NSMakeRange((NSUInteger)(startIdx + 1), (NSUInteger)(endIdx - startIdx))];
        if (wordStr.length == 0 || wordStr.length >= MAX_BUFF) {
            return CaretRestoreResult::NoWordAtCaret;
        }
        
        // Map characters to engine wordData
        std::vector<Uint32> wordData;
        wordData.reserve(wordStr.length);
        for (NSUInteger i = 0; i < wordStr.length; i++) {
            unichar c = [wordStr characterAtIndex:i];
            Uint32 val = lookupReverseChar(c);
            if (val == 0xFFFFFFFF) {
                return CaretRestoreResult::NoWordAtCaret;
            }
            wordData.push_back(val);
        }
        
        if (!wordData.empty()) {
            loadCurrentTypingWord(wordData);
            return CaretRestoreResult::RestoredWord;
        }
        return CaretRestoreResult::NoWordAtCaret;
    }

    struct SuspendedWordState {
        std::vector<Uint32> wordData;
        NSString *appBundleId;
        NSTimeInterval timestamp;
        BOOL isValid;
        
        SuspendedWordState() : appBundleId(nil), timestamp(0), isValid(NO) {}
    };
    static SuspendedWordState _suspendedWord;

    void saveSuspendedWordIfNeeded() {
        if (isCurrentWordActive()) {
            getCurrentTypingWord(_suspendedWord.wordData);
            _suspendedWord.appBundleId = FRONT_APP ? [NSString stringWithString:FRONT_APP] : @"";
            _suspendedWord.timestamp = [NSDate timeIntervalSinceReferenceDate];
            _suspendedWord.isValid = YES;
        }
    }

    void clearSuspendedWord() {
        _suspendedWord.isValid = NO;
        _suspendedWord.wordData.clear();
        _suspendedWord.appBundleId = nil;
    }

    bool tryRestoreSuspendedWord(CGKeyCode keycode) {
        if (!_suspendedWord.isValid || _suspendedWord.wordData.empty()) return false;
        NSTimeInterval now = [NSDate timeIntervalSinceReferenceDate];
        if (now - _suspendedWord.timestamp > 60.0) {
            clearSuspendedWord();
            return false;
        }
        NSString *currApp = FRONT_APP ? FRONT_APP : @"";
        if (_suspendedWord.appBundleId.length > 0 && currApp.length > 0 && ![_suspendedWord.appBundleId isEqualToString:currApp]) {
            // Keep suspended word intact in case user switches back to target app
            return false;
        }
        if (!isAlphanumericKeyCode(keycode)) {
            clearSuspendedWord();
            return false;
        }
        bool ok = loadCurrentTypingWord(_suspendedWord.wordData);
        clearSuspendedWord();
        return ok;
    }

    void saveSmartSwitchKeyData() {
        getSmartSwitchKeySaveData(savedSmartSwitchKeyData);
        NSData* _data = [NSData dataWithBytes:savedSmartSwitchKeyData.data() length:savedSmartSwitchKeyData.size()];
        NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
        [prefs setObject:_data forKey:@"smartSwitchKey"];
    }
    
    void OnActiveAppChanged() { //use for smart switch key; improved on Sep 28th, 2019
        queryFrontMostApp();
        _languageTemp = getAppInputMethodStatus(string(_frontMostApp.UTF8String), vLanguage | (vCodeTable << 1));
        if ((_languageTemp & 0x01) != vLanguage) { //for input method
            if (_languageTemp != -1) {
                vLanguage = _languageTemp;
                [appDelegate onImputMethodChanged:NO];
                startNewSession();
            } else {
                saveSmartSwitchKeyData();
            }
        }
        if (vRememberCode && (_languageTemp >> 1) != vCodeTable) { //for remember table code feature
            if (_languageTemp != -1) {
                [appDelegate onCodeTableChanged:(_languageTemp >> 1)];
            } else {
                saveSmartSwitchKeyData();
            }
        }
    }
    
    void OnTableCodeChange() {
        onTableCodeChange();
        if (vRememberCode) {
            queryFrontMostApp();
            setAppInputMethodStatus(string(_frontMostApp.UTF8String), vLanguage | (vCodeTable << 1));
            saveSmartSwitchKeyData();
        }
    }
    
    void OnInputMethodChanged() {
        if (vUseSmartSwitchKey) {
            queryFrontMostApp();
            setAppInputMethodStatus(string(_frontMostApp.UTF8String), vLanguage | (vCodeTable << 1));
            saveSmartSwitchKeyData();
        }
    }
    
    void OnSpellCheckingChanged() {
        vSetCheckSpelling();
    }
    
    void InsertKeyLength(const Uint8& len) {
        _syncKey.push_back(len);
    }
    
    void SendPureCharacter(const Uint16& ch) {
        _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
        _newEventUp = CGEventCreateKeyboardEvent(myEventSource, 0, false);
        CGEventKeyboardSetUnicodeString(_newEventDown, 1, &ch);
        CGEventKeyboardSetUnicodeString(_newEventUp, 1, &ch);
        CGEventTapPostEvent(_proxy, _newEventDown);
        CGEventTapPostEvent(_proxy, _newEventUp);
        CFRelease(_newEventDown);
        CFRelease(_newEventUp);
        if (IS_DOUBLE_CODE(vCodeTable)) {
            InsertKeyLength(1);
        }
    }
    
    void SendKeyCode(Uint32 data) {
        _newChar = (Uint16)data;
        if (!(data & CHAR_CODE_MASK)) {
            if (IS_DOUBLE_CODE(vCodeTable)) //VNI
                InsertKeyLength(1);
            
            _newEventDown = CGEventCreateKeyboardEvent(myEventSource, _newChar, true);
            _newEventUp = CGEventCreateKeyboardEvent(myEventSource, _newChar, false);
            _privateFlag = CGEventGetFlags(_newEventDown);
            
            if (data & CAPS_MASK) {
                _privateFlag |= kCGEventFlagMaskShift;
            } else {
                _privateFlag &= ~kCGEventFlagMaskShift;
            }
            _privateFlag |= kCGEventFlagMaskNonCoalesced;
            
            CGEventSetFlags(_newEventDown, _privateFlag);
            CGEventSetFlags(_newEventUp, _privateFlag);
            CGEventTapPostEvent(_proxy, _newEventDown);
            CGEventTapPostEvent(_proxy, _newEventUp);
        } else {
            if (vCodeTable == 0) { //unicode 2 bytes code
                _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
                _newEventUp = CGEventCreateKeyboardEvent(myEventSource, 0, false);
                CGEventKeyboardSetUnicodeString(_newEventDown, 1, &_newChar);
                CGEventKeyboardSetUnicodeString(_newEventUp, 1, &_newChar);
                CGEventTapPostEvent(_proxy, _newEventDown);
                CGEventTapPostEvent(_proxy, _newEventUp);
            } else if (vCodeTable == 1 || vCodeTable == 2 || vCodeTable == 4) { //others such as VNI Windows, TCVN3: 1 byte code
                _newCharHi = HIBYTE(_newChar);
                _newChar = LOBYTE(_newChar);
                
                _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
                _newEventUp = CGEventCreateKeyboardEvent(myEventSource, 0, false);
                CGEventKeyboardSetUnicodeString(_newEventDown, 1, &_newChar);
                CGEventKeyboardSetUnicodeString(_newEventUp, 1, &_newChar);
                CGEventTapPostEvent(_proxy, _newEventDown);
                CGEventTapPostEvent(_proxy, _newEventUp);
                if (_newCharHi > 32) {
                    if (vCodeTable == 2) //VNI
                        InsertKeyLength(2);
                    CFRelease(_newEventDown);
                    CFRelease(_newEventUp);
                    _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
                    _newEventUp = CGEventCreateKeyboardEvent(myEventSource, 0, false);
                    CGEventKeyboardSetUnicodeString(_newEventDown, 1, &_newCharHi);
                    CGEventKeyboardSetUnicodeString(_newEventUp, 1, &_newCharHi);
                    CGEventTapPostEvent(_proxy, _newEventDown);
                    CGEventTapPostEvent(_proxy, _newEventUp);
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
                _newEventUp = CGEventCreateKeyboardEvent(myEventSource, 0, false);
                CGEventKeyboardSetUnicodeString(_newEventDown, (_newCharHi > 0 ? 2 : 1), _uniChar);
                CGEventKeyboardSetUnicodeString(_newEventUp, (_newCharHi > 0 ? 2 : 1), _uniChar);
                CGEventTapPostEvent(_proxy, _newEventDown);
                CGEventTapPostEvent(_proxy, _newEventUp);
            }
        }
        CFRelease(_newEventDown);
        CFRelease(_newEventUp);
    }
    
    void SendEmptyCharacter() {
        if (IS_DOUBLE_CODE(vCodeTable)) //VNI or Unicode Compound
            InsertKeyLength(1);
        
        _newChar = 0x202F; //empty char
        if ([_niceSpaceApp containsObject:FRONT_APP]) {
            _newChar = 0x200C; //Unicode character with empty space
        }
        
        _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
        _newEventUp = CGEventCreateKeyboardEvent(myEventSource, 0, false);
        CGEventKeyboardSetUnicodeString(_newEventDown, 1, &_newChar);
        CGEventKeyboardSetUnicodeString(_newEventUp, 1, &_newChar);
        CGEventTapPostEvent(_proxy, _newEventDown);
        CGEventTapPostEvent(_proxy, _newEventUp);
        CFRelease(_newEventDown);
        CFRelease(_newEventUp);
    }
    
    void SendVirtualKey(const Byte& vKey) {
        CGEventRef eventVkeyDown = CGEventCreateKeyboardEvent (myEventSource, vKey, true);
        CGEventRef eventVkeyUp = CGEventCreateKeyboardEvent (myEventSource, vKey, false);
        
        CGEventTapPostEvent(_proxy, eventVkeyDown);
        CGEventTapPostEvent(_proxy, eventVkeyUp);
        
        CFRelease(eventVkeyDown);
        CFRelease(eventVkeyUp);
    }

    void SendSingleBackspace(bool isTerminal = false) {
        CGEventRef bsDown = CGEventCreateKeyboardEvent(myEventSource, 51, true);
        CGEventRef bsUp = CGEventCreateKeyboardEvent(myEventSource, 51, false);
        uint64_t now = mach_absolute_time();
        CGEventSetTimestamp(bsDown, now);
        CGEventSetTimestamp(bsUp, now);
        
        CGEventTapPostEvent(_proxy, bsDown);
        CGEventTapPostEvent(_proxy, bsUp);
        
        CFRelease(bsDown);
        CFRelease(bsUp);
        
        if (isTerminal) {
            usleep(1500); // 1.5ms inter-keystroke spacing for terminal PTY line discipline
        }
    }

    void SendBackspace(bool isTerminal = false) {
        SendSingleBackspace(isTerminal);
        
        if (IS_DOUBLE_CODE(vCodeTable)) { //VNI or Unicode Compound
            if (_syncKey.back() > 1) {
                if (!(vCodeTable == 3 && containUnicodeCompoundApp(FRONT_APP))) {
                    SendSingleBackspace(isTerminal);
                }
            }
            _syncKey.pop_back();
        }
    }
    
    void SendShiftAndLeftArrow() {
        CGEventRef eventVkeyDown = CGEventCreateKeyboardEvent (myEventSource, KEY_LEFT, true);
        CGEventRef eventVkeyUp = CGEventCreateKeyboardEvent (myEventSource, KEY_LEFT, false);
        _privateFlag = CGEventGetFlags(eventVkeyDown);
        _privateFlag |= kCGEventFlagMaskShift;
        CGEventSetFlags(eventVkeyDown, _privateFlag);
        CGEventSetFlags(eventVkeyUp, _privateFlag);
        
        CGEventTapPostEvent(_proxy, eventVkeyDown);
        CGEventTapPostEvent(_proxy, eventVkeyUp);
        
        if (IS_DOUBLE_CODE(vCodeTable)) { //VNI or Unicode Compound
            if (_syncKey.back() > 1) {
                if (!(vCodeTable == 3 && containUnicodeCompoundApp(FRONT_APP))) {
                    CGEventTapPostEvent(_proxy, eventVkeyDown);
                    CGEventTapPostEvent(_proxy, eventVkeyUp);
                }
            }
            _syncKey.pop_back();
        }
        CFRelease(eventVkeyDown);
        CFRelease(eventVkeyUp);
    }
    
    void SendCutKey() {
        CGEventRef eventVkeyDown = CGEventCreateKeyboardEvent (myEventSource, KEY_X, true);
        CGEventRef eventVkeyUp = CGEventCreateKeyboardEvent (myEventSource, KEY_X, false);
        _privateFlag = CGEventGetFlags(eventVkeyDown);
        _privateFlag |= NX_COMMANDMASK;
        CGEventSetFlags(eventVkeyDown, _privateFlag);
        CGEventSetFlags(eventVkeyUp, _privateFlag);
        
        CGEventTapPostEvent(_proxy, eventVkeyDown);
        CGEventTapPostEvent(_proxy, eventVkeyUp);
        
        CFRelease(eventVkeyDown);
        CFRelease(eventVkeyUp);
    }
    
    void SendNewCharString(const bool& dataFromMacro=false, const Uint16& offset=0) {
        _j = 0;
        _newCharSize = dataFromMacro ? pData->macroData.size() : pData->newCharCount;
        _willContinuteSending = false;
        _willSendControlKey = false;
        
        if (_newCharSize > 0) {
            for (_k = dataFromMacro ? offset : pData->newCharCount - 1 - offset;
                 dataFromMacro ? _k < pData->macroData.size() : _k >= 0;
                 dataFromMacro ? _k++ : _k--) {
                
                if (_j >= 16) {
                    _willContinuteSending = true;
                    break;
                }
                
                _tempChar = DYNA_DATA(dataFromMacro, _k);
                if (_tempChar & PURE_CHARACTER_MASK) {
                    _newCharString[_j++] = _tempChar;
                    if (IS_DOUBLE_CODE(vCodeTable)) {
                        InsertKeyLength(1);
                    }
                } else if (!(_tempChar & CHAR_CODE_MASK)) {
                    if (IS_DOUBLE_CODE(vCodeTable)) //VNI
                        InsertKeyLength(1);
                    _newCharString[_j++] = keyCodeToCharacter(_tempChar);
                } else {
                    if (vCodeTable == 0) {  //unicode 2 bytes code
                        _newCharString[_j++] = _tempChar;
                    } else if (vCodeTable == 1 || vCodeTable == 2 || vCodeTable == 4) { //others such as VNI Windows, TCVN3: 1 byte code
                        _newChar = _tempChar;
                        _newCharHi = HIBYTE(_newChar);
                        _newChar = LOBYTE(_newChar);
                        _newCharString[_j++] = _newChar;
                        
                        if (_newCharHi > 32) {
                            if (vCodeTable == 2) //VNI
                                InsertKeyLength(2);
                            _newCharString[_j++] = _newCharHi;
                            _newCharSize++;
                        } else {
                            if (vCodeTable == 2) //VNI
                                InsertKeyLength(1);
                        }
                    } else if (vCodeTable == 3) { //Unicode Compound
                        _newChar = _tempChar;
                        _newCharHi = (_newChar >> 13);
                        _newChar &= 0x1FFF;
                        
                        InsertKeyLength(_newCharHi > 0 ? 2 : 1);
                        _newCharString[_j++] = _newChar;
                        if (_newCharHi > 0) {
                            _newCharSize++;
                            _newCharString[_j++] = _unicodeCompoundMark[_newCharHi - 1];
                        }
                        
                    }
                }
            }//end for
        }
        
        if (!_willContinuteSending && (pData->code == vRestore || pData->code == vRestoreAndStartNewSession)) { //if is restore
            if (keyCodeToCharacter(_keycode) != 0) {
                _newCharSize++;
                _newCharString[_j++] = keyCodeToCharacter(_keycode | ((_flag & kCGEventFlagMaskAlphaShift) || (_flag & kCGEventFlagMaskShift) ? CAPS_MASK : 0));
            } else {
                _willSendControlKey = true;
            }
        }
        if (!_willContinuteSending && pData->code == vRestoreAndStartNewSession) {
            startNewSession();
        }
        
        _newEventDown = CGEventCreateKeyboardEvent(myEventSource, 0, true);
        _newEventUp = CGEventCreateKeyboardEvent(myEventSource, 0, false);
        uint64_t now = mach_absolute_time();
        CGEventSetTimestamp(_newEventDown, now);
        CGEventSetTimestamp(_newEventUp, now);
        CGEventKeyboardSetUnicodeString(_newEventDown, _willContinuteSending ? 16 : _newCharSize - offset, _newCharString);
        CGEventKeyboardSetUnicodeString(_newEventUp, _willContinuteSending ? 16 : _newCharSize - offset, _newCharString);
        CGEventTapPostEvent(_proxy, _newEventDown);
        CGEventTapPostEvent(_proxy, _newEventUp);
        CFRelease(_newEventDown);
        CFRelease(_newEventUp);

        if (_willContinuteSending) {
            SendNewCharString(dataFromMacro, dataFromMacro ? _k : 16);
        }
        
        //the case when hCode is vRestore or vRestoreAndStartNewSession, the word is invalid and last key is control key such as TAB, LEFT ARROW, RIGHT ARROW,...
        if (_willSendControlKey) {
            SendKeyCode(_keycode);
        }
    }
            
    bool checkHotKey(int hotKeyData, bool checkKeyCode=true) {
        if ((hotKeyData & (~0x8000)) == EMPTY_HOTKEY)
            return false;
        if (HAS_CONTROL(hotKeyData) ^ GET_BOOL(_lastFlag & kCGEventFlagMaskControl))
            return false;
        if (HAS_OPTION(hotKeyData) ^ GET_BOOL(_lastFlag & kCGEventFlagMaskAlternate))
            return false;
        if (HAS_COMMAND(hotKeyData) ^ GET_BOOL(_lastFlag & kCGEventFlagMaskCommand))
            return false;
        if (HAS_SHIFT(hotKeyData) ^ GET_BOOL(_lastFlag & kCGEventFlagMaskShift))
            return false;
        if (checkKeyCode) {
            if (GET_SWITCH_KEY(hotKeyData) != _keycode)
                return false;
        }
        return true;
    }
    
    void switchLanguage() {
        if (vLanguage == 0)
            vLanguage = 1;
        else
            vLanguage = 0;
        if (HAS_BEEP(vSwitchKeyStatus))
            NSBeep();
        [appDelegate onImputMethodChanged:YES];
        startNewSession();
    }
    
    void handleMacro() {
        NSString* frontApp = FRONT_APP;
        BOOL isTerm = isTerminalApp(frontApp);
        //fix autocomplete
        if (shouldUseRecommendWorkaround(frontApp)) {
            SendEmptyCharacter();
            pData->backspaceCount++;
        }
        
        //send backspace
        if (pData->backspaceCount > 0) {
            for (int i = 0; i < pData->backspaceCount; i++) {
                SendBackspace(isTerm);
            }
            if (isTerm) {
                usleep(1500);
            }
        }
        //send real data
        if (!vSendKeyStepByStep) {
            SendNewCharString(true);
        } else {
            for (int i = 0; i < pData->macroData.size(); i++) {
                if (pData->macroData[i] & PURE_CHARACTER_MASK) {
                    SendPureCharacter(pData->macroData[i]);
                } else {
                    SendKeyCode(pData->macroData[i]);
                }
            }
        }
        SendKeyCode(_keycode | (_flag & kCGEventFlagMaskShift ? CAPS_MASK : 0));
    }

    // TODO: Research API to convert character into CGKeyCode more elegantly!
    int ConvertKeyStringToKeyCode(NSString *keyString, CGKeyCode fallback) {
        // Infomation about capitalization (shift/caps) is already included
        // in the original CGEvent, only find out which position on keyboard a key is pressed
        NSString *lowercasedKeyString = [keyString lowercaseString];
        if (!lowercasedKeyString) {
            return fallback;
        }
        
        NSNumber *keycode = [keyStringToKeyCodeMap objectForKey:lowercasedKeyString];

        if (keycode) {
            return [keycode intValue];
        }
        return fallback;
    }

    // If conversion fails, return fallbackKeyCode
    CGKeyCode ConvertEventToKeyboadLayoutCompatKeyCode(CGEventRef keyEvent, CGKeyCode fallbackKeyCode) {
        NSEvent *kbLayoutCompatEvent = [NSEvent eventWithCGEvent:keyEvent];
        NSString *kbLayoutCompatKeyString = kbLayoutCompatEvent.charactersIgnoringModifiers;
        return ConvertKeyStringToKeyCode(kbLayoutCompatKeyString,
                                         fallbackKeyCode);
    }

    /**
     * MAIN HOOK entry, very important function.
     * MAIN Callback.
     */
    CGEventRef OpenKeyCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
        //dont handle my event
        if (CGEventGetIntegerValueField(event, kCGEventSourceStateID) == CGEventSourceGetSourceStateID(myEventSource)) {
            return event;
        }
        
        _flag = CGEventGetFlags(event);
        _keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        
        if (type == kCGEventKeyDown && vPerformLayoutCompat) {
            // If conversion fail, use current keycode
           _keycode = ConvertEventToKeyboadLayoutCompatKeyCode(event, _keycode);
        }
        
        //switch language shortcut; convert hotkey
        if (type == kCGEventKeyDown) {
            if (GET_SWITCH_KEY(vSwitchKeyStatus) != _keycode && GET_SWITCH_KEY(convertToolHotKey) != _keycode) {
                _lastFlag = 0;
            } else {
                if (GET_SWITCH_KEY(vSwitchKeyStatus) == _keycode && checkHotKey(vSwitchKeyStatus, GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE)){
                    switchLanguage();
                    _lastFlag = 0;
                    _hasJustUsedHotKey = true;
                    return NULL;
                }
                if (GET_SWITCH_KEY(convertToolHotKey) == _keycode && checkHotKey(convertToolHotKey, GET_SWITCH_KEY(convertToolHotKey) != 0xFE)){
                    [appDelegate onQuickConvert];
                    _lastFlag = 0;
                    _hasJustUsedHotKey = true;
                    return NULL;
                }
            }
            _hasJustUsedHotKey = _lastFlag != 0;
        } else if (type == kCGEventFlagsChanged) {
            if (_lastFlag == 0 || _lastFlag < _flag) {
                _lastFlag = _flag;
            } else if (_lastFlag > _flag)  {
                //check switch
                if (checkHotKey(vSwitchKeyStatus, GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE)) {
                    _lastFlag = 0;
                    switchLanguage();
                    _hasJustUsedHotKey = true;
                    return NULL;
                }
                if (checkHotKey(convertToolHotKey, GET_SWITCH_KEY(convertToolHotKey) != 0xFE)) {
                    _lastFlag = 0;
                    [appDelegate onQuickConvert];
                    _hasJustUsedHotKey = true;
                    return NULL;
                }
                //check temporarily turn off spell checking
                if (vTempOffSpelling && !_hasJustUsedHotKey && _lastFlag & kCGEventFlagMaskControl) {
                    vTempOffSpellChecking();
                }
                if (vTempOffOpenKey && !_hasJustUsedHotKey && _lastFlag & kCGEventFlagMaskCommand) {
                    vTempOffEngine();
                }
                _lastFlag = 0;
                _hasJustUsedHotKey = false;
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
            if (type == kCGEventLeftMouseDown || type == kCGEventRightMouseDown) {
                saveSuspendedWordIfNeeded();
            }
            RequestNewSession();
            return event;
        }

        //if "turn off Vietnamese when in other language" mode on
        if(vOtherLanguage){
            TISInputSourceRef isource = TISCopyCurrentKeyboardInputSource();
            if ( isource != NULL )
            {
                CFArrayRef languages = (CFArrayRef) TISGetInputSourceProperty(isource, kTISPropertyInputSourceLanguages);
                
                if (CFArrayGetCount(languages) > 0) {
                    CFStringRef langRef = (CFStringRef)CFArrayGetValueAtIndex(languages, 0);
                    NSString *currentLanguage = (__bridge NSString *)langRef;
                    if(![currentLanguage isLike:@"en"]){
                        return event;
                    }
                    CFRelease(langRef);
                    CFRelease(isource);
                }
            }
        }
        
        //handle keyboard
        if (type == kCGEventKeyDown) {
            if (_keycode == KEY_SPACE || _keycode == KEY_RETURN || _keycode == KEY_ENTER || 
                _keycode == KEY_TAB || _keycode == KEY_ESC || _keycode == KEY_DELETE ||
                _keycode == KEY_LEFT || _keycode == KEY_RIGHT || _keycode == KEY_UP || _keycode == KEY_DOWN ||
                OTHER_CONTROL_KEY) {
                clearSuspendedWord();
            }

            if (getCurrentWordIndex() == 0 && !OTHER_CONTROL_KEY && isAlphanumericKeyCode(_keycode)) {
                CaretRestoreResult res = tryRestoreWordFromCaretContext();
                if (res == CaretRestoreResult::RestoredWord) {
                    clearSuspendedWord();
                } else if (res == CaretRestoreResult::NoWordAtCaret) {
                    clearSuspendedWord();
                } else { // Unsupported (Chrome, etc.)
                    tryRestoreSuspendedWord(_keycode);
                }
            }
            //send event signal to Engine
            vKeyHandleEvent(vKeyEvent::Keyboard,
                            vKeyEventState::KeyDown,
                            _keycode,
                            _flag & kCGEventFlagMaskShift ? 1 : (_flag & kCGEventFlagMaskAlphaShift ? 2 : 0),
                            OTHER_CONTROL_KEY);

            if (isCurrentWordActive()) {
                saveSuspendedWordIfNeeded();
            } else if (getCurrentWordIndex() == 0) {
                clearSuspendedWord();
            }
            if (pData->code == vDoNothing) { //do nothing
                if (IS_DOUBLE_CODE(vCodeTable)) { //VNI
                    if (pData->extCode == 1) { //break key
                        _syncKey.clear();
                    } else if (pData->extCode == 2) { //delete key
                        if (_syncKey.size() > 0) {
                            if (_syncKey.back() > 1 && (vCodeTable == 2 || !containUnicodeCompoundApp(FRONT_APP))) {
                                //send one more backspace
                                SendSingleBackspace(isTerminalApp(FRONT_APP));
                            }
                            _syncKey.pop_back();
                        }
                       
                    } else if (pData->extCode == 3) { //normal key
                        InsertKeyLength(1);
                    }
                }
                return event;
            } else if (pData->code == vWillProcess || pData->code == vRestore || pData->code == vRestoreAndStartNewSession) { //handle result signal
                NSString* currentFrontApp = FRONT_APP;
                BOOL isTerm = isTerminalApp(currentFrontApp);
                
                //fix autocomplete
                if (shouldUseRecommendWorkaround(currentFrontApp) && pData->extCode != 4) {
                    if (vFixChromiumBrowser && [_unicodeCompoundApp containsObject:currentFrontApp]) {
                        if (pData->backspaceCount > 0) {
                            SendShiftAndLeftArrow();
                            if (pData->backspaceCount == 1)
                                pData->backspaceCount--;
                        }
                    } else {
                        SendEmptyCharacter();
                        pData->backspaceCount++;
                    
                    }
                }

                if (shouldUseSelectionReplacement(currentFrontApp) && pData->backspaceCount > 0) {
                    for (_i = 0; _i < pData->backspaceCount; _i++) {
                        SendShiftAndLeftArrow();
                    }
                    pData->backspaceCount = 0;
                }
                
                //send backspace
                if (pData->backspaceCount > 0 && pData->backspaceCount < MAX_BUFF) {
                    for (_i = 0; _i < pData->backspaceCount; _i++) {
                        SendBackspace(isTerm);
                    }
                    if (isTerm) {
                        usleep(1500);
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
                    if (pData->code == vRestore || pData->code == vRestoreAndStartNewSession) {
                        SendKeyCode(_keycode | ((_flag & kCGEventFlagMaskAlphaShift) || (_flag & kCGEventFlagMaskShift) ? CAPS_MASK : 0));
                    }
                    if (pData->code == vRestoreAndStartNewSession) {
                        startNewSession();
                    }
                }
            } else if (pData->code == vReplaceMaro) { //MACRO
                handleMacro();
            }
            
            return NULL;
        }
        
        return event;
    }
}
