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
-(void)selectMenuSource:(NSMenuItem *)sender;
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
    NSArray<NSMenuItem *> *styles=[delegate valueForKey:@"menuTypingStyles"];
    NSMenuItem *english=[delegate valueForKey:@"menuEnglish"];
    assert(styles.count==4);
    NSStatusItem *status=[delegate valueForKey:@"statusItem"];
    for (NSUInteger i=0;i<styles.count;i++) {
        [delegate selectMenuSource:styles[i]];
        assert(vLanguage==1 && vInputType==i && [prefs integerForKey:@"InputType"]==i);
        assert(styles[i].state==NSControlStateValueOn && english.state==NSControlStateValueOff);
        assert([status.button.title isEqual:styles[i].title] && status.button.image.isTemplate);
        for (NSUInteger j=0;j<styles.count;j++) assert(styles[j].state==(i==j ? NSControlStateValueOn : NSControlStateValueOff));
    }
    assert(saves==4);
    [delegate selectMenuSource:english];
    assert(vLanguage==0 && vInputType==3 && saves==5 && settingsOpened==0);
    assert(english.state==NSControlStateValueOn && [status.button.title isEqual:@"English"]);
    for (NSMenuItem *item in styles) assert(item.state==NSControlStateValueOff);
    [prefs setObject:@{@"org.openkey.menu-test":@1} forKey:@"AppLanguageRules"];
    [delegate menuWillOpen:[delegate valueForKey:@"theMenu"]]; assert(!english.enabled && styles[0].enabled);
    [delegate selectMenuSource:styles[0]]; assert(vLanguage==1 && vInputType==0 && saves==6);
    [delegate selectMenuSource:english]; assert(vLanguage==1 && saves==6);
    [prefs setObject:@{@"org.openkey.menu-test":@0} forKey:@"AppLanguageRules"];
    [delegate menuWillOpen:[delegate valueForKey:@"theMenu"]]; assert(english.enabled && !styles[0].enabled);
    [delegate selectMenuSource:styles[1]]; assert(vInputType==0 && saves==6);
    [delegate selectMenuSource:english]; assert(vLanguage==0 && saves==7);
    [prefs removeObjectForKey:@"AppLanguageRules"]; trusted=NO;
    [delegate menuWillOpen:[delegate valueForKey:@"theMenu"]]; assert(!english.enabled && !styles[0].enabled);
    [delegate selectMenuSource:styles[0]]; assert(settingsOpened==1 && saves==7);
    trusted=YES; active=NO;
    [delegate menuWillOpen:[delegate valueForKey:@"theMenu"]]; assert(!english.enabled && !styles[0].enabled);
    assert(![[delegate valueForKey:@"menuConnection"] isHidden]);
    NSMenu *application=NSApp.mainMenu.itemArray.firstObject.submenu;
    NSMenuItem *settings=[application itemWithTitle:@"Cài đặt…"];
    assert([settings.keyEquivalent isEqual:@","] && settings.target==delegate);
    assert(settings.action==@selector(onControlPanelSelected));
    NSMenu *menu=[delegate valueForKey:@"theMenu"];
    assert(menu.numberOfItems==12 && [menu itemWithTitle:@"Gõ tắt…"]==nil);
    [[NSStatusBar systemStatusBar] removeStatusItem:[delegate valueForKey:@"statusItem"]];
    [prefs removePersistentDomainForName:suite];
    puts("Menu: all typing sources, active badge/name/checkmarks, saved mode, fixed rules, permission gate and Command-Comma passed");
} }
