#import <Cocoa/Cocoa.h>
#import "MyTextField.h"

@interface SettingsWindowController : NSWindowController <NSWindowDelegate, NSToolbarDelegate, NSTableViewDataSource, NSTableViewDelegate, NSSearchFieldDelegate, MyTextFieldDelegate>
- (void)showPage:(NSString *)page;
@end
