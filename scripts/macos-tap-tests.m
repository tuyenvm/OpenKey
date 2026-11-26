// Test the real manager lifecycle with a local Mach port. No keyboard tap/input is created.
#define CGEventTapCreate FakeTapCreate
#define CGEventTapEnable FakeTapEnable
#define CGEventTapIsEnabled FakeTapEnabled
#define MJAccessibilityIsEnabled FakePermission
#define IsSecureEventInputEnabled FakeSecureInput
#include "../Sources/OpenKey/macOS/ModernKey/OpenKeyManager.m"
#include <assert.h>
static BOOL trusted=YES,enabled=YES,secure=NO;
static int creates=0,initializations=0,resets=0;
static CFMachPortRef port;
BOOL FakePermission(void) {return trusted;}
Boolean FakeSecureInput(void) {return secure;}
void FakeTapEnable(CFMachPortRef tap,bool value) {enabled=value;}
bool FakeTapEnabled(CFMachPortRef tap) {return enabled;}
CFMachPortRef FakeTapCreate(CGEventTapLocation location,CGEventTapPlacement placement,CGEventTapOptions options,CGEventMask mask,CGEventTapCallBack callback,void *userInfo) {
    ++creates;Boolean fresh=NO;port=CFMachPortCreate(NULL,NULL,NULL,&fresh);return port;
}
void OpenKeyInit(void) {++initializations;}
void ResetInputSession(void) {++resets;}
void RefreshInputContext(void) {}
NSString *ConvertUtil(NSString *str) {return str;}
CGEventRef OpenKeyCallback(CGEventTapProxy proxy,CGEventType type,CGEventRef event,void *refcon) {return event;}
int main(void) {@autoreleasepool {
    assert([OpenKeyManager initEventTap]);assert(initializations==1 && creates==1);
    enabled=NO;[OpenKeyManager recoverEventTap];assert(enabled);
    [OpenKeyManager handleTapDisabledByUser];enabled=NO;
    [OpenKeyManager recoverEventTap];assert(!enabled);
    [OpenKeyManager stopEventTap];assert(![OpenKeyManager resumeEventTap]);
    assert([OpenKeyManager initEventTap]);assert(enabled);
    secure=YES;enabled=NO;[OpenKeyManager recoverEventTap];assert(!enabled);
    secure=NO;[OpenKeyManager recoverEventTap];assert(enabled);
    CFMachPortInvalidate(port);
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,1.1,false);
    assert([OpenKeyManager isActive] && creates==3 && initializations==1);
    trusted=NO;
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,1.1,false);
    assert(![OpenKeyManager isInited]);
    trusted=YES;assert([OpenKeyManager initEventTap]);assert(initializations==1);
    [OpenKeyManager stopEventTap];int stoppedAt=creates;
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,1.1,false);
    assert(![OpenKeyManager isInited] && creates==stoppedAt && resets>0);
    puts("Tap lifecycle: recovery, explicit pause, secure input, port recreation, permission revocation and stop passed");
}}
