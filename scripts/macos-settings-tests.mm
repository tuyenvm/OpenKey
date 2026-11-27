// Isolated settings checks/preview: no keyboard hook, permission prompt or login item.
#import <objc/runtime.h>
#import "../Sources/OpenKey/macOS/ModernKey/SettingsWindowController.mm"
#import <assert.h>

static NSUserDefaults *testDefaults;
@interface NSUserDefaults (OpenKeySettingsTests)
+(NSUserDefaults *)openKeyTestDefaults;
@end
@implementation NSUserDefaults (OpenKeySettingsTests)
+(NSUserDefaults *)openKeyTestDefaults { return testDefaults; }
@end
AppDelegate *appDelegate;
int vLanguage=1,vInputType=1,vCodeTable=0,vUseSmartSwitchKey=1;
int vFreeMark=0,vCheckSpelling=1,vSwitchKeyStatus=0x7A000206,vUseModernOrthography=0,vQuickTelex=0;
int vRestoreIfWrongSpelling=0,vFixRecommendBrowser=1,vUseMacro=1,vUseMacroInEnglishMode=0,vAutoCapsMacro=0;
int vUpperCaseFirstChar=0,vTempOffSpelling=0,vAllowConsonantZFWJ=0,vQuickStartConsonant=0,vQuickEndConsonant=0;
int vRememberCode=1,vOtherLanguage=0,vTempOffOpenKey=0;
int vSendKeyStepByStep=0,vFixChromiumBrowser=0,vPerformLayoutCompat=0,vShowIconOnDock=0;
static BOOL trusted=NO,active=NO,paused=NO;
BOOL MJAccessibilityIsEnabled(void) { return trusted; }
void MJAccessibilityOpenPanel(void) {}
static BOOL recordingShortcut=NO;
extern "C" void SetShortcutRecording(BOOL recording) { recordingShortcut=recording; }
void OnActiveAppChanged(void) {}
void RequestNewSession(void) {}
void OnSpellCheckingChanged(void) { vSetCheckSpelling(); }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincomplete-implementation"
@implementation AppDelegate
-(void)setInputLanguage:(int)language { vLanguage=language; }
-(void)onInputTypeSelectedIndex:(int)index { vInputType=index; }
-(void)onCodeTableChanged:(int)index { vCodeTable=index; }
-(void)setRunOnStartup:(BOOL)value {}
-(void)setGrayIcon:(BOOL)value {}
-(void)showIconOnDock:(BOOL)value {}
-(void)loadDefaultConfig {}
-(void)setQuickConvertString {}
@end
@implementation OpenKeyManager
+(NSArray *)getTableCodes { return @[@"Unicode", @"TCVN3 (ABC)", @"VNI Windows", @"Unicode tổ hợp", @"CP 1258"]; }
+(void)checkNewVersion:(NSWindow *)window callbackFunc:(CheckNewVersionCallback)callback { if (callback) callback(); }
+(BOOL)quickConvert { return NO; }
+(BOOL)isActive { return active; }
+(BOOL)isInited { return active; }
+(BOOL)isPaused { return paused; }
+(BOOL)resumeEventTap { if (!paused) active=trusted; return active; }
+(BOOL)initEventTap { paused=NO;active=trusted;return active; }
+(BOOL)stopEventTap { active=NO;return YES; }
+(void)showMessage:(NSWindow *)window message:(NSString *)message subMsg:(NSString *)subMsg {}
@end
#pragma clang diagnostic pop
int main(void) { @autoreleasepool {
    NSString *suite=[@"org.openkey.settings-tests." stringByAppendingString:NSUUID.UUID.UUIDString];
    testDefaults=[[NSUserDefaults alloc] initWithSuiteName:suite];
    method_exchangeImplementations(class_getClassMethod(NSUserDefaults.class,@selector(standardUserDefaults)),
        class_getClassMethod(NSUserDefaults.class,@selector(openKeyTestDefaults)));
    [NSApplication sharedApplication]; appDelegate=[AppDelegate new];
    [testDefaults setObject:@"typing" forKey:@"SettingsPane"];
    assert(addMacro("oktest","kiểm tra tiếng Việt"));
    vector<Byte> bytes; getMacroSaveData(bytes);
    NSData *originalMacros=[NSData dataWithBytes:bytes.data() length:bytes.size()];
    SettingsWindowController *controller=[SettingsWindowController new];
    assert([[controller valueForKey:@"gateVisible"] boolValue]);
    NSTabViewController *panes=[controller valueForKey:@"panes"];
    assert(panes.tabViewItems.count==4 && panes.selectedTabViewItemIndex==1);
    [controller showWindow:nil]; assert(controller.window.visible);
    trusted=YES; [controller refresh]; assert(active);
    assert(![[controller valueForKey:@"gateVisible"] boolValue] && !controller.window.visible);
    [controller showWindow:nil]; assert(controller.window.contentViewController==panes);
    [controller showPage:@"general"]; assert(panes.selectedTabViewItemIndex==0);
    [controller showPage:@"macros"]; assert(panes.selectedTabViewItemIndex==0);
    assert(![controller respondsToSelector:NSSelectorFromString(@"openTextReplacements:")]);
    assert(![controller respondsToSelector:NSSelectorFromString(@"exportLegacyReplacements:")]);
    vector<Byte> after; getMacroSaveData(after);
    assert([[NSData dataWithBytes:after.data() length:after.size()] isEqual:originalMacros]);
    MyTextField *recorder=[controller valueForKey:@"inputKey"];
    assert([recorder.accessibilityRole isEqual:NSAccessibilityButtonRole]);
    assert([recorder accessibilityPerformPress] && recordingShortcut);
    NSEvent *key=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:NSEventModifierFlagOption
        timestamp:0 windowNumber:controller.window.windowNumber context:nil characters:@"x" charactersIgnoringModifiers:@"x" isARepeat:NO keyCode:kVK_ANSI_X];
    [recorder keyDown:key]; assert((vSwitchKeyStatus&0xFF)==kVK_ANSI_X && (vSwitchKeyStatus&0x200));
    assert([recorder.stringValue isEqual:@"⌥X"] && !recordingShortcut);
    assert([recorder accessibilityPerformPress]);
    [controller windowDidResignKey:[NSNotification notificationWithName:NSWindowDidResignKeyNotification object:controller.window]]; assert(!recordingShortcut);
    [controller showPage:@"typing"];
    id spelling=[controller valueForKey:@"controls"][@"Spelling"];
    [spelling setState:NSControlStateValueOff]; [controller changeOption:spelling];
    assert(vCheckSpelling==0 && ![testDefaults boolForKey:@"Spelling"]);
    assert(![[controller valueForKey:@"controls"][@"RestoreIfInvalidWord"] isEnabled]);
    if (@available(macOS 13.0,*)) for (OKSettingsFormController *form in [controller valueForKey:@"formControllers"]) assert(![form refreshControls]);
    NSPopUpButton *method=[controller valueForKey:@"inputType"];
    [method selectItemAtIndex:0]; [controller changeInputType:method]; assert(vInputType==0);
    NSPopUpButton *encoding=[controller valueForKey:@"encoding"];
    [encoding selectItemAtIndex:2]; [controller changeEncoding:encoding]; assert(vCodeTable==2);
    NSDictionary *controls=[controller valueForKey:@"controls"];
    for (NSString *option in @[@"vAllowConsonantZFWJ",@"UpperCaseFirstChar",@"vQuickStartConsonant",@"vQuickEndConsonant",@"vTempOffSpelling",@"vTempOffOpenKey"]) {
        assert(controls[option]); [controls[option] setState:NSControlStateValueOn]; [controller changeOption:controls[option]];
        assert([testDefaults boolForKey:option]);
    }
    [controller showPage:@"compatibility"]; assert(panes.selectedTabViewItemIndex==3);
    for (NSString *option in @[@"FixRecommendBrowser",@"vFixChromiumBrowser",@"SendKeyStepByStep",@"vPerformLayoutCompat",@"vOtherLanguage"]) {
        assert(controls[option]); [controls[option] setState:NSControlStateValueOn]; [controller changeOption:controls[option]];
        assert([testDefaults boolForKey:option]);
    }
    [controller showPage:@"general"];
    [controls[@"SwitchBeep"] setState:NSControlStateValueOn]; [controller changeOption:controls[@"SwitchBeep"]];
    [controller onShortcutRecorded:0x7A000206]; assert(vSwitchKeyStatus==0x7A008206);
    [controller showPage:@"apps"]; assert(panes.selectedTabViewItemIndex==2);
    assert(controller.window.contentViewController==panes && controller.window.attachedSheet==nil);
    NSPopUpButton *rule=[[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    [rule addItemsWithTitles:@[@"English",@"Tiếng Việt",@"Nhớ lựa chọn"]];
    rule.identifier=@"org.openkey.test"; [rule selectItemAtIndex:0];
    [controller changeRule:rule]; [controller reloadRules];
    assert([testDefaults dictionaryForKey:@"AppLanguageRules"].count==1);
    [[controller valueForKey:@"rulesTable"] selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];
    [controller removeApp:nil]; assert([testDefaults dictionaryForKey:@"AppLanguageRules"].count==0);
    [controller showConversion];
    NSWindow *sheet=[[controller valueForKey:@"utilityWC"] window];
    assert(sheet.contentLayoutRect.size.width>=600 && sheet.contentLayoutRect.size.height>=360);
    NSPopUpButton *from=[controller valueForKey:@"fromCode"]; [from selectItemAtIndex:2];
    [controller changeConversion:nil]; assert(convertToolFromCode==2);
    [controller closeUtility:nil];
    [controller showPage:@"general"]; assert([recorder accessibilityPerformPress] && recordingShortcut);
    trusted=NO; [controller refresh]; assert(!recordingShortcut); assert(!active && [[controller valueForKey:@"gateVisible"] boolValue]);
    assert(controller.window.toolbar==nil && controller.window.contentViewController!=panes);
    paused=YES; trusted=YES; [controller refresh]; assert(!active && paused);
    [controller close];
    [testDefaults removePersistentDomainForName:suite];
    puts("Settings: four panes, restored typing/compatibility controls, permission gate, shortcut recorder, app rules and conversion passed");
} }
