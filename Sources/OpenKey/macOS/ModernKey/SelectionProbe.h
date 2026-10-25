//
//  SelectionProbe.h
//  OpenKey
//
//  Last resort to read the text before the caret in an app that does not answer the Accessibility API:
//  select it with Shift+Left, copy it, put the caret back with Right, and give the clipboard back.
//  Everything here runs on the main thread.
//

#ifndef SelectionProbe_h
#define SelectionProbe_h

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Called when the probe is over. before is the NFC text right before the caret (at most the number of
 * characters asked for), or nil when it could not be read. It runs before the keys typed meanwhile are
 * replayed, so the engine can be seeded in time for them.
 */
typedef void (^SelectionProbeCompletion)(NSString* before);

/**
 * Apps where sending Shift+Left / Cmd+C / Right is not harmless: terminals, file lists, spreadsheets...
 */
BOOL SelectionProbeAllowedFor(NSString* bundleId);

BOOL SelectionProbeIsRunning(void);

/**
 * Select the last `count` characters before the caret of the app `pid` and read them.
 * The caret and the clipboard are put back. source must be the event source OpenKey ignores in its own
 * event tap. Does nothing (completion gets nil) when the app is not in front, a modifier key is held, or
 * the clipboard is too big to be given back safely.
 */
void SelectionProbeStart(CGEventSourceRef source, pid_t pid, int count, SelectionProbeCompletion done);

/**
 * While the probe runs, what the user types would replace the selected text. The event tap hands every
 * key event to this function; when it returns YES the event was kept and must be swallowed. Kept events
 * are posted again, in order, as soon as the probe is over.
 */
BOOL SelectionProbeHoldEvent(CGEventRef event);

/** YES for an event posted again after the probe: let it through as a normal key. */
BOOL SelectionProbeIsReplay(CGEventRef event);

/** The user clicked while the probe was running: the selection is gone, do not move the caret anymore. */
void SelectionProbeAbort(void);

#ifdef __cplusplus
}
#endif

#endif /* SelectionProbe_h */
