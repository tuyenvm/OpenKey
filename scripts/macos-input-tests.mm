// Native regression checks. Event posting is intercepted: this test sends no UI input.
#import <Cocoa/Cocoa.h>
static NSString *testAppID=@"org.openkey.test.rule";
@interface TestRunningApp : NSObject
@property(readonly) NSString *bundleIdentifier;
@end
@implementation TestRunningApp
-(NSString *)bundleIdentifier { return testAppID; }
@end
@interface TestWorkspace : NSObject
+(instancetype)sharedWorkspace;
@property(readonly) NSRunningApplication *frontmostApplication;
@end
@implementation TestWorkspace
+(instancetype)sharedWorkspace { static TestWorkspace *workspace=[TestWorkspace new];return workspace; }
-(NSRunningApplication *)frontmostApplication { static TestRunningApp *app=[TestRunningApp new];return (NSRunningApplication *)app; }
@end
static NSString *testSuite;
@interface TestDefaults : NSUserDefaults
+(instancetype)standardUserDefaults;
@end
@implementation TestDefaults
+(instancetype)standardUserDefaults {
    static TestDefaults *prefs;
    if (!prefs) {
        testSuite=[@"org.openkey.native-tests." stringByAppendingString:NSUUID.UUID.UUIDString];
        prefs=[[TestDefaults alloc] initWithSuiteName:testSuite];
    }
    return prefs;
}
@end
#define NSWorkspace TestWorkspace
#define NSUserDefaults TestDefaults
extern "C" void TestCGEventTapPostEvent(CGEventTapProxy,CGEventRef);
#define CGEventTapPostEvent TestCGEventTapPostEvent
#include "../Sources/OpenKey/macOS/ModernKey/OpenKey.mm"
#undef CGEventTapPostEvent
#include <cassert>

ViewController *viewController=nil;
AppDelegate *appDelegate=nil;
int vLanguage=1,vInputType=1,vCodeTable=0,vFreeMark=0,vCheckSpelling=1;
int vSwitchKeyStatus=0xFE0000FE,vUseModernOrthography=0,vQuickTelex=0,vRestoreIfWrongSpelling=0;
int vFixRecommendBrowser=0,vUseMacro=0,vUseMacroInEnglishMode=0,vAutoCapsMacro=0,vUseSmartSwitchKey=0;
int vUpperCaseFirstChar=0,vTempOffSpelling=0,vAllowConsonantZFWJ=0,vQuickStartConsonant=0,vQuickEndConsonant=0;
int vRememberCode=0,vOtherLanguage=0,vTempOffOpenKey=0;
int vSendKeyStepByStep=0,vFixChromiumBrowser=0,vPerformLayoutCompat=0;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincomplete-implementation"
@implementation AppDelegate
-(void)setInputLanguage:(int)language { vLanguage=language; }
-(void)onCodeTableChanged:(int)code { vCodeTable=code; }
-(void)onImputMethodChanged:(BOOL)notify { if (notify) OnInputMethodChanged(); }
@end
#pragma clang diagnostic pop
static int recovered=0,paused=0;
static std::vector<CGEventRef> posts;
extern "C" void TestCGEventTapPostEvent(CGEventTapProxy proxy,CGEventRef event) {posts.push_back(CGEventCreateCopy(event));}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincomplete-implementation"
@implementation OpenKeyManager
+(void)recoverEventTap {++recovered;}
+(void)handleTapDisabledByUser {++paused;}
@end
#pragma clang diagnostic pop
int main() {@autoreleasepool {
    assert(OKTargetsSpotlight(42,42,@"com.apple.Notes"));
    assert(!OKTargetsSpotlight(43,42,@"com.apple.Spotlight"));
    assert(!OKTargetsSpotlight(0,42,@"com.apple.Notes"));
    assert(OKIsTerminalApp(@"com.apple.Terminal"));
    assert(!OKIsTerminalApp(@"com.apple.Notes"));
    OpenKeyCallback(NULL,kCGEventTapDisabledByTimeout,NULL,NULL);
    OpenKeyCallback(NULL,kCGEventTapDisabledByUserInput,NULL,NULL);
    assert(recovered==1 && paused==1);
    myEventSource=CGEventSourceCreate(kCGEventSourceStatePrivate);
    pData=(vKeyHookState*)vKeyInit();
    _eventTargetPID=42;_eventTargetSerial=1234;
    for(Uint16 key:{Uint16(KEY_V),Uint16(KEY_I),Uint16(KEY_E),Uint16(KEY_6)})
        vKeyHandleEvent(vKeyEvent::Keyboard,vKeyEventState::KeyDown,key);
    assert(pData->code==vWillProcess && pData->newCharCount==1);
    SendNewCharString();
    assert(posts.size()==2);
    UniChar units[20]={};UniCharCount count=0;
    CGEventKeyboardGetUnicodeString(posts[0],20,&count,units);
    assert(count==1 && units[0]==0xEA);
    for(auto event:posts) {
        assert(CGEventGetIntegerValueField(event,kCGEventTargetUnixProcessID)==42);
        assert(CGEventGetIntegerValueField(event,kCGEventTargetProcessSerialNumber)==1234);
        CFRelease(event);
    }
    posts.clear();
    ResetInputSession();
    convertToolHotKey=0xFE0000FE;
    CGEventSourceRef externalSource=CGEventSourceCreate(kCGEventSourceStatePrivate);
    for(CGKeyCode key:{CGKeyCode(KEY_A),CGKeyCode(KEY_1)}) {
        CGEventRef input=CGEventCreateKeyboardEvent(externalSource,key,true);
        CGEventSetFlags(input,0);
        CGEventSetIntegerValueField(input,kCGEventSourceUnixProcessID,getpid()+1);
        CGEventSetIntegerValueField(input,kCGEventTargetUnixProcessID,42);
        OpenKeyCallback(NULL,kCGEventKeyDown,input,NULL);
        CFRelease(input);
    }
    assert(posts.size()==4); // Backspace pair + accented Unicode pair; other private sources compose.
    count=0;CGEventKeyboardGetUnicodeString(posts[2],20,&count,units);
    assert(count==1 && units[0]==0xE1);
    CGEventRef own=CGEventCreateKeyboardEvent(myEventSource,KEY_A,true);
    CGEventSetIntegerValueField(own,kCGEventSourceUnixProcessID,getpid());
    auto before=posts.size();assert(OpenKeyCallback(NULL,kCGEventKeyDown,own,NULL)==own);
    assert(posts.size()==before);
    CFRelease(own);CFRelease(externalSource);
    for(auto event:posts)CFRelease(event);posts.clear();
    appDelegate=[AppDelegate new];
    _frontMostApp=@"org.openkey.test.rule";
    _appLanguageRule=0;vLanguage=1;ApplyAppLanguageSettings();assert(vLanguage==0);
    switchLanguage();assert(vLanguage==0); // Fixed English rejects shortcut overrides.
    _appLanguageRule=1;ApplyAppLanguageSettings();assert(vLanguage==1);
    switchLanguage();assert(vLanguage==1);
    setAppInputMethodStatus("org.openkey.test.rule",1|(3<<1));
    _appLanguageRule=0;vUseSmartSwitchKey=1;vRememberCode=1;
    ApplyAppLanguageSettings();assert(vLanguage==0 && vCodeTable==3); // Language rule preserves encoding memory.
    vRememberCode=0;vCodeTable=0;
    _appLanguageRule=-1;vUseSmartSwitchKey=0;vLanguage=0;
    ApplyAppLanguageSettings();assert(vLanguage==0); // Inherit disabled global remembering.
    setAppInputMethodStatus("org.openkey.test.rule",1);
    _appLanguageRule=2;ApplyAppLanguageSettings();assert(vLanguage==1);
    // Exercise the complete shortcut -> save -> leave -> restore path, not just a seeded lookup.
    testAppID=@"org.openkey.test.remember";
    [[NSUserDefaults standardUserDefaults] setObject:@{testAppID:@2} forKey:@"AppLanguageRules"];
    setAppInputMethodStatus(testAppID.UTF8String,1);
    vLanguage=1;vUseSmartSwitchKey=0;OnActiveAppChanged();
    vSwitchKeyStatus=0x7A000206; // Option-Z, no beep.
    auto shortcut=[&](CGEventType type,CGKeyCode key,CGEventFlags flags) {
        CGEventRef event=CGEventCreateKeyboardEvent(NULL,key,true);
        CGEventSetType(event,type);CGEventSetFlags(event,flags);
        OpenKeyCallback(NULL,type,event,NULL);CFRelease(event);
    };
    shortcut(kCGEventFlagsChanged,kVK_Option,kCGEventFlagMaskAlternate);
    shortcut(kCGEventKeyDown,kVK_ANSI_Z,kCGEventFlagMaskAlternate);
    shortcut(kCGEventKeyUp,kVK_ANSI_Z,kCGEventFlagMaskAlternate);
    shortcut(kCGEventFlagsChanged,kVK_Option,0);
    assert(vLanguage==0);
    assert(getAppInputMethodStatus(testAppID.UTF8String,1)==0);
    testAppID=@"org.openkey.test.other";OnActiveAppChanged();
    [appDelegate setInputLanguage:1];
    testAppID=@"org.openkey.test.remember";OnActiveAppChanged();
    assert(vLanguage==0);
    // A shortcut can arrive before the workspace activation notification. Resolve the
    // current app and its rule together instead of rejecting it using the old app's rule.
    testAppID=@"org.openkey.test.other"; OnActiveAppChanged();
    testAppID=@"org.openkey.test.remember"; setAppInputMethodStatus(testAppID.UTF8String,1); vLanguage=0;
    OnInputMethodChanged();
    assert(getAppInputMethodStatus(testAppID.UTF8String,1)==0);
    // A stale remembered-app rule must not save a choice for a fixed-language app.
    [[NSUserDefaults standardUserDefaults] setObject:@{testAppID:@2, @"org.openkey.test.fixed":@1} forKey:@"AppLanguageRules"];
    OnActiveAppChanged();
    testAppID=@"org.openkey.test.fixed"; setAppInputMethodStatus(testAppID.UTF8String,1);
    vLanguage=0; OnInputMethodChanged();
    assert(getAppInputMethodStatus(testAppID.UTF8String,1)==1);
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"AppLanguageRules"];
    vUseSmartSwitchKey=0; vRememberCode=0; vCodeTable=0;
    _appLanguageRule=-1; vLanguage=1; vSwitchKeyStatus=0x7A000206;
    SetShortcutRecording(YES);
    shortcut(kCGEventFlagsChanged,kVK_Option,kCGEventFlagMaskAlternate);
    shortcut(kCGEventKeyDown,kVK_ANSI_Z,kCGEventFlagMaskAlternate);
    assert(vLanguage==1); // Recording must not trigger the global shortcut first.
    SetShortcutRecording(NO);
    shortcut(kCGEventFlagsChanged,kVK_Option,kCGEventFlagMaskAlternate);
    shortcut(kCGEventKeyDown,kVK_ANSI_Z,kCGEventFlagMaskAlternate);
    assert(vLanguage==0);
    vSwitchKeyStatus=0xFE0000FE;
    for (CGEventFlags flags:{CGEventFlags(kCGEventFlagMaskSecondaryFn),CGEventFlags(0)}) {
        CGEventRef event=CGEventCreateKeyboardEvent(NULL,kVK_Function,true);
        CGEventSetType(event,kCGEventFlagsChanged); CGEventSetFlags(event,flags);
        assert(OpenKeyCallback(NULL,kCGEventFlagsChanged,event,NULL)==event); CFRelease(event);
    }
    assert(vLanguage==0); // Fn/Globe belongs to macOS.
    vCodeTable=3;_syncKey.clear();SendBackspace();SendShiftAndLeftArrow();
    assert(posts.size()==4); // Empty tracking state must not dereference vector::back().
    for(auto event:posts)CFRelease(event);
    CFRelease(myEventSource);
    addMacro("legacy","giữ dữ liệu"); vector<Byte> macros; getMacroSaveData(macros);
    NSData *original=[NSData dataWithBytes:macros.data() length:macros.size()];
    [[NSUserDefaults standardUserDefaults] setObject:original forKey:@"macroData"];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"UseMacro"];
    OpenKeyInit();
    assert(!vUseMacro && !vUseMacroInEnglishMode && hasMacro("legacy"));
    assert([[[NSUserDefaults standardUserDefaults] dataForKey:@"macroData"] isEqual:original]);
    CFRelease(myEventSource);
    [[NSUserDefaults standardUserDefaults] removePersistentDomainForName:testSuite];
    puts("Native input: timeout/user-disable, target routing, Unicode delivery, Spotlight ownership, app rules, shortcut recording, native Fn, macro retirement and empty buffers passed");
}}
