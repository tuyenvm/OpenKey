//
//  AXTextContext.h
//  OpenKey
//
//  Read the text around the caret of the focused text field with the Accessibility API.
//

#ifndef AXTextContext_h
#define AXTextContext_h

#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Number of characters read before the caret: one more than the longest word the engine takes (10),
 * so the start of that word can be told from the middle of a longer one.
 */
#define AX_CONTEXT_LENGTH 11

typedef enum {
    AXContextOK,          //before and after are filled
    AXContextRetry,       //accessibility was just switched on for this app, try again shortly
    AXContextNotYet,      //the focus is not on what was clicked yet (or is still moving): try again in a moment
    AXContextUnavailable, //the app does not answer the Accessibility API; another way of reading may work
    AXContextSkipped      //must not be read at all (password, terminal, text is selected) or is not a text field
} AXContextStatus;

/**
 * Read the text right before and right after the caret (NFC) of the focused text field of an application.
 * It talks to another process and can block for a moment, so never call it from the event tap callback.
 * Only call it from one serial queue.
 * clickPoint (screen coordinates, may be NULL): where the user clicked. The app moves its focus a moment after
 * the click, so right after it the focused field can still be the previous one; reading that would give the
 * text of another field. When the focused field does not contain the point, the answer is AXContextNotYet.
 * It never reads password fields or terminals, and nothing is written to the log except the bundle id
 * and Accessibility error codes (turn it on with: defaults write com.tuyenmai.openkey vSeedFromCaretLog -bool YES).
 */
AXContextStatus AXReadCaretContext(pid_t pid, NSString* bundleId, const CGPoint* clickPoint, NSString** before, NSString** after);

#ifdef __cplusplus
}
#endif

#endif /* AXTextContext_h */
