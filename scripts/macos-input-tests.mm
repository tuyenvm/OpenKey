// Native regression checks. Event posting is intercepted: this test sends no UI input.
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
    vCodeTable=3;_syncKey.clear();SendBackspace();SendShiftAndLeftArrow();
    assert(posts.size()==4); // Empty tracking state must not dereference vector::back().
    for(auto event:posts)CFRelease(event);
    CFRelease(myEventSource);
    puts("Native input: timeout/user-disable, target routing, Unicode delivery, Spotlight ownership and empty buffers passed");
}}
