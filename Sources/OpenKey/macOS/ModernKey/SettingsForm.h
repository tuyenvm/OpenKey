#import <Cocoa/Cocoa.h>

API_AVAILABLE(macos(13.0))
@interface OKSettingsFormController : NSViewController
- (instancetype)initWithEntries:(NSArray<NSDictionary *> *)entries;
- (void)refreshControls;
@end
