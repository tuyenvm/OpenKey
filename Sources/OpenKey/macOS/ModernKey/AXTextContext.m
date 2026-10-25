//
//  AXTextContext.m
//  OpenKey
//
//  Read the text around the caret of the focused text field with the Accessibility API.
//

#import <ApplicationServices/ApplicationServices.h>
#import <os/log.h>
#import "AXTextContext.h"

#define AX_TIMEOUT 0.15f //seconds; a stuck app must not hold us up
#define AX_MAX_FALLBACK_LENGTH 100000 //do not pull a whole big document just to look at the caret

//apps whose accessibility text is not "the text before the caret" (terminals) or that we must stay out of
static BOOL isDeniedApp(NSString* bundleId) {
    static NSSet* denied;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        denied = [NSSet setWithObjects:@"com.apple.Terminal", @"com.googlecode.iterm2", @"dev.warp.Warp-Stable",
                  @"net.kovidgoyal.kitty", @"com.mitchellh.ghostty", @"org.alacritty", @"io.alacritty",
                  @"com.github.wez.wezterm", @"co.zeit.hyper", nil];
    });
    return bundleId != nil && ([denied containsObject:bundleId] || [bundleId isEqualToString:[[NSBundle mainBundle] bundleIdentifier]]);
}

static void logStep(NSString* bundleId, const char* step, AXError error) {
    if (![[NSUserDefaults standardUserDefaults] boolForKey:@"vSeedFromCaretLog"])
        return;
    os_log(OS_LOG_DEFAULT, "seed %{public}@ %{public}s -> AXError %d", bundleId, step, (int)error);
}

//Chromium and Electron only build their accessibility tree for a client that asks for it, and Firefox
//answers to AXEnhancedUserInterface. Other apps do not support these attributes, so they are left alone.
//Returns YES when something was switched on and a retry is worth it.
static NSMutableSet* enabledPids; //only used from the serial queue that calls AXReadCaretContext
static BOOL switchOnAccessibility(AXUIElementRef app, pid_t pid, NSString* bundleId) {
    if (enabledPids == nil)
        enabledPids = [NSMutableSet new];
    NSNumber* pidKey = @(pid);
    if ([enabledPids containsObject:pidKey])
        return NO;
    [enabledPids addObject:pidKey];

    BOOL changed = AXUIElementSetAttributeValue(app, CFSTR("AXManualAccessibility"), kCFBooleanTrue) == kAXErrorSuccess;
    if ([bundleId hasPrefix:@"org.mozilla."] || [bundleId hasPrefix:@"app.zen-browser."]) {
        changed |= AXUIElementSetAttributeValue(app, CFSTR("AXEnhancedUserInterface"), kCFBooleanTrue) == kAXErrorSuccess;
    }
    logStep(bundleId, changed ? "accessibility switched on" : "accessibility not supported", kAXErrorSuccess);
    return changed;
}

//the focused element of the app; the system-wide one is only a fallback, it often fails with Chromium
static AXUIElementRef copyFocusedElement(AXUIElementRef app, pid_t pid, NSString* bundleId) {
    CFTypeRef focused = NULL;
    AXError error = AXUIElementCopyAttributeValue(app, kAXFocusedUIElementAttribute, &focused);
    if (error == kAXErrorSuccess && focused != NULL && CFGetTypeID(focused) == AXUIElementGetTypeID())
        return (AXUIElementRef)focused;
    if (focused != NULL)
        CFRelease(focused);
    logStep(bundleId, "focused element of the app", error);

    AXUIElementRef systemWide = AXUIElementCreateSystemWide();
    AXUIElementSetMessagingTimeout(systemWide, AX_TIMEOUT);
    focused = NULL;
    error = AXUIElementCopyAttributeValue(systemWide, kAXFocusedUIElementAttribute, &focused);
    CFRelease(systemWide);
    if (error == kAXErrorSuccess && focused != NULL && CFGetTypeID(focused) == AXUIElementGetTypeID()) {
        pid_t owner = 0;
        if (AXUIElementGetPid((AXUIElementRef)focused, &owner) == kAXErrorSuccess && owner == pid)
            return (AXUIElementRef)focused;
    }
    if (focused != NULL)
        CFRelease(focused);
    logStep(bundleId, "focused element (system wide)", error);
    return NULL;
}

//does the element cover the screen point? YES when the app does not tell where the element is
static BOOL frameContainsPoint(AXUIElementRef element, CGPoint point) {
    CFTypeRef positionValue = NULL, sizeValue = NULL;
    CGPoint position = CGPointZero;
    CGSize size = CGSizeZero;
    BOOL known = NO;
    if (AXUIElementCopyAttributeValue(element, kAXPositionAttribute, &positionValue) == kAXErrorSuccess && positionValue != NULL &&
        AXUIElementCopyAttributeValue(element, kAXSizeAttribute, &sizeValue) == kAXErrorSuccess && sizeValue != NULL) {
        known = CFGetTypeID(positionValue) == AXValueGetTypeID() && CFGetTypeID(sizeValue) == AXValueGetTypeID() &&
                AXValueGetValue(positionValue, kAXValueTypeCGPoint, &position) && AXValueGetValue(sizeValue, kAXValueTypeCGSize, &size);
    }
    if (positionValue != NULL)
        CFRelease(positionValue);
    if (sizeValue != NULL)
        CFRelease(sizeValue);
    if (!known || size.width <= 0 || size.height <= 0)
        return YES;
    return CGRectContainsPoint(CGRectInset(CGRectMake(position.x, position.y, size.width, size.height), -3, -3), point);
}

static BOOL isSecureField(AXUIElementRef element) {
    CFTypeRef subrole = NULL;
    BOOL secure = NO;
    if (AXUIElementCopyAttributeValue(element, kAXSubroleAttribute, &subrole) == kAXErrorSuccess && subrole != NULL) {
        secure = CFGetTypeID(subrole) == CFStringGetTypeID() && CFEqual(subrole, kAXSecureTextFieldSubrole);
        CFRelease(subrole);
    }
    return secure;
}

static AXError copySelectedRange(AXUIElementRef element, CFRange* range, NSString* bundleId) {
    CFTypeRef value = NULL;
    AXError error = AXUIElementCopyAttributeValue(element, kAXSelectedTextRangeAttribute, &value);
    if (error != kAXErrorSuccess || value == NULL) {
        logStep(bundleId, "selected text range", error);
        return error != kAXErrorSuccess ? error : kAXErrorNoValue;
    }
    BOOL ok = CFGetTypeID(value) == AXValueGetTypeID() && AXValueGetValue(value, kAXValueTypeCFRange, range);
    CFRelease(value);
    return ok ? kAXErrorSuccess : kAXErrorFailure;
}

//a piece of the text, as the app itself reports it
static NSString* copyStringForRange(AXUIElementRef element, CFRange range, NSString* bundleId) {
    if (range.length <= 0)
        return @"";
    AXValueRef rangeValue = AXValueCreate(kAXValueTypeCFRange, &range);
    CFTypeRef result = NULL;
    AXError error = AXUIElementCopyParameterizedAttributeValue(element, kAXStringForRangeParameterizedAttribute, rangeValue, &result);
    CFRelease(rangeValue);
    if (error == kAXErrorSuccess && result != NULL && CFGetTypeID(result) == CFStringGetTypeID())
        return CFBridgingRelease(result);
    if (result != NULL)
        CFRelease(result);
    logStep(bundleId, "string for range", error);
    return nil;
}

//number of characters of the text field, -1 when the app does not tell
static CFIndex characterCount(AXUIElementRef element) {
    CFTypeRef count = NULL;
    if (AXUIElementCopyAttributeValue(element, kAXNumberOfCharactersAttribute, &count) != kAXErrorSuccess || count == NULL)
        return -1;
    CFIndex total = -1;
    if (CFGetTypeID(count) == CFNumberGetTypeID())
        CFNumberGetValue(count, kCFNumberCFIndexType, &total);
    CFRelease(count);
    return total;
}

//fallback for apps without AXStringForRange: cut the piece out of the whole value
static NSString* copyStringFromValue(AXUIElementRef element, CFRange range, NSString* bundleId) {
    CFIndex total = characterCount(element);
    if (total < 0 || total > AX_MAX_FALLBACK_LENGTH)
        return nil;

    CFTypeRef value = NULL;
    AXError error = AXUIElementCopyAttributeValue(element, kAXValueAttribute, &value);
    if (error != kAXErrorSuccess || value == NULL) {
        logStep(bundleId, "value", error);
        return nil;
    }
    if (CFGetTypeID(value) != CFStringGetTypeID()) {
        CFRelease(value);
        return nil;
    }
    NSString* text = CFBridgingRelease(value);
    NSUInteger start = MIN((NSUInteger)range.location, text.length);
    NSUInteger end = MIN((NSUInteger)(range.location + range.length), text.length);
    return [text substringWithRange:NSMakeRange(start, end - start)];
}

//do not start or end in the middle of a surrogate pair
static NSString* trimSurrogates(NSString* text) {
    if (text.length > 0 && CFStringIsSurrogateLowCharacter([text characterAtIndex:0]))
        text = [text substringFromIndex:1];
    if (text.length > 0 && CFStringIsSurrogateHighCharacter([text characterAtIndex:text.length - 1]))
        text = [text substringToIndex:text.length - 1];
    return text;
}

AXContextStatus AXReadCaretContext(pid_t pid, NSString* bundleId, const CGPoint* clickPoint, NSString** before, NSString** after) {
    *before = nil;
    *after = nil;
    if (isDeniedApp(bundleId))
        return AXContextSkipped;

    AXUIElementRef app = AXUIElementCreateApplication(pid);
    if (app == NULL)
        return AXContextUnavailable;
    AXUIElementSetMessagingTimeout(app, AX_TIMEOUT);

    AXContextStatus status = AXContextUnavailable;
    AXUIElementRef focused = copyFocusedElement(app, pid, bundleId);
    if (focused == NULL) {
        if (switchOnAccessibility(app, pid, bundleId))
            status = AXContextRetry;
        CFRelease(app);
        return status;
    }
    AXUIElementSetMessagingTimeout(focused, AX_TIMEOUT);

    CFRange caret = CFRangeMake(0, 0);
    AXError rangeError = kAXErrorSuccess;
    if (clickPoint != NULL && !frameContainsPoint(focused, *clickPoint)) {
        logStep(bundleId, "focus is not under the click yet", kAXErrorSuccess);
        status = AXContextNotYet;
    } else if (isSecureField(focused)) {
        logStep(bundleId, "secure field, skipped", kAXErrorSuccess);
        status = AXContextSkipped;
    } else if ((rangeError = copySelectedRange(focused, &caret, bundleId)) != kAXErrorSuccess) {
        if (switchOnAccessibility(app, pid, bundleId))
            status = AXContextRetry;
        else if (rangeError == kAXErrorNoValue || rangeError == kAXErrorAttributeUnsupported)
            status = AXContextSkipped; //the app answers, and this element is not a text field
    } else if (caret.length != 0 || caret.location < 0) {
        logStep(bundleId, "text is selected, skipped", kAXErrorSuccess);
        status = AXContextSkipped;
    } else {
        CFIndex start = MAX(0, caret.location - AX_CONTEXT_LENGTH);
        CFRange beforeRange = CFRangeMake(start, caret.location - start);
        NSString* text = beforeRange.length == 0 ? @"" : copyStringForRange(focused, beforeRange, bundleId);
        if (text == nil)
            text = copyStringFromValue(focused, beforeRange, bundleId);
        if (text != nil) {
            *before = [trimSurrogates(text) precomposedStringWithCanonicalMapping];
            //the part after the caret is only looked at to know where the caret is, it is best effort.
            //AXStringForRange fails for a range that goes past the end, so ask only for what is there
            CFIndex total = characterCount(focused);
            CFIndex available = total >= 0 ? MAX(0, total - caret.location) : AX_CONTEXT_LENGTH;
            CFRange afterRange = CFRangeMake(caret.location, MIN(available, AX_CONTEXT_LENGTH));
            NSString* tail = afterRange.length == 0 ? @"" : copyStringForRange(focused, afterRange, bundleId);
            if (tail == nil)
                tail = copyStringFromValue(focused, afterRange, bundleId);
            *after = tail != nil ? [trimSurrogates(tail) precomposedStringWithCanonicalMapping] : @"";
            status = AXContextOK;
        }
    }

    CFRelease(focused);
    CFRelease(app);
    return status;
}
