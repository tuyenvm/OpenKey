#import "SettingsWindowController.h"
#import "AppDelegate.h"
#import "OpenKeyManager.h"
#import "MJAccessibilityUtils.h"
#import <Carbon/Carbon.h>

extern AppDelegate *appDelegate;
extern int vLanguage, vInputType, vCodeTable, vUseSmartSwitchKey;
extern void OnActiveAppChanged(void);
extern void RequestNewSession(void);

@implementation SettingsWindowController {
    NSTextField *_status, *_testHint;
    NSPopUpButton *_inputType, *_encoding;
    NSSegmentedControl *_language;
    NSButton *_retry, *_remember, *_globe;
    NSTableView *_rulesTable;
    NSArray<NSString *> *_ruleIDs;
    NSTimer *_timer;
    BOOL _wasTrusted;
}

static NSTextField *Label(NSString *text, BOOL heading) {
    NSTextField *label = [NSTextField wrappingLabelWithString:text];
    label.font = heading ? [NSFont boldSystemFontOfSize:16] : [NSFont systemFontOfSize:13];
    if (!heading) label.textColor = NSColor.secondaryLabelColor;
    [label.widthAnchor constraintEqualToConstant:570].active = YES;
    return label;
}

- (NSButton *)button:(NSString *)title action:(SEL)action {
    return [NSButton buttonWithTitle:title target:self action:action];
}

- (NSStackView *)stack {
    NSStackView *stack = [NSStackView stackViewWithViews:@[]];
    stack.orientation = NSUserInterfaceLayoutOrientationVertical;
    stack.alignment = NSLayoutAttributeLeading;
    stack.spacing = 14;
    return stack;
}

- (void)addPage:(NSString *)title to:(NSTabView *)tabs stack:(NSStackView *)stack {
    NSTabViewItem *tab = [[NSTabViewItem alloc] initWithIdentifier:title];
    tab.label = title;
    NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 620, 460)];
    tab.view = view;
    [view addSubview:stack];
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
        [stack.leadingAnchor constraintEqualToAnchor:view.leadingAnchor constant:24],
        [stack.topAnchor constraintEqualToAnchor:view.topAnchor constant:24],
        [stack.trailingAnchor constraintLessThanOrEqualToAnchor:view.trailingAnchor constant:-24],
        [stack.bottomAnchor constraintLessThanOrEqualToAnchor:view.bottomAnchor constant:-20]
    ]];
    [tabs addTabViewItem:tab];
}

- (NSView *)row:(NSString *)title control:(NSView *)control {
    NSTextField *label = [NSTextField labelWithString:title];
    [label.widthAnchor constraintEqualToConstant:160].active = YES;
    return [NSStackView stackViewWithViews:@[label, control]];
}

- (instancetype)init {
    NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 640, 560)
        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable
        backing:NSBackingStoreBuffered defer:NO];
    self = [super initWithWindow:window];
    if (!self) return nil;
    window.title = @"OpenKey — Cài đặt";
    window.delegate = self;
    window.releasedWhenClosed = NO;
    [window center];
    NSTabView *tabs = [[NSTabView alloc] initWithFrame:NSInsetRect(window.contentView.bounds, 12, 12)];
    tabs.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [window.contentView addSubview:tabs];

    NSStackView *setup = [self stack];
    [setup addArrangedSubview:Label(@"Bắt đầu với OpenKey", YES)];
    [setup addArrangedSubview:Label(@"1. Cho phép OpenKey trong Cài đặt hệ thống → Quyền riêng tư & Bảo mật → Trợ năng. Quyền này cho phép OpenKey nhận và thay thế phím gõ trên toàn hệ thống.", NO)];
    _status = Label(@"", NO);
    [_status setAccessibilityIdentifier:@"OpenKeyInputStatus"];
    [setup addArrangedSubview:_status];
    _retry = [self button:@"Thử kết nối lại" action:@selector(retry:)];
    [setup addArrangedSubview:[NSStackView stackViewWithViews:@[
        [self button:@"Mở cài đặt Trợ năng…" action:@selector(openAccessibility:)], _retry]]];
    [setup addArrangedSubview:Label(@"2. Chọn nguồn nhập ABC của macOS để tránh hai bộ gõ cùng xử lý. Chọn Telex hoặc VNI trong thẻ Gõ tiếng Việt.", NO)];
    [setup addArrangedSubview:[self button:@"Mở cài đặt Bàn phím…" action:@selector(openKeyboard:)]];
    _testHint = Label(@"", NO);
    [setup addArrangedSubview:_testHint];
    NSTextField *test = [NSTextField textFieldWithString:@""];
    test.placeholderString = @"Gõ thử tại đây bằng bàn phím của bạn";
    [test setAccessibilityLabel:@"Gõ thử tiếng Việt"];
    [test.widthAnchor constraintEqualToConstant:570].active = YES;
    [setup addArrangedSubview:test];
    [setup addArrangedSubview:Label(@"Đối chiếu kết quả bằng mắt. Trạng thái kết nối không đảm bảo mọi ứng dụng đều tương thích.", NO)];
    [self addPage:@"Thiết lập" to:tabs stack:setup];

    NSStackView *typing = [self stack];
    [typing addArrangedSubview:Label(@"Gõ tiếng Việt", YES)];
    _language = [NSSegmentedControl segmentedControlWithLabels:@[@"Tiếng Việt", @"English"]
        trackingMode:NSSegmentSwitchTrackingSelectOne target:self action:@selector(changeLanguage:)];
    [typing addArrangedSubview:[self row:@"Ngôn ngữ" control:_language]];
    _inputType = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, 240, 26) pullsDown:NO];
    [_inputType addItemsWithTitles:@[@"Telex", @"VNI", @"Simple Telex 1", @"Simple Telex 2"]];
    [_inputType setAccessibilityLabel:@"Kiểu gõ"];
    _inputType.target = self; _inputType.action = @selector(changeInputType:);
    [typing addArrangedSubview:[self row:@"Kiểu gõ" control:_inputType]];
    _encoding = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, 240, 26) pullsDown:NO];
    [_encoding addItemsWithTitles:[OpenKeyManager getTableCodes]];
    [_encoding setAccessibilityLabel:@"Bảng mã"];
    _encoding.target = self; _encoding.action = @selector(changeEncoding:);
    [typing addArrangedSubview:[self row:@"Bảng mã" control:_encoding]];
    [typing addArrangedSubview:Label(@"Unicode phù hợp với hầu hết ứng dụng. Đổi phím chuyển ngôn ngữ và các tùy chọn gõ trong Cài đặt nâng cao.", NO)];
    _remember = [NSButton checkboxWithTitle:@"Nhớ ngôn ngữ theo ứng dụng" target:self action:@selector(changeRemember:)];
    [typing addArrangedSubview:_remember];
    _globe = [NSButton checkboxWithTitle:@"Chạm riêng phím Fn/🌐 để chuyển ngôn ngữ" target:self action:@selector(changeGlobe:)];
    [typing addArrangedSubview:_globe];
    [typing addArrangedSubview:Label(@"Tùy chọn Fn/🌐 mặc định tắt. Nếu bật, hãy tự đặt “Nhấn phím 🌐 để” thành “Không làm gì” trong cài đặt Bàn phím để tránh chuyển nguồn nhập hai lần. Fn kết hợp phím khác vẫn dùng chức năng của macOS.", NO)];
    [typing addArrangedSubview:[self button:@"Cài đặt nâng cao…" action:@selector(advanced:)]];
    [self addPage:@"Gõ tiếng Việt" to:tabs stack:typing];

    NSStackView *apps = [self stack];
    [apps addArrangedSubview:Label(@"Ngôn ngữ theo ứng dụng", YES)];
    [apps addArrangedSubview:Label(@"Quy tắc Tiếng Việt hoặc English giữ cố định ngôn ngữ và bỏ qua phím chuyển. “Nhớ lựa chọn” ghi nhớ riêng cho ứng dụng, kể cả khi tùy chọn nhớ chung đang tắt. Xóa quy tắc để dùng cài đặt chung.", NO)];
    NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 570, 240)];
    scroll.hasVerticalScroller = YES; scroll.borderType = NSBezelBorder;
    [scroll.widthAnchor constraintEqualToConstant:570].active = YES;
    [scroll.heightAnchor constraintEqualToConstant:240].active = YES;
    _rulesTable = [[NSTableView alloc] initWithFrame:scroll.bounds];
    NSTableColumn *app = [[NSTableColumn alloc] initWithIdentifier:@"app"];
    app.title = @"Ứng dụng"; app.width = 330;
    NSTableColumn *rule = [[NSTableColumn alloc] initWithIdentifier:@"rule"];
    rule.title = @"Ngôn ngữ"; rule.width = 210;
    [_rulesTable addTableColumn:app]; [_rulesTable addTableColumn:rule];
    _rulesTable.rowHeight = 32;
    _rulesTable.dataSource = self; _rulesTable.delegate = self;
    scroll.documentView = _rulesTable;
    [apps addArrangedSubview:scroll];
    [apps addArrangedSubview:[NSStackView stackViewWithViews:@[
        [self button:@"Thêm ứng dụng…" action:@selector(addApp:)],
        [self button:@"Xóa quy tắc đã chọn" action:@selector(removeApp:)]]]];
    [self addPage:@"Ứng dụng" to:tabs stack:apps];
    [self reloadRules];
    return self;
}

- (void)showWindow:(id)sender {
    [super showWindow:sender];
    BOOL onScreen = NO;
    for (NSScreen *screen in NSScreen.screens)
        if (NSIntersectsRect(self.window.frame, screen.visibleFrame)) onScreen = YES;
    if (!onScreen) [self.window center];
    [self refresh];
    if (!_timer) {
        __weak SettingsWindowController *weakSelf = self;
        _timer = [NSTimer timerWithTimeInterval:1 repeats:YES block:^(NSTimer *timer) { [weakSelf refresh]; }];
        [[NSRunLoop mainRunLoop] addTimer:_timer forMode:NSRunLoopCommonModes];
    }
}

- (void)windowWillClose:(NSNotification *)notification { [_timer invalidate]; _timer = nil; }
- (void)dealloc { [_timer invalidate]; }

- (void)refresh {
    BOOL trusted = MJAccessibilityIsEnabled();
    if (trusted && !_wasTrusted && ![OpenKeyManager isInited] && [OpenKeyManager resumeEventTap]) OnActiveAppChanged();
    if (!trusted && [OpenKeyManager isInited]) [OpenKeyManager stopEventTap];
    _wasTrusted = trusted;
    if (!trusted) _status.stringValue = @"Chưa có quyền Trợ năng. Bật OpenKey trong danh sách; cửa sổ này sẽ tự kiểm tra lại.";
    else if (IsSecureEventInputEnabled()) _status.stringValue = @"macOS đang bảo vệ trường nhập. OpenKey tạm bỏ qua xử lý; hãy rời trường mật khẩu hoặc ứng dụng đang bật Nhập bảo mật.";
    else if ([OpenKeyManager isPaused]) _status.stringValue = @"Xử lý bàn phím đang tạm dừng. Chỉ bấm Thử kết nối lại khi bạn muốn tiếp tục.";
    else if ([OpenKeyManager isActive]) _status.stringValue = @"Đã có quyền Trợ năng và kết nối bàn phím đang hoạt động.";
    else _status.stringValue = @"Đã có quyền nhưng chưa kết nối được bàn phím. Thử kết nối lại; nếu vẫn lỗi, thoát và mở lại OpenKey.";
    _retry.enabled = trusted && ![OpenKeyManager isActive];
    if (_inputType.indexOfSelectedItem != vInputType) [_inputType selectItemAtIndex:MAX(0, MIN(3, vInputType))];
    if (_encoding.indexOfSelectedItem != vCodeTable) [_encoding selectItemAtIndex:MAX(0, MIN(4, vCodeTable))];
    _language.selectedSegment = vLanguage ? 0 : 1;
    _remember.state = vUseSmartSwitchKey ? NSControlStateValueOn : NSControlStateValueOff;
    _globe.state = [[NSUserDefaults standardUserDefaults] boolForKey:@"GlobeSwitch"];
    _testHint.stringValue = vInputType == 1 ? @"3. Bật Tiếng Việt rồi gõ a1 → á, vie65t → việt." : @"3. Bật Tiếng Việt rồi gõ as → á, tieengs → tiếng.";
}

- (void)openAccessibility:(id)sender { MJAccessibilityOpenPanel(); }
- (void)openKeyboard:(id)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.Keyboard-Settings.extension"]];
}
- (void)retry:(id)sender { [OpenKeyManager initEventTap]; [self refresh]; }
- (void)advanced:(id)sender { [appDelegate onAdvancedSettings]; }
- (void)changeLanguage:(id)sender {
    [appDelegate setInputLanguage:_language.selectedSegment == 0]; RequestNewSession(); [self refresh];
}
- (void)changeInputType:(id)sender {
    [appDelegate onInputTypeSelectedIndex:(int)_inputType.indexOfSelectedItem]; RequestNewSession(); [self refresh];
}
- (void)changeEncoding:(id)sender { [appDelegate onCodeTableChanged:(int)_encoding.indexOfSelectedItem]; [self refresh]; }
- (void)changeRemember:(id)sender {
    vUseSmartSwitchKey = _remember.state == NSControlStateValueOn;
    [[NSUserDefaults standardUserDefaults] setBool:vUseSmartSwitchKey forKey:@"UseSmartSwitchKey"];
    OnActiveAppChanged();
}
- (void)changeGlobe:(id)sender {
    [[NSUserDefaults standardUserDefaults] setBool:_globe.state == NSControlStateValueOn forKey:@"GlobeSwitch"];
    OnActiveAppChanged(); RequestNewSession();
}

- (NSDictionary *)rules { return [[NSUserDefaults standardUserDefaults] dictionaryForKey:@"AppLanguageRules"] ?: @{}; }
- (void)reloadRules {
    _ruleIDs = [[[self rules] allKeys] sortedArrayUsingSelector:@selector(compare:)];
    [_rulesTable reloadData];
}
- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView { return _ruleIDs.count; }
- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)column row:(NSInteger)row {
    NSString *bundleID = _ruleIDs[row];
    if ([column.identifier isEqualToString:@"app"]) {
        NSURL *url = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:bundleID];
        NSString *name = url ? [[NSFileManager defaultManager] displayNameAtPath:url.path] : bundleID;
        NSTextField *label = [NSTextField labelWithString:name];
        label.toolTip = bundleID;
        return label;
    }
    NSPopUpButton *popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, 200, 26) pullsDown:NO];
    [popup addItemsWithTitles:@[@"English", @"Tiếng Việt", @"Nhớ lựa chọn"]];
    id value = [self rules][bundleID];
    NSInteger index = [value isKindOfClass:NSNumber.class] ? [value integerValue] : 2;
    [popup selectItemAtIndex:MAX(0, MIN(2, index))];
    popup.identifier = bundleID; popup.target = self; popup.action = @selector(changeRule:);
    [popup setAccessibilityLabel:[NSString stringWithFormat:@"Quy tắc %@", bundleID]];
    return popup;
}
- (void)changeRule:(NSPopUpButton *)sender {
    NSMutableDictionary *rules = [[self rules] mutableCopy];
    rules[sender.identifier] = @(sender.indexOfSelectedItem);
    [[NSUserDefaults standardUserDefaults] setObject:rules forKey:@"AppLanguageRules"];
    OnActiveAppChanged();
}
- (void)addApp:(id)sender {
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.allowedFileTypes = @[@"app"]; panel.allowsMultipleSelection = NO;
    panel.canChooseDirectories = NO; panel.directoryURL = [NSURL fileURLWithPath:@"/Applications"];
    [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
        if (result != NSModalResponseOK) return;
        NSString *bundleID = [NSBundle bundleWithURL:panel.URL].bundleIdentifier;
        if (!bundleID.length) {
            [OpenKeyManager showMessage:self.window message:@"Không thể thêm ứng dụng" subMsg:@"Ứng dụng cần có mã định danh bundle hợp lệ."];
            return;
        }
        NSMutableDictionary *rules = [[self rules] mutableCopy];
        if (!rules[bundleID]) rules[bundleID] = @2;
        [[NSUserDefaults standardUserDefaults] setObject:rules forKey:@"AppLanguageRules"];
        [self reloadRules]; OnActiveAppChanged();
    }];
}
- (void)removeApp:(id)sender {
    NSInteger row = _rulesTable.selectedRow;
    if (row < 0 || row >= _ruleIDs.count) return;
    NSMutableDictionary *rules = [[self rules] mutableCopy];
    [rules removeObjectForKey:_ruleIDs[row]];
    [[NSUserDefaults standardUserDefaults] setObject:rules forKey:@"AppLanguageRules"];
    [self reloadRules]; OnActiveAppChanged();
}
@end
