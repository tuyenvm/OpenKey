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
static SettingsWindowController *previewController;
BOOL MJAccessibilityIsEnabled(void) { return trusted; }
void MJAccessibilityOpenPanel(void) {}
void OnActiveAppChanged(void) {}
void RequestNewSession(void) {}
void OnSpellCheckingChanged(void) { vSetCheckSpelling(); }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincomplete-implementation"
@implementation AppDelegate
-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app { return YES; }
-(void)applicationDidFinishLaunching:(NSNotification *)note {
    [previewController showPage:@"typing"];
    [previewController showWindow:nil]; [NSApp activateIgnoringOtherApps:YES];
}
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
    BOOL preview=[[[NSBundle mainBundle] bundleIdentifier] hasPrefix:@"org.openkey.settings-"];
    NSString *suite=preview ? @"org.openkey.settings-preview.preferences" : [@"org.openkey.settings-tests." stringByAppendingString:NSUUID.UUID.UUIDString];
    testDefaults=[[NSUserDefaults alloc] initWithSuiteName:suite];
    method_exchangeImplementations(class_getClassMethod(NSUserDefaults.class,@selector(standardUserDefaults)),
        class_getClassMethod(NSUserDefaults.class,@selector(openKeyTestDefaults)));
    [NSApplication sharedApplication]; appDelegate=[AppDelegate new];
    SettingsWindowController *controller=[SettingsWindowController new];
    if (preview) {
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSMenu *menu=[NSMenu new]; NSMenuItem *application=[NSMenuItem new]; [menu addItem:application];
        NSMenu *commands=[NSMenu new]; [commands addItemWithTitle:@"Quit Preview" action:@selector(terminate:) keyEquivalent:@"q"];
        application.submenu=commands; NSApp.mainMenu=menu;
        previewController=controller; NSApp.delegate=appDelegate; [NSApp run];
        return 0;
    }
    [controller refresh];
    assert(![[controller valueForKey:@"retry"] isEnabled]);
    trusted=YES; [controller refresh]; assert(active); // Approval needs no restart.
    trusted=NO; [controller refresh]; assert(!active); // Revocation stops interception.
    paused=YES; trusted=YES; [controller refresh]; assert(!active && paused);
    [controller retry:nil]; assert(active && !paused); // Only explicit retry clears pause.
    [controller showPage:@"apps"];
    NSPopUpButton *rule=[[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    [rule addItemsWithTitles:@[@"English",@"Tiếng Việt",@"Nhớ lựa chọn"]];
    rule.identifier=@"org.openkey.test"; [rule selectItemAtIndex:0];
    [controller changeRule:rule]; [controller reloadRules];
    assert([testDefaults dictionaryForKey:@"AppLanguageRules"].count==1);
    assert([controller numberOfRowsInTableView:[controller valueForKey:@"rulesTable"]]==1);
    [[controller valueForKey:@"rulesTable"] selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];
    [controller removeApp:nil]; assert([testDefaults dictionaryForKey:@"AppLanguageRules"].count==0);
    [controller showPage:@"typing"];
    id spelling=[controller valueForKey:@"controls"][@"Spelling"];
    [spelling setState:NSControlStateValueOff]; [controller changeOption:spelling];
    assert(vCheckSpelling==0 && ![testDefaults boolForKey:@"Spelling"]);
    [controller showPage:@"shortcuts"];
    NSButton *modifier=[controller valueForKey:@"controls"][@"input.256"];
    modifier.state=NSControlStateValueOn; [controller changeModifier:modifier];
    assert(vSwitchKeyStatus & 0x100);
    [controller showPage:@"conversion"];
    NSPopUpButton *caseOption=[controller valueForKey:@"caseOption"];
    [caseOption selectItemAtIndex:1]; [controller changeConversion:caseOption];
    assert(convertToolToAllCaps && !convertToolToAllNonCaps);
    [controller showPage:@"macros"];
    [[controller valueForKey:@"macroName"] setStringValue:@"test"];
    [[controller valueForKey:@"macroContent"] setStringValue:@"tiếng Việt"];
    [controller saveMacro:nil]; assert(hasMacro("test"));
    [controller showPage:@"system"]; [controller showPage:@"compatibility"];
    NSSearchField *search=[controller valueForKey:@"search"]; search.stringValue=@"clipboard";
    [controller controlTextDidChange:[NSNotification notificationWithName:NSControlTextDidChangeNotification object:search]];
    assert([[controller valueForKey:@"currentPage"] isEqual:@"conversion"]);
    [testDefaults removePersistentDomainForName:suite];
    puts("Settings: permissions, pause/retry, app rules, migrated options/shortcuts, conversion, macros and search passed");
} }
