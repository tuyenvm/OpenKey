// Exercise production AppDelegate menu actions without a keyboard hook or user preferences.
#import <objc/runtime.h>
#import "../Sources/OpenKey/macOS/ModernKey/AppDelegate.h"
#import "../Sources/OpenKey/macOS/ModernKey/SettingsWindowController.h"
#import "../Sources/OpenKey/macOS/ModernKey/OpenKeyManager.h"
#import "../Sources/OpenKey/macOS/ModernKey/MJAccessibilityUtils.h"
#import "../Sources/OpenKey/engine/Engine.h"
#include <cassert>
extern int vSwitchKeyStatus;
ViewController *viewController=nil;
bool convertToolDontAlertWhenCompleted=false;
static NSUserDefaults *prefs;
static BOOL trusted=YES,active=YES;
static int saves=0,settingsOpened=0;
extern "C" {
void OnInputMethodChanged(void) { ++saves; }
void OnTableCodeChange(void) {}
void RequestNewSession(void) {}
void RefreshInputContext(void) {}
void OnActiveAppChanged(void) {}
}
BOOL MJAccessibilityIsEnabled(void) { return trusted; }
@interface NSUserDefaults (MenuTests)
+(NSUserDefaults *)menuTestDefaults;
@end
@implementation NSUserDefaults (MenuTests)
+(NSUserDefaults *)menuTestDefaults { return prefs; }
@end
@interface MenuRunningApp : NSObject
@property(readonly) NSString *bundleIdentifier;
@end
@implementation MenuRunningApp
-(NSString *)bundleIdentifier { return @"org.openkey.menu-test"; }
@end
@interface NSWorkspace (MenuTests)
-(NSRunningApplication *)menuTestApp;
@end
@implementation NSWorkspace (MenuTests)
-(NSRunningApplication *)menuTestApp { static MenuRunningApp *app=[MenuRunningApp new]; return (NSRunningApplication *)app; }
@end
@interface AppDelegate (MenuTests)
-(void)createStatusBarMenu;
-(void)selectMenuLanguage:(NSMenuItem *)sender;
@end
@interface MenuDelegate : AppDelegate
@end
@implementation MenuDelegate
-(void)onControlPanelSelected { ++settingsOpened; }
@end
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincomplete-implementation"
@implementation SettingsWindowController
@end
@implementation OpenKeyManager
+(BOOL)isActive { return active; }
+(BOOL)initEventTap { return active=trusted; }
@end
#pragma clang diagnostic pop
int main() { @autoreleasepool {
    NSString *suite=[@"org.openkey.menu-tests." stringByAppendingString:NSUUID.UUID.UUIDString];
    prefs=[[NSUserDefaults alloc] initWithSuiteName:suite];
    method_exchangeImplementations(class_getClassMethod(NSUserDefaults.class,@selector(standardUserDefaults)),
        class_getClassMethod(NSUserDefaults.class,@selector(menuTestDefaults)));
    [NSApplication sharedApplication];
    MenuDelegate *delegate=[MenuDelegate new]; [delegate createStatusBarMenu];
    method_exchangeImplementations(class_getInstanceMethod(NSWorkspace.class,@selector(frontmostApplication)),
        class_getInstanceMethod(NSWorkspace.class,@selector(menuTestApp)));
    NSMenuItem *vietnamese=[delegate valueForKey:@"menuVietnamese"], *english=[delegate valueForKey:@"menuEnglish"];
    [delegate selectMenuLanguage:vietnamese];
    assert(vLanguage==1 && [prefs integerForKey:@"InputMethod"]==1 && saves==1);
    [delegate selectMenuLanguage:english];
    assert(vLanguage==0 && saves==2 && settingsOpened==0);
    assert(english.state==NSControlStateValueOn && vietnamese.state==NSControlStateValueOff);
    [prefs setObject:@{@"org.openkey.menu-test":@1} forKey:@"AppLanguageRules"];
    [delegate menuWillOpen:[delegate valueForKey:@"theMenu"]]; assert(!english.enabled && !vietnamese.enabled);
    [delegate selectMenuLanguage:vietnamese]; assert(vLanguage==0 && saves==2);
    [prefs removeObjectForKey:@"AppLanguageRules"]; trusted=NO;
    [delegate menuWillOpen:[delegate valueForKey:@"theMenu"]]; assert(!english.enabled && !vietnamese.enabled);
    [delegate selectMenuLanguage:vietnamese]; assert(settingsOpened==1 && saves==2);
    NSMenu *application=NSApp.mainMenu.itemArray.firstObject.submenu;
    NSMenuItem *settings=[application itemWithTitle:@"Cài đặt…"];
    assert([settings.keyEquivalent isEqual:@","] && settings.target==delegate);
    assert(settings.action==@selector(onControlPanelSelected));
    NSMenu *menu=[delegate valueForKey:@"theMenu"];
    assert(menu.numberOfItems==9 && [menu itemWithTitle:@"Gõ tắt…"]==nil);
    [[NSStatusBar systemStatusBar] removeStatusItem:[delegate valueForKey:@"statusItem"]];
    [prefs removePersistentDomainForName:suite];
    puts("Menu: real language actions, saved mode/checkmarks, fixed rules, permission gate and Command-Comma passed");
} }
