// Isolated settings checks/preview: no keyboard hook, permission prompt or login item.
#import <objc/runtime.h>
#import "../Sources/OpenKey/macOS/ModernKey/SettingsWindowController.m"
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
static BOOL trusted=NO,active=NO,paused=NO;
static SettingsWindowController *previewController;
BOOL MJAccessibilityIsEnabled(void) { return trusted; }
void MJAccessibilityOpenPanel(void) {}
void OnActiveAppChanged(void) {}
void RequestNewSession(void) {}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincomplete-implementation"
@implementation AppDelegate
-(void)applicationDidFinishLaunching:(NSNotification *)note {
    [previewController showWindow:nil]; [NSApp activateIgnoringOtherApps:YES];
}
-(void)setInputLanguage:(int)language { vLanguage=language; }
-(void)onInputTypeSelectedIndex:(int)index { vInputType=index; }
-(void)onCodeTableChanged:(int)index { vCodeTable=index; }
-(void)onAdvancedSettings {}
@end
@implementation OpenKeyManager
+(NSArray *)getTableCodes { return @[@"Unicode", @"TCVN3 (ABC)", @"VNI Windows", @"Unicode tổ hợp", @"CP 1258"]; }
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
    BOOL preview=[[[NSBundle mainBundle] bundleIdentifier] isEqual:@"org.openkey.settings-preview"];
    NSString *suite=preview ? @"org.openkey.settings-preview.preferences" : [@"org.openkey.settings-tests." stringByAppendingString:NSUUID.UUID.UUIDString];
    testDefaults=[[NSUserDefaults alloc] initWithSuiteName:suite];
    method_exchangeImplementations(class_getClassMethod(NSUserDefaults.class,@selector(standardUserDefaults)),
        class_getClassMethod(NSUserDefaults.class,@selector(openKeyTestDefaults)));
    [NSApplication sharedApplication]; appDelegate=[AppDelegate new];
    SettingsWindowController *controller=[SettingsWindowController new];
    if (preview) {
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        previewController=controller; NSApp.delegate=appDelegate; [NSApp run];
        return 0;
    }
    [controller refresh];
    assert(![[controller valueForKey:@"retry"] isEnabled]);
    trusted=YES; [controller refresh]; assert(active); // Approval needs no restart.
    trusted=NO; [controller refresh]; assert(!active); // Revocation stops interception.
    paused=YES; trusted=YES; [controller refresh]; assert(!active && paused);
    [controller retry:nil]; assert(active && !paused); // Only explicit retry clears pause.
    NSPopUpButton *rule=[[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    [rule addItemsWithTitles:@[@"English",@"Tiếng Việt",@"Nhớ lựa chọn"]];
    rule.identifier=@"org.openkey.test"; [rule selectItemAtIndex:0];
    [controller changeRule:rule]; [controller reloadRules];
    assert([testDefaults dictionaryForKey:@"AppLanguageRules"].count==1);
    assert([controller numberOfRowsInTableView:[controller valueForKey:@"rulesTable"]]==1);
    [[controller valueForKey:@"rulesTable"] selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];
    [controller removeApp:nil]; assert([testDefaults dictionaryForKey:@"AppLanguageRules"].count==0);
    [testDefaults removePersistentDomainForName:suite];
    puts("Settings: permission approval/revocation, explicit pause/retry and isolated app-rule persistence passed");
} }
