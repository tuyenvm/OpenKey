#import <Cocoa/Cocoa.h>
#import "MyTextField.h"

@interface SettingsWindowController : NSWindowController <NSWindowDelegate, NSTableViewDataSource, NSTableViewDelegate, MyTextFieldDelegate>
- (void)showPage:(NSString *)page;
- (void)showConversion;
- (void)openTextReplacements:(id)sender;
@end
