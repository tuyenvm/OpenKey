#import <Cocoa/Cocoa.h>
#import "MyTextField.h"

@interface SettingsWindowController : NSWindowController <NSWindowDelegate, NSTableViewDataSource, NSTableViewDelegate, NSSearchFieldDelegate, MyTextFieldDelegate>
- (void)showPage:(NSString *)page;
@end
