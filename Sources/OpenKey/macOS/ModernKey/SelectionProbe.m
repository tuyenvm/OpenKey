//
//  SelectionProbe.m
//  OpenKey
//
//  Last resort to read the text before the caret in an app that does not answer the Accessibility API:
//  select it with Shift+Left, copy it, put the caret back with Right, and give the clipboard back.
//

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>
#import "SelectionProbe.h"

#define PROBE_REPLAY_MAGIC 0x4F4B5052 //'OKPR': marks the key events posted again after the probe
#define PROBE_COPY_DELAY_MS 35 //let the app finish selecting before copying
#define PROBE_POLL_MS 15
#define PROBE_POLL_COUNT 16 //about 240 ms for the app to answer the copy (an empty field never does, and waits for it)
#define PROBE_RETRY_AT 8 //no answer after about half of it: the copy may have been missed, ask once more
#define PROBE_COLLAPSE_MS 30 //let the app move the caret before the clipboard is given back
#define PROBE_MAX_CLIPBOARD_BYTES 1000000 //a bigger clipboard can not be given back quickly and safely

static BOOL running = NO;
static BOOL aborted = NO;
static NSMutableArray* heldEvents = nil;

BOOL SelectionProbeAllowedFor(NSString* bundleId) {
    static NSSet* denied;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        denied = [NSSet setWithObjects:
                  //terminals: Shift+Left and Cmd+C are not editing keys there
                  @"com.apple.Terminal", @"com.googlecode.iterm2", @"dev.warp.Warp-Stable", @"net.kovidgoyal.kitty",
                  @"com.mitchellh.ghostty", @"org.alacritty", @"io.alacritty", @"com.github.wez.wezterm", @"co.zeit.hyper",
                  //Shift+Left selects items or cells, not characters
                  @"com.apple.finder", @"com.apple.Spotlight", @"com.microsoft.Excel", @"com.apple.iWork.Numbers",
                  //OpenKey itself
                  @"com.tuyenmai.openkey", nil];
    });
    return bundleId != nil && ![denied containsObject:bundleId];
}

BOOL SelectionProbeIsRunning(void) {
    return running;
}

BOOL SelectionProbeIsReplay(CGEventRef event) {
    return CGEventGetIntegerValueField(event, kCGEventSourceUserData) == PROBE_REPLAY_MAGIC;
}

BOOL SelectionProbeHoldEvent(CGEventRef event) {
    if (!running)
        return NO;
    CGEventRef copy = CGEventCreateCopy(event);
    if (copy == NULL)
        return NO;
    [heldEvents addObject:(__bridge id)copy];
    CFRelease(copy);
    return YES;
}

void SelectionProbeAbort(void) {
    if (running)
        aborted = YES;
}

static BOOL isFrontmost(pid_t pid) {
    return [[NSWorkspace sharedWorkspace] frontmostApplication].processIdentifier == pid;
}

//the real keyboard, not the combined state: that one also holds the modifiers of the key events posted here
static BOOL modifiersHeld(void) {
    CGEventFlags flags = CGEventSourceFlagsState(kCGEventSourceStateHIDSystemState);
    return (flags & (kCGEventFlagMaskShift | kCGEventFlagMaskControl | kCGEventFlagMaskAlternate | kCGEventFlagMaskCommand)) != 0;
}

static void after(int ms, dispatch_block_t block) {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)ms * NSEC_PER_MSEC), dispatch_get_main_queue(), block);
}

static void postKeyEvent(CGEventSourceRef source, CGKeyCode key, bool down, CGEventFlags flags) {
    CGEventRef event = CGEventCreateKeyboardEvent(source, key, down);
    if (event == NULL)
        return;
    CGEventSetFlags(event, flags);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
}

//Taps `key` `times` times, with `modifierKey` held when there is one, the way a keyboard does it. The last
//event carries no flags: the system keeps the flags of the last event it was given, so a modifier that is
//not released explicitly would stay "down" for the next real key press.
static void postChord(CGEventSourceRef source, CGKeyCode modifierKey, CGEventFlags flags, CGKeyCode key, int times) {
    if (flags != 0)
        postKeyEvent(source, modifierKey, true, flags);
    for (int i = 0; i < times; i++) {
        postKeyEvent(source, key, true, flags);
        postKeyEvent(source, key, false, flags);
    }
    if (flags != 0)
        postKeyEvent(source, modifierKey, false, 0);
}

//The key that types `character` while Command is held, under the keyboard layout `layout`: this is how an app
//finds its shortcuts (Cmd+C is the key that types "c", which is not the QWERTY C key on Dvorak, for example).
static CGKeyCode commandKeyCodeInLayout(const UCKeyboardLayout* layout, UInt32 keyboardType, UniChar character, CGKeyCode fallback) {
    if (layout == NULL)
        return fallback;
    for (CGKeyCode code = 0; code < 128; code++) {
        UInt32 deadKeys = 0;
        UniCharCount length = 0;
        UniChar typed[4];
        OSStatus status = UCKeyTranslate(layout, code, kUCKeyActionDown, (cmdKey >> 8) & 0xFF, keyboardType,
                                         kUCKeyTranslateNoDeadKeysBit, &deadKeys, 4, &length, typed);
        if (status == noErr && length == 1 && typed[0] == character)
            return code;
    }
    return fallback;
}

static CGKeyCode commandKeyCodeForCurrentLayout(CGEventSourceRef source, UniChar character, CGKeyCode fallback) {
    CGKeyCode code = fallback;
    TISInputSourceRef layout = TISCopyCurrentKeyboardLayoutInputSource();
    if (layout != NULL) {
        CFDataRef data = (CFDataRef)TISGetInputSourceProperty(layout, kTISPropertyUnicodeKeyLayoutData);
        if (data != NULL)
            code = commandKeyCodeInLayout((const UCKeyboardLayout*)CFDataGetBytePtr(data), CGEventSourceGetKeyboardType(source), character, fallback);
        CFRelease(layout);
    }
    return code;
}

//every item of the clipboard with all its types, or nil when it is too big to be given back
static NSArray* copyPasteboard(NSPasteboard* pasteboard) {
    NSMutableArray* saved = [NSMutableArray array];
    NSUInteger bytes = 0;
    for (NSPasteboardItem* item in pasteboard.pasteboardItems) {
        NSMutableDictionary* kept = [NSMutableDictionary dictionary];
        for (NSPasteboardType type in item.types) {
            NSData* data = [item dataForType:type];
            if (data == nil)
                continue;
            bytes += data.length;
            if (bytes > PROBE_MAX_CLIPBOARD_BYTES)
                return nil;
            kept[type] = data;
        }
        [saved addObject:kept];
    }
    return saved;
}

static void restorePasteboard(NSPasteboard* pasteboard, NSArray* saved) {
    [pasteboard clearContents];
    NSMutableArray* items = [NSMutableArray array];
    for (NSDictionary* kept in saved) {
        NSPasteboardItem* item = [NSPasteboardItem new];
        for (NSPasteboardType type in kept) {
            [item setData:kept[type] forType:type];
        }
        [items addObject:item];
    }
    if (items.count > 0)
        [pasteboard writeObjects:items];
}

//what was copied has to look like "the last few characters before the caret", else it is something else
//(for example an editor that copies the whole line when nothing is selected)
static NSString* validatedText(NSString* text, int count) {
    if (text.length == 0)
        return nil;
    unichar last = [text characterAtIndex:text.length - 1];
    if (last == '\n' || last == '\r' || last == 0x2028 || last == 0x2029)
        return nil;
    __block int characters = 0;
    [text enumerateSubstringsInRange:NSMakeRange(0, text.length)
                             options:NSStringEnumerationByComposedCharacterSequences
                          usingBlock:^(NSString* s, NSRange r, NSRange er, BOOL* stop) { characters++; }];
    if (characters > count)
        return nil;
    return [text precomposedStringWithCanonicalMapping];
}

static void replayEvents(NSArray* events) {
    for (id item in events) {
        CGEventRef event = (__bridge CGEventRef)item;
        CGEventSetIntegerValueField(event, kCGEventSourceUserData, PROBE_REPLAY_MAGIC);
        CGEventPost(kCGHIDEventTap, event);
    }
}

@interface ProbeRun : NSObject
@property (nonatomic) CGEventSourceRef source; //owned by the caller
@property (nonatomic) pid_t pid;
@property (nonatomic) int count;
@property (nonatomic, copy) SelectionProbeCompletion done;
@property (nonatomic, strong) NSPasteboard* pasteboard;
@property (nonatomic, strong) NSArray* saved;
@property (nonatomic) NSInteger changeCount;
@property (nonatomic) BOOL askedAgain;
@property (nonatomic) CGKeyCode copyKey; //the key of Cmd+C under the current keyboard layout
@end

@implementation ProbeRun

- (void)start {
    postChord(self.source, kVK_Shift, kCGEventFlagMaskShift, kVK_LeftArrow, self.count);
    after(PROBE_COPY_DELAY_MS, ^{ [self copySelection]; });
}

- (void)copySelection {
    if (aborted || !isFrontmost(self.pid)) {
        [self finish:nil];
        return;
    }
    postChord(self.source, kVK_Command, kCGEventFlagMaskCommand, self.copyKey, 1);
    [self waitForCopy:PROBE_POLL_COUNT];
}

- (void)waitForCopy:(int)left {
    if (self.pasteboard.changeCount != self.changeCount) {
        [self collapse:[self.pasteboard stringForType:NSPasteboardTypeString]];
    } else if (left <= 0 || aborted) {
        //nothing was copied: nothing was selected (start of the text) or the app does not copy. In the second
        //case the selection stays, which is why the probe is only used where something says there is text.
        [self finish:nil];
    } else {
        if (left == PROBE_RETRY_AT && !self.askedAgain && isFrontmost(self.pid)) {
            self.askedAgain = YES;
            postChord(self.source, kVK_Command, kCGEventFlagMaskCommand, self.copyKey, 1);
        }
        after(PROBE_POLL_MS, ^{ [self waitForCopy:left - 1]; });
    }
}

//Right collapses the selection onto its right end, which is where the caret was
- (void)collapse:(NSString*)text {
    if (!aborted && isFrontmost(self.pid))
        postChord(self.source, 0, 0, kVK_RightArrow, 1);
    after(PROBE_COLLAPSE_MS, ^{ [self finish:text]; });
}

//give the clipboard back, hand over the result, then let the keys typed meanwhile through
- (void)finish:(NSString*)text {
    restorePasteboard(self.pasteboard, self.saved);
    NSArray* held = heldEvents;
    heldEvents = nil;
    running = NO;
    self.done(validatedText(text, self.count));
    replayEvents(held);
}

@end

void SelectionProbeStart(CGEventSourceRef source, pid_t pid, int count, SelectionProbeCompletion done) {
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    NSArray* saved = nil;
    if (!running && source != NULL && count > 0 && isFrontmost(pid) && !modifiersHeld())
        saved = copyPasteboard(pasteboard);
    if (saved == nil) {
        done(nil);
        return;
    }

    running = YES;
    aborted = NO;
    heldEvents = [NSMutableArray array];

    ProbeRun* run = [ProbeRun new];
    run.source = source;
    run.pid = pid;
    run.count = count;
    run.done = done;
    run.pasteboard = pasteboard;
    run.saved = saved;
    run.changeCount = pasteboard.changeCount;
    run.copyKey = commandKeyCodeForCurrentLayout(source, 'c', kVK_ANSI_C);
    [run start];
}
