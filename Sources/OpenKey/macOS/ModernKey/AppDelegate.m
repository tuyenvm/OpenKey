//
//  AppDelegate.m
//  ModernKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <ServiceManagement/ServiceManagement.h>
#include <libproc.h>
#include <sys/proc_info.h>
#import "AppDelegate.h"
#import "ViewController.h"
#import "OpenKeyManager.h"
#import "MJAccessibilityUtils.h"
#import "SettingsWindowController.h"

AppDelegate* appDelegate;
extern ViewController* viewController;
extern void OnTableCodeChange(void);
extern void OnInputMethodChanged(void);
extern void RequestNewSession(void);
extern void RefreshInputContext(void);
extern void OnActiveAppChanged(void);

//see document in Engine.h
int vLanguage = 1;
int vInputType = 0;
int vFreeMark = 0;
int vCodeTable = 0;
int vCheckSpelling = 1;
int vUseModernOrthography = 1;
int vQuickTelex = 0;
#define DEFAULT_SWITCH_STATUS 0x7A000206 //default option + z
int vSwitchKeyStatus = DEFAULT_SWITCH_STATUS;
int vRestoreIfWrongSpelling = 0;
int vFixRecommendBrowser = 1;
int vUseMacro = 0;
int vUseMacroInEnglishMode = 1;
int vAutoCapsMacro = 0;
int vSendKeyStepByStep = 0;
int vUseSmartSwitchKey = 1;
int vUpperCaseFirstChar = 0;
int vTempOffSpelling = 0;
int vAllowConsonantZFWJ = 0;
int vQuickStartConsonant = 0;
int vQuickEndConsonant = 0;
int vRememberCode = 1; //new on version 2.0
int vOtherLanguage = 1; //new on version 2.0
int vTempOffOpenKey = 0; //new on version 2.0

int vShowIconOnDock = 0; //new on version 2.0

int vPerformLayoutCompat = 0;

//beta feature
int vFixChromiumBrowser = 0; //new on version 2.0

extern int convertToolHotKey;
extern bool convertToolDontAlertWhenCompleted;

@interface AppDelegate ()

@end


@implementation AppDelegate {
    SettingsWindowController *_settingsWC;
    
    NSStatusItem *statusItem;
    NSMenu *theMenu;
    
    NSArray<NSMenuItem *> *menuTypingStyles;
    NSMenuItem *menuEnglish, *menuConnection;
}

-(void)askPermission {
    [self onControlPanelSelected];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    appDelegate = self;
    
    [self registerSupportedNotification];
    
    //set quick tooltip
    [[NSUserDefaults standardUserDefaults] setObject: [NSNumber numberWithInt: 50]
                                              forKey: @"NSInitialToolTipDelay"];
    
    //check whether this app has been launched before that or not
    //Only check instances owned by current user (for multi-user/Fast User Switching support)
    uid_t currentUID = getuid();
    NSArray<NSRunningApplication *>* runningApps = [[NSWorkspace sharedWorkspace] runningApplications];
    pid_t myPID = [[NSProcessInfo processInfo] processIdentifier];
    BOOL alreadyRunning = NO;

    for (NSRunningApplication *app in runningApps) {
        if ([app.bundleIdentifier isEqualToString:OPENKEY_BUNDLE] &&
            app.processIdentifier != myPID) {
            pid_t pid = app.processIdentifier;
            struct proc_bsdinfo proc;
            int size = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &proc, sizeof(proc));
            if (size == sizeof(proc) && proc.pbi_uid == currentUID) {
                alreadyRunning = YES;
                break;
            }
        }
    }

    if (alreadyRunning) {
        [NSApp terminate:nil];
        return;
    }
    
    
    vShowIconOnDock=0;
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
    
    if (vSwitchKeyStatus & 0x8000)
        NSBeep();

    [self createStatusBarMenu];
    
    //init
    dispatch_async(dispatch_get_main_queue(), ^{
        if (![OpenKeyManager initEventTap]) {
            [self onControlPanelSelected];
        } else {
            OnActiveAppChanged();

        }
        [self setQuickConvertString];
    });
    
    //load default config if is first launch
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"NonFirstTime"] == 0) {
        [self loadDefaultConfig];
    }
    [[NSUserDefaults standardUserDefaults] setInteger:1 forKey:@"NonFirstTime"];
    // Settings and macros remain usable before Accessibility is granted.
    [OpenKeyManager prepareEngine];
    
    //check update if enable
    NSInteger dontCheckUpdate = [[NSUserDefaults standardUserDefaults] integerForKey:@"DontCheckUpdate"];
    if (!dontCheckUpdate)
        [OpenKeyManager checkNewVersion:nil callbackFunc:nil];
    
    //correct run on startup
    NSInteger val = [[NSUserDefaults standardUserDefaults] integerForKey:@"RunOnStartup"];
    [appDelegate setRunOnStartup:val];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag {
    [self onControlPanelSelected];
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

- (NSMenuItem *)item:(NSString *)title action:(SEL)action key:(NSString *)key inMenu:(NSMenu *)menu {
    NSMenuItem *item=[menu addItemWithTitle:title action:action keyEquivalent:key]; item.target=self; return item;
}
- (NSImage *)inputBadge:(NSString *)label {
    NSImage *image=[[NSImage alloc] initWithSize:NSMakeSize(26,18)];
    [image lockFocus];
    [NSColor.blackColor setStroke];
    NSBezierPath *outline=[NSBezierPath bezierPathWithRoundedRect:NSMakeRect(1,1,24,16) xRadius:4 yRadius:4];
    outline.lineWidth=1.2; [outline stroke];
    NSDictionary *attributes=@{NSFontAttributeName:[NSFont boldSystemFontOfSize:10], NSForegroundColorAttributeName:NSColor.blackColor};
    NSSize size=[label sizeWithAttributes:attributes];
    [label drawAtPoint:NSMakePoint((26-size.width)/2,(18-size.height)/2) withAttributes:attributes];
    [image unlockFocus]; image.template=YES; return image;
}
- (NSArray<NSString *> *)typingStyleNames { return @[@"Telex",@"VNI",@"Simple Telex 1",@"Simple Telex 2"]; }
- (void)createStatusBarMenu {
    statusItem=[[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    theMenu=[NSMenu new]; theMenu.autoenablesItems=NO; theMenu.delegate=self;
    menuConnection=[self item:@"Cho phép OpenKey…" action:@selector(reconnect:) key:@"" inMenu:theMenu];
    menuEnglish=[self item:@"English" action:@selector(selectMenuSource:) key:@"" inMenu:theMenu]; menuEnglish.tag=-1;
    menuEnglish.image=[self inputBadge:@"EN"];
    NSMutableArray *styles=[NSMutableArray new];
    for (NSString *name in [self typingStyleNames]) {
        NSMenuItem *item=[self item:name action:@selector(selectMenuSource:) key:@"" inMenu:theMenu];
        item.tag=styles.count; item.image=[self inputBadge:@"VI"]; [styles addObject:item];
    }
    menuTypingStyles=styles;
    [theMenu addItem:NSMenuItem.separatorItem];
    [self item:@"Cài đặt…" action:@selector(onControlPanelSelected) key:@"," inMenu:theMenu];
    [self item:@"Chuyển mã văn bản…" action:@selector(onConvertTool) key:@"" inMenu:theMenu];
    [theMenu addItem:NSMenuItem.separatorItem];
    [self item:@"Giới thiệu OpenKey" action:@selector(onAboutSelected) key:@"" inMenu:theMenu];
    [self item:@"Thoát OpenKey" action:@selector(quit:) key:@"q" inMenu:theMenu];
    statusItem.menu=theMenu;
    NSMenu *main=[NSMenu new], *application=[NSMenu new], *edit=[NSMenu new], *tools=[NSMenu new];
    NSMenuItem *app=[NSMenuItem new]; app.submenu=application; [main addItem:app];
    [self item:@"Giới thiệu OpenKey" action:@selector(onAboutSelected) key:@"" inMenu:application];
    [application addItem:NSMenuItem.separatorItem];
    [self item:@"Cài đặt…" action:@selector(onControlPanelSelected) key:@"," inMenu:application];
    [self item:@"Kiểm tra cập nhật…" action:@selector(checkUpdate:) key:@"" inMenu:application];
    [application addItem:NSMenuItem.separatorItem];
    NSMenuItem *services=[application addItemWithTitle:@"Dịch vụ" action:nil keyEquivalent:@""];
    services.submenu=[NSMenu new]; NSApp.servicesMenu=services.submenu;
    [application addItem:NSMenuItem.separatorItem];
    NSMenuItem *hide=[application addItemWithTitle:@"Ẩn OpenKey" action:@selector(hide:) keyEquivalent:@"h"]; hide.target=NSApp;
    [self item:@"Thoát OpenKey" action:@selector(quit:) key:@"q" inMenu:application];
    NSMenuItem *editItem=[[NSMenuItem alloc] initWithTitle:@"Sửa" action:nil keyEquivalent:@""]; editItem.submenu=edit; [main addItem:editItem];
    [edit addItemWithTitle:@"Hoàn tác" action:@selector(undo:) keyEquivalent:@"z"];
    [edit addItem:NSMenuItem.separatorItem];
    [edit addItemWithTitle:@"Cắt" action:@selector(cut:) keyEquivalent:@"x"];
    [edit addItemWithTitle:@"Sao chép" action:@selector(copy:) keyEquivalent:@"c"];
    [edit addItemWithTitle:@"Dán" action:@selector(paste:) keyEquivalent:@"v"];
    [edit addItemWithTitle:@"Chọn tất cả" action:@selector(selectAll:) keyEquivalent:@"a"];
    NSMenuItem *toolItem=[[NSMenuItem alloc] initWithTitle:@"Công cụ" action:nil keyEquivalent:@""]; toolItem.submenu=tools; [main addItem:toolItem];
    [self item:@"Chuyển mã văn bản…" action:@selector(onConvertTool) key:@"" inMenu:tools];
    NSApp.mainMenu=main;
    [self fillData];
}
- (NSInteger)fixedInputLanguage {
    id rule=[[NSUserDefaults standardUserDefaults] dictionaryForKey:@"AppLanguageRules"][[NSWorkspace sharedWorkspace].frontmostApplication.bundleIdentifier ?: @""];
    return [rule isKindOfClass:NSNumber.class] && ([rule integerValue]==0 || [rule integerValue]==1) ? [rule integerValue] : -1;
}
- (void)menuWillOpen:(NSMenu *)menu {
    BOOL trusted=MJAccessibilityIsEnabled(); BOOL active=[OpenKeyManager isActive];
    menuConnection.hidden=trusted && active;
    menuConnection.title=trusted ? @"Kết nối lại…" : @"Cho phép OpenKey…";
    NSInteger fixed=[self fixedInputLanguage];
    menuEnglish.enabled=trusted && active && fixed!=1;
    for (NSMenuItem *item in menuTypingStyles) item.enabled=trusted && active && fixed!=0;
    [self fillData];
}
- (void)selectMenuSource:(NSMenuItem *)sender {
    if (sender.tag < -1 || sender.tag >= (NSInteger)menuTypingStyles.count) return;
    if (!MJAccessibilityIsEnabled() || ![OpenKeyManager isActive]) { [self onControlPanelSelected]; return; }
    int language=sender.tag>=0;
    NSInteger fixed=[self fixedInputLanguage]; if (fixed!=-1 && fixed!=language) return;
    if (language) [[NSUserDefaults standardUserDefaults] setInteger:sender.tag forKey:@"InputType"];
    [self setInputLanguage:language]; OnInputMethodChanged(); RequestNewSession();
}
- (void)reconnect:(id)sender {
    if (!MJAccessibilityIsEnabled()) { [self onControlPanelSelected]; return; }
    if ([OpenKeyManager initEventTap]) OnActiveAppChanged();
}
- (void)quit:(id)sender { [NSApp terminate:nil]; }
- (void)checkUpdate:(id)sender { [OpenKeyManager checkNewVersion:nil callbackFunc:nil]; }
- (void)setQuickConvertString {} // Retained for legacy controller callers; conversion is a Tools command.

-(void)loadDefaultConfig {
    vLanguage = 1; [[NSUserDefaults standardUserDefaults] setInteger:vLanguage forKey:@"InputMethod"];
    vInputType = 0; [[NSUserDefaults standardUserDefaults] setInteger:vInputType forKey:@"InputType"];
    vFreeMark = 0; [[NSUserDefaults standardUserDefaults] setInteger:vFreeMark forKey:@"FreeMark"];
    vCheckSpelling = 1; [[NSUserDefaults standardUserDefaults] setInteger:vCheckSpelling forKey:@"Spelling"];
    vCodeTable = 0; [[NSUserDefaults standardUserDefaults] setInteger:vCodeTable forKey:@"CodeTable"];
    vSwitchKeyStatus = DEFAULT_SWITCH_STATUS; [[NSUserDefaults standardUserDefaults] setInteger:vSwitchKeyStatus forKey:@"SwitchKeyStatus"];
    vQuickTelex = 0; [[NSUserDefaults standardUserDefaults] setInteger:vQuickTelex forKey:@"QuickTelex"];
    vUseModernOrthography = 0; [[NSUserDefaults standardUserDefaults] setInteger:vUseModernOrthography forKey:@"ModernOrthography"];
    vRestoreIfWrongSpelling = 0; [[NSUserDefaults standardUserDefaults] setInteger:vRestoreIfWrongSpelling forKey:@"RestoreIfInvalidWord"];
    vFixRecommendBrowser = 1; [[NSUserDefaults standardUserDefaults] setInteger:vFixRecommendBrowser forKey:@"FixRecommendBrowser"];
    vUseMacro = 0; [[NSUserDefaults standardUserDefaults] setInteger:vUseMacro forKey:@"UseMacro"];
    vUseMacroInEnglishMode = 0; [[NSUserDefaults standardUserDefaults] setInteger:vUseMacroInEnglishMode forKey:@"UseMacroInEnglishMode"];
    vSendKeyStepByStep = 0;[[NSUserDefaults standardUserDefaults] setInteger:vUseMacroInEnglishMode forKey:@"SendKeyStepByStep"];
    vUseSmartSwitchKey = 0;[[NSUserDefaults standardUserDefaults] setInteger:vUseSmartSwitchKey forKey:@"UseSmartSwitchKey"];
    vUpperCaseFirstChar = 0;[[NSUserDefaults standardUserDefaults] setInteger:vUpperCaseFirstChar forKey:@"UpperCaseFirstChar"];
    vTempOffSpelling = 0;[[NSUserDefaults standardUserDefaults] setInteger:vTempOffSpelling forKey:@"vTempOffSpelling"];
    vAllowConsonantZFWJ = 0;[[NSUserDefaults standardUserDefaults] setInteger:vAllowConsonantZFWJ forKey:@"vAllowConsonantZFWJ"];
    vQuickStartConsonant = 0;[[NSUserDefaults standardUserDefaults] setInteger:vQuickStartConsonant forKey:@"vQuickStartConsonant"];
    vQuickEndConsonant = 0;[[NSUserDefaults standardUserDefaults] setInteger:vQuickEndConsonant forKey:@"vQuickEndConsonant"];
    vRememberCode = 1;[[NSUserDefaults standardUserDefaults] setInteger:vRememberCode forKey:@"vRememberCode"];
    vOtherLanguage = 1;[[NSUserDefaults standardUserDefaults] setInteger:vOtherLanguage forKey:@"vOtherLanguage"];
    vTempOffOpenKey = 0;[[NSUserDefaults standardUserDefaults] setInteger:vTempOffOpenKey forKey:@"vTempOffOpenKey"];
    vShowIconOnDock = 0;[[NSUserDefaults standardUserDefaults] setInteger:vShowIconOnDock forKey:@"vShowIconOnDock"];
    vFixChromiumBrowser = 0;[[NSUserDefaults standardUserDefaults] setInteger:vFixChromiumBrowser forKey:@"vFixChromiumBrowser"];
    vPerformLayoutCompat = 0;[[NSUserDefaults standardUserDefaults] setInteger:vPerformLayoutCompat forKey:@"vPerformLayoutCompat"];

    [[NSUserDefaults standardUserDefaults] setInteger:1 forKey:@"GrayIcon"];
    [[NSUserDefaults standardUserDefaults] setInteger:0 forKey:@"RunOnStartup"];

    [self fillData];
    [viewController fillData];
}

-(void)setRunOnStartup:(BOOL)val {
    CFStringRef appId = (__bridge CFStringRef)@"com.tuyenmai.OpenKeyHelper";
    SMLoginItemSetEnabled(appId, val);
}

-(void)setGrayIcon:(BOOL)val {
    [self fillData];
}

-(void)showIconOnDock:(BOOL)val {
    [NSApp setActivationPolicy: val ? NSApplicationActivationPolicyRegular : NSApplicationActivationPolicyAccessory];
}

#pragma mark -StatusBar menu data

- (void)fillData {
    NSUserDefaults *prefs=NSUserDefaults.standardUserDefaults;
    vUseSmartSwitchKey=(int)[prefs integerForKey:@"UseSmartSwitchKey"];
    vLanguage=(int)[prefs integerForKey:@"InputMethod"];
    vInputType=(int)[prefs integerForKey:@"InputType"];
    vCodeTable=(int)[prefs integerForKey:@"CodeTable"];
    vSwitchKeyStatus=(int)[prefs integerForKey:@"SwitchKeyStatus"];
    if (!vSwitchKeyStatus) vSwitchKeyStatus=DEFAULT_SWITCH_STATUS;
    for (NSMenuItem *item in menuTypingStyles) item.state=vLanguage && vInputType==item.tag ? NSControlStateValueOn : NSControlStateValueOff;
    menuEnglish.state=vLanguage ? NSControlStateValueOff : NSControlStateValueOn;
    NSString *name=vLanguage ? [self typingStyleNames][MAX(0,MIN(3,vInputType))] : @"English";
    NSString *badge=vLanguage ? @"VI" : @"EN";
    if (![statusItem.button.title isEqual:name]) {
        statusItem.button.image=[self inputBadge:badge];
        statusItem.button.imagePosition=NSImageLeft; statusItem.button.title=name;
    }
    statusItem.button.toolTip=[@"OpenKey — " stringByAppendingString:name];
    statusItem.button.accessibilityLabel=[NSString stringWithFormat:@"OpenKey, %@",name];
}

-(void)setInputLanguage:(int)language {
    vLanguage = language ? 1 : 0;
    [[NSUserDefaults standardUserDefaults] setInteger:vLanguage forKey:@"InputMethod"];
    [self fillData];
    [viewController fillData];
}

-(void)onImputMethodChanged:(BOOL)willNotify {
    NSInteger intInputMethod = [[NSUserDefaults standardUserDefaults] integerForKey:@"InputMethod"];
    if (intInputMethod == 0)
        intInputMethod = 1;
    else
        intInputMethod = 0;
    vLanguage = (int)intInputMethod;
    [[NSUserDefaults standardUserDefaults] setInteger:intInputMethod forKey:@"InputMethod"];

    [self fillData];
    [viewController fillData];
    
    if (willNotify)
        OnInputMethodChanged();
}

#pragma mark -StatusBar menu action
- (void)onInputMethodSelected {
    NSDictionary *rules = [[NSUserDefaults standardUserDefaults] dictionaryForKey:@"AppLanguageRules"];
    NSNumber *rule = rules[[NSWorkspace sharedWorkspace].frontmostApplication.bundleIdentifier ?: @""];
    if ([rule isKindOfClass:NSNumber.class] && (rule.integerValue == 0 || rule.integerValue == 1)) return;
    [self onImputMethodChanged:YES];
}

- (void)onInputTypeSelected:(id)sender {
    NSMenuItem *menuItem = (NSMenuItem*) sender;
    [self onInputTypeSelectedIndex:(int)menuItem.tag];
}

- (void)onInputTypeSelectedIndex:(int)index {
    [[NSUserDefaults standardUserDefaults] setInteger:index forKey:@"InputType"];
    vInputType = index;
    [self fillData];
    [viewController fillData];
}

- (void)onCodeTableChanged:(int)index {
    [[NSUserDefaults standardUserDefaults] setInteger:index forKey:@"CodeTable"];
    vCodeTable = index;
    [self fillData];
    [viewController fillData];
    OnTableCodeChange();
}

- (void)onCodeSelected:(id)sender {
    NSMenuItem *menuItem = (NSMenuItem*) sender;
    [self onCodeTableChanged:(int)menuItem.tag];
}

-(void)onConvertTool {
    [self onControlPanelSelected];
    [_settingsWC showConversion];
}

-(void)onQuickConvert {
    if ([OpenKeyManager quickConvert]) {
        if (!convertToolDontAlertWhenCompleted) {
            [OpenKeyManager showMessage: nil message:@"Chuyển mã thành công!" subMsg:@"Kết quả đã được lưu trong clipboard."];
        }
    } else {
        [OpenKeyManager showMessage: nil message:@"Không có dữ liệu trong clipboard!" subMsg:@"Hãy sao chép một đoạn text để chuyển đổi!"];
    }
}

-(void) onControlPanelSelected {
    RefreshInputContext();
    if (!_settingsWC) _settingsWC = [[SettingsWindowController alloc] init];
    [_settingsWC showWindow:nil];
    [_settingsWC.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

-(void) onAboutSelected {
    [NSApp orderFrontStandardAboutPanelWithOptions:@{NSAboutPanelOptionCredits:[[NSAttributedString alloc] initWithString:@"Bộ gõ tiếng Việt nguồn mở\nMai Vũ Tuyên và cộng đồng đóng góp · GPL-3.0"]}];
    [NSApp activateIgnoringOtherApps:YES];
}

#pragma mark -Short key event
-(void)onSwitchLanguage {
    [self onInputMethodSelected];
    [viewController fillData];
}

#pragma mark Reset OpenKey after mac computer awake
-(void)receiveWakeNote: (NSNotification*)note {
    [OpenKeyManager resumeEventTap];
}

-(void)receiveSleepNote: (NSNotification*)note {
    [OpenKeyManager stopEventTap];
}

-(void)receiveActiveSpaceChanged: (NSNotification*)note {
    RequestNewSession();
}

-(void)activeAppChanged: (NSNotification*)note {
    if ([OpenKeyManager isInited]) {
        OnActiveAppChanged();
    }
}

-(void)inputSourceChanged:(NSNotification*)note {
    if ([OpenKeyManager isInited]) RefreshInputContext();
}

-(void)registerSupportedNotification {
    [[NSDistributedNotificationCenter defaultCenter] addObserver:self
        selector:@selector(inputSourceChanged:)
        name:(__bridge NSString *)kTISNotifySelectedKeyboardInputSourceChanged object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver: self
                                                           selector: @selector(receiveWakeNote:)
                                                               name: NSWorkspaceDidWakeNotification object: NULL];
    
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver: self
                                                           selector: @selector(receiveSleepNote:)
                                                               name: NSWorkspaceWillSleepNotification object: NULL];
    
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver: self
                                                           selector: @selector(receiveActiveSpaceChanged:)
                                                               name: NSWorkspaceActiveSpaceDidChangeNotification object: NULL];
    
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver: self
                                                           selector: @selector(activeAppChanged:)
                                                               name: NSWorkspaceDidActivateApplicationNotification object: NULL];
}
@end
