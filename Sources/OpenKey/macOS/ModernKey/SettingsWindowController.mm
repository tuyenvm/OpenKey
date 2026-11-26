#import "SettingsWindowController.h"
#import "AppDelegate.h"
#import "OpenKeyManager.h"
#import "MJAccessibilityUtils.h"
#import "Engine.h"
#import "ConvertTool.h"
#import <Carbon/Carbon.h>

extern AppDelegate *appDelegate;
extern int vSendKeyStepByStep, vFixChromiumBrowser, vPerformLayoutCompat, vShowIconOnDock;
extern "C" void OnActiveAppChanged(void);
extern "C" void RequestNewSession(void);
extern "C" void OnSpellCheckingChanged(void);

@interface OKSettingsDocument : NSView
@end
@implementation OKSettingsDocument
- (BOOL)isFlipped { return YES; }
@end

@interface OKSettingsSidebarRow : NSTableRowView
@end
@implementation OKSettingsSidebarRow
- (void)drawSelectionInRect:(NSRect)dirtyRect {
    [(self.emphasized ? NSColor.selectedContentBackgroundColor : NSColor.unemphasizedSelectedContentBackgroundColor) setFill];
    [[NSBezierPath bezierPathWithRoundedRect:NSInsetRect(self.bounds, 6, 2) xRadius:7 yRadius:7] fill];
}
@end

@implementation SettingsWindowController {
    NSTableView *_sidebar, *_rulesTable, *_macroTable;
    NSSearchField *_search;
    NSTextField *_pageTitle, *_status, *_sidebarStatus, *_testHint, *_macroName, *_macroContent;
    NSScrollView *_pageScroll;
    NSStackView *_content;
    NSButton *_retry;
    NSPopUpButton *_inputType, *_encoding, *_fromCode, *_toCode, *_caseOption;
    NSSegmentedControl *_language;
    MyTextField *_inputKey, *_convertKey;
    NSMutableDictionary<NSString *, NSControl *> *_controls;
    NSArray<NSString *> *_ruleIDs, *_visiblePages;
    NSArray<NSDictionary *> *_pages;
    NSString *_currentPage;
    NSTimer *_timer;
    BOOL _wasTrusted;
    vector<vector<Uint32>> _macroKeys;
    vector<string> _macroNames, _macroContents;
}

static NSTextField *Text(NSString *text, CGFloat size, BOOL secondary) {
    NSTextField *label = [NSTextField wrappingLabelWithString:text];
    label.font = [NSFont systemFontOfSize:size];
    label.textColor = secondary ? NSColor.secondaryLabelColor : NSColor.labelColor;
    return label;
}

- (NSStackView *)stack {
    NSStackView *stack = [NSStackView stackViewWithViews:@[]];
    stack.orientation = NSUserInterfaceLayoutOrientationVertical;
    stack.alignment = NSLayoutAttributeLeading;
    stack.spacing = 16;
    return stack;
}

- (NSButton *)button:(NSString *)title action:(SEL)action {
    return [NSButton buttonWithTitle:title target:self action:action];
}

- (NSView *)row:(NSString *)title detail:(NSString *)detail control:(NSView *)control {
    NSView *row = [NSView new];
    NSStackView *labels = [self stack]; labels.spacing = 4;
    [labels addArrangedSubview:Text(title, 13, NO)];
    if (detail.length) [labels addArrangedSubview:Text(detail, 11, YES)];
    [row addSubview:labels]; [row addSubview:control];
    labels.translatesAutoresizingMaskIntoConstraints = NO;
    control.translatesAutoresizingMaskIntoConstraints = NO;
    [control setContentHuggingPriority:NSLayoutPriorityRequired forOrientation:NSLayoutConstraintOrientationHorizontal];
    [NSLayoutConstraint activateConstraints:@[
        [row.heightAnchor constraintGreaterThanOrEqualToConstant:detail.length ? 66 : 48],
        [labels.leadingAnchor constraintEqualToAnchor:row.leadingAnchor constant:14],
        [labels.centerYAnchor constraintEqualToAnchor:row.centerYAnchor],
        [labels.topAnchor constraintGreaterThanOrEqualToAnchor:row.topAnchor constant:12],
        [labels.bottomAnchor constraintLessThanOrEqualToAnchor:row.bottomAnchor constant:-12],
        [labels.trailingAnchor constraintLessThanOrEqualToAnchor:control.leadingAnchor constant:-18],
        [labels.widthAnchor constraintGreaterThanOrEqualToConstant:180],
        [control.trailingAnchor constraintEqualToAnchor:row.trailingAnchor constant:-14],
        [control.centerYAnchor constraintEqualToAnchor:row.centerYAnchor],
        [control.topAnchor constraintGreaterThanOrEqualToAnchor:row.topAnchor constant:10],
        [control.bottomAnchor constraintLessThanOrEqualToAnchor:row.bottomAnchor constant:-10]
    ]];
    return row;
}

- (void)group:(NSString *)title rows:(NSArray<NSView *> *)rows {
    NSStackView *section = [self stack]; section.spacing = 8;
    if (title.length) {
        NSTextField *heading = Text(title, 12, YES);
        heading.font = [NSFont systemFontOfSize:12 weight:NSFontWeightMedium];
        [section addArrangedSubview:heading];
    }
    NSBox *box = [NSBox new];
    box.boxType = NSBoxCustom; box.borderType = NSNoBorder;
    box.cornerRadius = 10; box.fillColor = [NSColor.labelColor colorWithAlphaComponent:0.045];
    box.contentViewMargins = NSZeroSize;
    NSStackView *body = [self stack]; body.spacing = 0; body.alignment = NSLayoutAttributeCenterX;
    for (NSView *row in rows) {
        if (body.arrangedSubviews.count) {
            NSBox *separator = [NSBox new]; separator.boxType = NSBoxSeparator;
            [body addArrangedSubview:separator];
            [separator.widthAnchor constraintEqualToAnchor:body.widthAnchor constant:-28].active = YES;
        }
        [body addArrangedSubview:row];
        [row.widthAnchor constraintEqualToAnchor:body.widthAnchor].active = YES;
    }
    [box.contentView addSubview:body]; body.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
        [body.leadingAnchor constraintEqualToAnchor:box.contentView.leadingAnchor],
        [body.trailingAnchor constraintEqualToAnchor:box.contentView.trailingAnchor],
        [body.topAnchor constraintEqualToAnchor:box.contentView.topAnchor],
        [body.bottomAnchor constraintEqualToAnchor:box.contentView.bottomAnchor]
    ]];
    [box.heightAnchor constraintEqualToAnchor:body.heightAnchor].active = YES;
    [section addArrangedSubview:box];
    [box.widthAnchor constraintEqualToAnchor:section.widthAnchor].active = YES;
    [_content addArrangedSubview:section];
    [section.widthAnchor constraintEqualToAnchor:_content.widthAnchor].active = YES;
}

- (void)note:(NSString *)text {
    NSTextField *note = Text(text, 11, YES);
    [_content addArrangedSubview:note];
    [note.widthAnchor constraintEqualToAnchor:_content.widthAnchor constant:-4].active = YES;
}

- (NSControl *)toggle:(NSString *)key {
    NSControl *control;
    if (@available(macOS 10.15, *)) control = [NSSwitch new];
    else control = [NSButton checkboxWithTitle:@"" target:nil action:NULL];
    control.identifier = key; control.target = self; control.action = @selector(changeOption:);
    [control setAccessibilityLabel:key];
    _controls[key] = control;
    return control;
}

- (NSView *)option:(NSString *)title key:(NSString *)key detail:(NSString *)detail {
    NSControl *control = [self toggle:key];
    [control setAccessibilityLabel:title];
    return [self row:title detail:detail control:control];
}

- (NSPopUpButton *)popup:(NSArray<NSString *> *)items action:(SEL)action {
    NSPopUpButton *popup = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    [popup addItemsWithTitles:items]; popup.target = self; popup.action = action;
    [popup.widthAnchor constraintEqualToConstant:178].active = YES;
    return popup;
}

- (instancetype)init {
    NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 920, 680)
        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
                  NSWindowStyleMaskResizable | NSWindowStyleMaskFullSizeContentView
        backing:NSBackingStoreBuffered defer:NO];
    self = [super initWithWindow:window]; if (!self) return nil;
    window.title = @"OpenKey"; window.titleVisibility = NSWindowTitleHidden;
    window.titlebarAppearsTransparent = YES; window.minSize = NSMakeSize(860, 600);
    window.delegate = self; window.releasedWhenClosed = NO; [window center];
    _pages = @[
        @{@"id":@"typing", @"title":@"Gõ tiếng Việt", @"icon":@"keyboard", @"keywords":@"kiểu gõ bảng mã chính tả dấu telex vni phụ âm khôi phục hoa gõ nhanh z w j f"},
        @{@"id":@"shortcuts", @"title":@"Phím tắt", @"icon":@"command", @"keywords":@"chuyển ngôn ngữ option control globe fn beep"},
        @{@"id":@"apps", @"title":@"Ứng dụng", @"icon":@"square.stack.3d.up", @"keywords":@"quy tắc nhớ ngôn ngữ bảng mã riêng"},
        @{@"id":@"macros", @"title":@"Gõ tắt", @"icon":@"text.badge.plus", @"keywords":@"macro thay thế viết hoa nhập xuất"},
        @{@"id":@"compatibility", @"title":@"Tương thích", @"icon":@"switch.2", @"keywords":@"trình duyệt chromium excel bàn phím ngôn ngữ khác"},
        @{@"id":@"conversion", @"title":@"Chuyển mã", @"icon":@"arrow.left.arrow.right", @"keywords":@"clipboard sao chép chữ hoa bỏ dấu unicode tcvn"},
        @{@"id":@"system", @"title":@"Hệ thống", @"icon":@"gearshape", @"keywords":@"khởi động dock biểu tượng cập nhật mặc định"},
        @{@"id":@"setup", @"title":@"Quyền truy cập", @"icon":@"hand.raised", @"keywords":@"trợ năng thiết lập thử bàn phím kết nối bảo mật"},
        @{@"id":@"about", @"title":@"Giới thiệu", @"icon":@"info.circle", @"keywords":@"phiên bản mã nguồn liên hệ"}
    ];
    _visiblePages = [_pages valueForKey:@"id"]; _controls = [NSMutableDictionary new];
    NSSplitViewController *split = [NSSplitViewController new];
    NSViewController *side = [NSViewController new]; side.view = [NSView new];
    NSSplitViewItem *sidebarItem = [NSSplitViewItem sidebarWithViewController:side];
    sidebarItem.minimumThickness = 210; sidebarItem.maximumThickness = 245;
    sidebarItem.canCollapse = NO;
    NSViewController *main = [NSViewController new]; main.view = [NSView new];
    [split addSplitViewItem:sidebarItem];
    [split addSplitViewItem:[NSSplitViewItem splitViewItemWithViewController:main]];
    window.contentViewController = split;
    [window setContentSize:NSMakeSize(920, 680)];
    NSTextField *brand = Text(@"OpenKey", 18, NO);
    brand.font = [NSFont systemFontOfSize:18 weight:NSFontWeightSemibold];
    _search = [NSSearchField new]; _search.placeholderString = @"Tìm cài đặt"; _search.delegate = self;
    [_search setAccessibilityLabel:@"Tìm cài đặt"];
    NSScrollView *navigation = [NSScrollView new];
    navigation.drawsBackground = NO; navigation.hasVerticalScroller = YES;
    _sidebar = [NSTableView new]; _sidebar.headerView = nil; _sidebar.backgroundColor = NSColor.clearColor;
    _sidebar.selectionHighlightStyle = NSTableViewSelectionHighlightStyleSourceList;
    if (@available(macOS 11.0, *)) _sidebar.style = NSTableViewStyleSourceList;
    _sidebar.autoresizingMask = NSViewWidthSizable;
    _sidebar.columnAutoresizingStyle = NSTableViewLastColumnOnlyAutoresizingStyle;
    _sidebar.rowHeight = 36; _sidebar.intercellSpacing = NSMakeSize(0, 4);
    _sidebar.dataSource = self; _sidebar.delegate = self;
    [_sidebar setAccessibilityLabel:@"Các mục cài đặt"];
    NSTableColumn *column = [[NSTableColumn alloc] initWithIdentifier:@"navigation"];
    column.width = 180; column.minWidth = 140; [_sidebar addTableColumn:column]; navigation.documentView = _sidebar;
    _sidebarStatus = Text(@"", 11, YES);
    for (NSView *view in @[brand, _search, navigation, _sidebarStatus]) {
        [side.view addSubview:view]; view.translatesAutoresizingMaskIntoConstraints = NO;
    }
    [NSLayoutConstraint activateConstraints:@[
        [brand.topAnchor constraintEqualToAnchor:side.view.topAnchor constant:54],
        [brand.leadingAnchor constraintEqualToAnchor:side.view.leadingAnchor constant:20],
        [_search.topAnchor constraintEqualToAnchor:brand.bottomAnchor constant:18],
        [_search.leadingAnchor constraintEqualToAnchor:side.view.leadingAnchor constant:14],
        [_search.trailingAnchor constraintEqualToAnchor:side.view.trailingAnchor constant:-14],
        [navigation.topAnchor constraintEqualToAnchor:_search.bottomAnchor constant:16],
        [navigation.leadingAnchor constraintEqualToAnchor:side.view.leadingAnchor constant:8],
        [navigation.trailingAnchor constraintEqualToAnchor:side.view.trailingAnchor constant:-8],
        [navigation.bottomAnchor constraintEqualToAnchor:_sidebarStatus.topAnchor constant:-16],
        [_sidebarStatus.leadingAnchor constraintEqualToAnchor:side.view.leadingAnchor constant:20],
        [_sidebarStatus.bottomAnchor constraintEqualToAnchor:side.view.bottomAnchor constant:-20]
    ]];
    _pageTitle = Text(@"", 20, NO); _pageTitle.font = [NSFont systemFontOfSize:20 weight:NSFontWeightSemibold];
    _pageScroll = [NSScrollView new]; _pageScroll.hasVerticalScroller = YES; _pageScroll.drawsBackground = NO;
    for (NSView *view in @[_pageTitle, _pageScroll]) { [main.view addSubview:view]; view.translatesAutoresizingMaskIntoConstraints = NO; }
    [NSLayoutConstraint activateConstraints:@[
        [_pageTitle.leadingAnchor constraintEqualToAnchor:main.view.leadingAnchor constant:28],
        [_pageTitle.topAnchor constraintEqualToAnchor:main.view.topAnchor constant:51],
        [_pageScroll.topAnchor constraintEqualToAnchor:_pageTitle.bottomAnchor constant:18],
        [_pageScroll.leadingAnchor constraintEqualToAnchor:main.view.leadingAnchor],
        [_pageScroll.trailingAnchor constraintEqualToAnchor:main.view.trailingAnchor],
        [_pageScroll.bottomAnchor constraintEqualToAnchor:main.view.bottomAnchor]
    ]];
    [self showPage:MJAccessibilityIsEnabled() ? @"typing" : @"setup"];
    return self;
}

- (void)showPage:(NSString *)page {
    if (![_visiblePages containsObject:page]) {
        _search.stringValue = @""; _visiblePages = [_pages valueForKey:@"id"]; [_sidebar reloadData];
    }
    NSInteger index = [_visiblePages indexOfObject:page];
    if (index == NSNotFound) return;
    [_sidebar selectRowIndexes:[NSIndexSet indexSetWithIndex:index] byExtendingSelection:NO];
    if (![_currentPage isEqual:page]) [self renderPage:page];
}

- (void)renderPage:(NSString *)page {
    _currentPage = page;
    for (NSDictionary *item in _pages) if ([item[@"id"] isEqual:page]) _pageTitle.stringValue = item[@"title"];
    [_controls removeAllObjects];
    _inputType = _encoding = _fromCode = _toCode = _caseOption = nil;
    _language = nil; _inputKey = _convertKey = nil;
    _status = _testHint = nil; _retry = nil;
    _rulesTable = _macroTable = nil; _macroName = _macroContent = nil;
    OKSettingsDocument *document = [OKSettingsDocument new];
    _pageScroll.documentView = document; document.translatesAutoresizingMaskIntoConstraints = NO;
    _content = [self stack]; _content.spacing = 22;
    [document addSubview:_content]; _content.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
        [document.widthAnchor constraintEqualToAnchor:_pageScroll.contentView.widthAnchor],
        [_content.leadingAnchor constraintEqualToAnchor:document.leadingAnchor constant:28],
        [_content.trailingAnchor constraintEqualToAnchor:document.trailingAnchor constant:-28],
        [_content.topAnchor constraintEqualToAnchor:document.topAnchor constant:4],
        [_content.bottomAnchor constraintEqualToAnchor:document.bottomAnchor constant:-28]
    ]];
    if ([page isEqual:@"typing"]) [self typingPage];
    else if ([page isEqual:@"shortcuts"]) [self shortcutsPage];
    else if ([page isEqual:@"apps"]) [self appsPage];
    else if ([page isEqual:@"macros"]) [self macrosPage];
    else if ([page isEqual:@"compatibility"]) [self compatibilityPage];
    else if ([page isEqual:@"conversion"]) [self conversionPage];
    else if ([page isEqual:@"system"]) [self systemPage];
    else if ([page isEqual:@"setup"]) [self setupPage];
    else if ([page isEqual:@"empty"]) { _pageTitle.stringValue = @"Không tìm thấy cài đặt"; [self note:@"Thử tìm theo chức năng, chẳng hạn phím tắt, chính tả hoặc clipboard."]; }
    else [self aboutPage];
    [self refresh];
    [_pageScroll.contentView scrollToPoint:NSZeroPoint];
}

- (void)typingPage {
    _language = [NSSegmentedControl segmentedControlWithLabels:@[@"Tiếng Việt", @"English"]
        trackingMode:NSSegmentSwitchTrackingSelectOne target:self action:@selector(changeLanguage:)];
    _inputType = [self popup:@[@"Telex", @"VNI", @"Simple Telex 1", @"Simple Telex 2"] action:@selector(changeInputType:)];
    _encoding = [self popup:[OpenKeyManager getTableCodes] action:@selector(changeEncoding:)];
    [_inputType setAccessibilityLabel:@"Kiểu gõ"]; [_encoding setAccessibilityLabel:@"Bảng mã"];
    [self group:@"" rows:@[
        [self row:@"Ngôn ngữ" detail:nil control:_language],
        [self row:@"Kiểu gõ" detail:nil control:_inputType],
        [self row:@"Bảng mã" detail:@"Unicode dùng cho hầu hết ứng dụng." control:_encoding]
    ]];
    [self group:@"Chính tả & dấu" rows:@[
        [self option:@"Kiểm tra chính tả" key:@"Spelling" detail:nil],
        [self option:@"Khôi phục phím khi từ không hợp lệ" key:@"RestoreIfInvalidWord" detail:nil],
        [self option:@"Cho phép z, w, j, f làm phụ âm" key:@"vAllowConsonantZFWJ" detail:nil],
        [self option:@"Đặt dấu oà, uý" key:@"ModernOrthography" detail:@"Tắt để dùng òa, úy."],
        [self option:@"Viết hoa đầu câu" key:@"UpperCaseFirstChar" detail:nil]
    ]];
    [self group:@"Gõ nhanh" rows:@[
        [self option:@"Telex nhanh" key:@"QuickTelex" detail:@"cc → ch, gg → gi, kk → kh, nn → ng…"],
        [self option:@"Phụ âm đầu nhanh" key:@"vQuickStartConsonant" detail:@"f → ph, j → gi, w → qu"],
        [self option:@"Phụ âm cuối nhanh" key:@"vQuickEndConsonant" detail:@"g → ng, h → nh, k → ch"]
    ]];
}

- (NSView *)shortcut:(BOOL)conversion {
    NSMutableArray *views = [NSMutableArray new];
    for (NSInteger i=0;i<4;i++) {
        NSButton *modifier = [NSButton checkboxWithTitle:@[@"⌃", @"⌥", @"⌘", @"⇧"][i] target:self action:@selector(changeModifier:)];
        modifier.tag = 1 << (8+i); modifier.identifier = conversion ? @"convert" : @"input";
        [modifier setAccessibilityLabel:@[@"Control", @"Option", @"Command", @"Shift"][i]];
        _controls[[NSString stringWithFormat:@"%@.%ld", modifier.identifier, (long)modifier.tag]] = modifier;
        [views addObject:modifier];
    }
    MyTextField *key = [MyTextField new]; key.Parent = self;
    [key.widthAnchor constraintEqualToConstant:58].active = YES;
    [key setAccessibilityLabel:conversion ? @"Phím chuyển mã" : @"Phím chuyển ngôn ngữ"];
    if (conversion) _convertKey = key; else _inputKey = key;
    [views addObject:key];
    NSStackView *stack = [NSStackView stackViewWithViews:views]; stack.spacing = 8;
    return stack;
}

- (void)shortcutsPage {
    [self group:@"Chuyển ngôn ngữ" rows:@[
        [self row:@"Phím chuyển" detail:nil control:[self shortcut:NO]],
        [self option:@"Âm báo khi chuyển" key:@"SwitchBeep" detail:nil],
        [self option:@"Chạm Fn / Globe để chuyển" key:@"GlobeSwitch" detail:nil]
    ]];
    [self note:@"Nếu dùng Fn / Globe, đặt “Nhấn phím 🌐 để” thành “Không làm gì” trong cài đặt Bàn phím. Các tổ hợp Fn khác vẫn hoạt động bình thường."];
    [self group:@"Tạm bỏ qua khi gõ" rows:@[
        [self option:@"Chạm Control để tắt kiểm tra chính tả" key:@"vTempOffSpelling" detail:@"Chỉ áp dụng cho từ đang gõ."],
        [self option:@"Chạm Command để tạm tắt OpenKey" key:@"vTempOffOpenKey" detail:@"Tự bật lại ở từ tiếp theo."]
    ]];
    [_content addArrangedSubview:[self button:@"Cài đặt Bàn phím…" action:@selector(openKeyboard:)]];
}

- (void)appsPage {
    [self group:@"" rows:@[
        [self option:@"Nhớ ngôn ngữ theo ứng dụng" key:@"UseSmartSwitchKey" detail:nil],
        [self option:@"Nhớ bảng mã theo ứng dụng" key:@"vRememberCode" detail:nil]
    ]];
    [self note:@"Quy tắc cố định bỏ qua phím chuyển ngôn ngữ. Xóa quy tắc để dùng cài đặt chung."];
    _rulesTable = [self tableWithColumns:@[@"Ứng dụng", @"Ngôn ngữ"] identifiers:@[@"app", @"rule"] height:240];
    [self reloadRules];
    [_content addArrangedSubview:[NSStackView stackViewWithViews:@[
        [self button:@"Thêm ứng dụng…" action:@selector(addApp:)],
        [self button:@"Xóa quy tắc" action:@selector(removeApp:)]]]];
}

- (void)compatibilityPage {
    [self group:@"Ứng dụng & bàn phím" rows:@[
        [self option:@"Sửa lỗi gợi ý từ" key:@"FixRecommendBrowser" detail:@"Dành cho trình duyệt, Excel và ô tự hoàn thành."],
        [self option:@"Tương thích Chromium" key:@"vFixChromiumBrowser" detail:@"Chỉ bật khi trình duyệt cần thêm xử lý."],
        [self option:@"Gửi từng phím riêng" key:@"SendKeyStepByStep" detail:@"Dùng khi ứng dụng không nhận đúng chuỗi ký tự."],
        [self option:@"Theo bố cục bàn phím hiện tại" key:@"vPerformLayoutCompat" detail:nil],
        [self option:@"Bỏ qua khi macOS dùng bộ gõ khác" key:@"vOtherLanguage" detail:@"Tránh hai bộ gõ cùng xử lý tiếng Việt."]
    ]];
}

- (void)setupPage {
    _status = Text(@"", 13, NO);
    _retry = [self button:@"Kết nối lại" action:@selector(retry:)];
    [self group:@"Trạng thái" rows:@[[self row:@"Kết nối bàn phím" detail:nil control:_retry]]];
    [_content addArrangedSubview:_status];
    [_status.widthAnchor constraintEqualToAnchor:_content.widthAnchor].active = YES;
    [self group:@"Quyền & nguồn nhập" rows:@[
        [self row:@"Trợ năng" detail:@"Cho phép OpenKey nhận và thay thế phím gõ." control:[self button:@"Mở cài đặt…" action:@selector(openAccessibility:)]],
        [self row:@"Nguồn nhập macOS" detail:@"Dùng ABC để tránh hai bộ gõ cùng xử lý." control:[self button:@"Bàn phím…" action:@selector(openKeyboard:)]]
    ]];
    _testHint = Text(@"", 12, YES); [_content addArrangedSubview:_testHint];
    NSTextField *test = [NSTextField textFieldWithString:@""]; test.placeholderString = @"Gõ thử tiếng Việt tại đây";
    [test setAccessibilityLabel:@"Gõ thử tiếng Việt"];
    [_content addArrangedSubview:test]; [test.widthAnchor constraintEqualToAnchor:_content.widthAnchor].active = YES;
    [self note:@"Trạng thái kết nối không thay thế việc kiểm tra gõ thực tế."];
}

- (void)systemPage {
    [self group:@"Khởi động & hiển thị" rows:@[
        [self option:@"Mở cùng macOS" key:@"RunOnStartup" detail:nil],
        [self option:@"Hiện cài đặt khi mở OpenKey" key:@"ShowUIOnStartup" detail:nil],
        [self option:@"Hiện biểu tượng trên Dock" key:@"vShowIconOnDock" detail:nil],
        [self option:@"Biểu tượng theo màu hệ thống" key:@"GrayIcon" detail:nil]
    ]];
    [self group:@"Cập nhật" rows:@[
        [self option:@"Kiểm tra khi khởi động" key:@"CheckUpdates" detail:nil],
        [self row:@"Phiên bản mới" detail:nil control:[self button:@"Kiểm tra ngay…" action:@selector(checkUpdate:)]]
    ]];
    [_content addArrangedSubview:[self button:@"Khôi phục cài đặt mặc định…" action:@selector(resetDefaults:)]];
}

- (void)aboutPage {
    NSString *version = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"] ?: @"Bản thử nghiệm";
    [self group:@"" rows:@[[self row:@"OpenKey" detail:@"Bộ gõ tiếng Việt mã nguồn mở" control:Text(version, 12, YES)]]];
    [self note:@"Mai Vũ Tuyên và cộng đồng đóng góp. Phát hành theo GPL-3.0."];
    for (NSArray *link in @[
        @[@"Trang chủ", @"https://open-key.org"], @[@"Mã nguồn", @"https://github.com/tuyenvm/OpenKey"],
        @[@"Liên hệ", @"mailto:maivutuyen.91@gmail.com"]]) {
        NSButton *button = [self button:link[0] action:@selector(openLink:)]; button.identifier = link[1]; [_content addArrangedSubview:button];
    }
}

- (NSTableView *)tableWithColumns:(NSArray *)titles identifiers:(NSArray *)identifiers height:(CGFloat)height {
    NSScrollView *scroll = [NSScrollView new]; scroll.hasVerticalScroller = YES;
    scroll.borderType = NSNoBorder; scroll.drawsBackground = NO;
    [scroll.heightAnchor constraintEqualToConstant:height].active = YES;
    NSTableView *table = [NSTableView new]; table.rowHeight = 34;
    table.usesAlternatingRowBackgroundColors = NO;
    if (@available(macOS 11.0, *)) table.style = NSTableViewStyleInset;
    table.dataSource = self; table.delegate = self;
    for (NSUInteger i=0;i<titles.count;i++) {
        NSTableColumn *column = [[NSTableColumn alloc] initWithIdentifier:identifiers[i]];
        column.title = titles[i]; column.width = i ? 200 : 310;
        [table addTableColumn:column];
    }
    scroll.documentView = table;
    [_content addArrangedSubview:scroll]; [scroll.widthAnchor constraintEqualToAnchor:_content.widthAnchor].active = YES;
    return table;
}

- (void)macrosPage {
    [self group:@"" rows:@[
        [self option:@"Bật gõ tắt" key:@"UseMacro" detail:nil],
        [self option:@"Gõ tắt trong chế độ English" key:@"UseMacroInEnglishMode" detail:nil],
        [self option:@"Tự điều chỉnh chữ hoa" key:@"vAutoCapsMacro" detail:nil]
    ]];
    _macroName = [NSTextField textFieldWithString:@""]; _macroName.placeholderString = @"Ví dụ: dc";
    _macroContent = [NSTextField textFieldWithString:@""]; _macroContent.placeholderString = @"Nội dung thay thế";
    [_macroName setAccessibilityLabel:@"Viết tắt"]; [_macroContent setAccessibilityLabel:@"Nội dung thay thế"];
    [_macroName.widthAnchor constraintEqualToConstant:178].active = YES;
    [_macroContent.widthAnchor constraintEqualToConstant:260].active = YES;
    [self group:@"Thêm hoặc sửa" rows:@[
        [self row:@"Viết tắt" detail:nil control:_macroName],
        [self row:@"Nội dung" detail:nil control:_macroContent]
    ]];
    [_content addArrangedSubview:[NSStackView stackViewWithViews:@[
        [self button:@"Lưu" action:@selector(saveMacro:)], [self button:@"Xóa mục đã chọn" action:@selector(deleteMacro:)],
        [self button:@"Nhập…" action:@selector(importMacros:)], [self button:@"Xuất…" action:@selector(exportMacros:)]]]];
    _macroTable = [self tableWithColumns:@[@"Viết tắt", @"Nội dung"] identifiers:@[@"macro", @"content"] height:160];
    [self reloadMacros];
}

- (void)conversionPage {
    _fromCode = [self popup:[OpenKeyManager getTableCodes] action:@selector(changeConversion:)];
    _toCode = [self popup:[OpenKeyManager getTableCodes] action:@selector(changeConversion:)];
    _caseOption = [self popup:@[@"Giữ nguyên", @"CHỮ HOA", @"chữ thường", @"Hoa đầu câu", @"Hoa Mỗi Từ"] action:@selector(changeConversion:)];
    [_fromCode setAccessibilityLabel:@"Từ bảng mã"]; [_toCode setAccessibilityLabel:@"Sang bảng mã"];
    [_caseOption setAccessibilityLabel:@"Chữ hoa / thường"];
    [self group:@"Chuyển văn bản trong clipboard" rows:@[
        [self row:@"Từ bảng mã" detail:nil control:_fromCode],
        [self row:@"Sang bảng mã" detail:nil control:_toCode],
        [self row:@"Chữ hoa / thường" detail:nil control:_caseOption],
        [self option:@"Bỏ dấu tiếng Việt" key:@"ConvertRemoveMark" detail:nil],
        [self option:@"Thông báo khi hoàn tất" key:@"ConvertAlert" detail:nil]
    ]];
    [_content addArrangedSubview:[NSStackView stackViewWithViews:@[
        [self button:@"Chuyển mã clipboard" action:@selector(convertClipboard:)],
        [self button:@"Đổi chiều" action:@selector(reverseConversion:)]]]];
    [self group:@"Chuyển mã nhanh" rows:@[[self row:@"Phím tắt" detail:nil control:[self shortcut:YES]]]];
    [self note:@"Sao chép văn bản trước khi chuyển. Kết quả sẽ thay thế nội dung clipboard."];
}

- (NSDictionary<NSString *, NSValue *> *)optionBindings {
    return @{
        @"Spelling":[NSValue valueWithPointer:&vCheckSpelling],
        @"ModernOrthography":[NSValue valueWithPointer:&vUseModernOrthography],
        @"QuickTelex":[NSValue valueWithPointer:&vQuickTelex],
        @"RestoreIfInvalidWord":[NSValue valueWithPointer:&vRestoreIfWrongSpelling],
        @"FixRecommendBrowser":[NSValue valueWithPointer:&vFixRecommendBrowser],
        @"UseMacro":[NSValue valueWithPointer:&vUseMacro],
        @"UseMacroInEnglishMode":[NSValue valueWithPointer:&vUseMacroInEnglishMode],
        @"vAutoCapsMacro":[NSValue valueWithPointer:&vAutoCapsMacro],
        @"SendKeyStepByStep":[NSValue valueWithPointer:&vSendKeyStepByStep],
        @"UseSmartSwitchKey":[NSValue valueWithPointer:&vUseSmartSwitchKey],
        @"UpperCaseFirstChar":[NSValue valueWithPointer:&vUpperCaseFirstChar],
        @"vTempOffSpelling":[NSValue valueWithPointer:&vTempOffSpelling],
        @"vAllowConsonantZFWJ":[NSValue valueWithPointer:&vAllowConsonantZFWJ],
        @"vQuickStartConsonant":[NSValue valueWithPointer:&vQuickStartConsonant],
        @"vQuickEndConsonant":[NSValue valueWithPointer:&vQuickEndConsonant],
        @"vRememberCode":[NSValue valueWithPointer:&vRememberCode],
        @"vOtherLanguage":[NSValue valueWithPointer:&vOtherLanguage],
        @"vTempOffOpenKey":[NSValue valueWithPointer:&vTempOffOpenKey],
        @"vShowIconOnDock":[NSValue valueWithPointer:&vShowIconOnDock],
        @"vFixChromiumBrowser":[NSValue valueWithPointer:&vFixChromiumBrowser],
        @"vPerformLayoutCompat":[NSValue valueWithPointer:&vPerformLayoutCompat]
    };
}

- (BOOL)optionValue:(NSString *)key {
    NSValue *binding = [self optionBindings][key];
    if (binding) return *(int *)binding.pointerValue != 0;
    if ([key isEqual:@"SwitchBeep"]) return (vSwitchKeyStatus & 0x8000) != 0;
    if ([key isEqual:@"ConvertRemoveMark"]) return convertToolRemoveMark;
    if ([key isEqual:@"ConvertAlert"]) return !convertToolDontAlertWhenCompleted;
    BOOL value = [[NSUserDefaults standardUserDefaults] boolForKey:[key isEqual:@"CheckUpdates"] ? @"DontCheckUpdate" : key];
    return [key isEqual:@"CheckUpdates"] ? !value : value;
}

- (void)changeOption:(id)sender {
    NSString *key = [sender identifier]; BOOL value = [sender state] == NSControlStateValueOn;
    NSValue *binding = [self optionBindings][key];
    if (binding) *(int *)binding.pointerValue = value;
    NSString *preference = key; BOOL storedValue = value;
    if ([key isEqual:@"SwitchBeep"]) {
        vSwitchKeyStatus = (vSwitchKeyStatus & ~0x8000) | (value ? 0x8000 : 0);
        [[NSUserDefaults standardUserDefaults] setInteger:vSwitchKeyStatus forKey:@"SwitchKeyStatus"];
    } else {
        if ([key isEqual:@"CheckUpdates"]) { preference = @"DontCheckUpdate"; storedValue = !value; }
        if ([key isEqual:@"ConvertRemoveMark"]) { convertToolRemoveMark = value; preference = @"convertToolRemoveMark"; }
        if ([key isEqual:@"ConvertAlert"]) { convertToolDontAlertWhenCompleted = !value; preference = @"convertToolDontAlertWhenCompleted"; }
        [[NSUserDefaults standardUserDefaults] setBool:storedValue forKey:preference];
    }
    if ([key isEqual:@"Spelling"]) OnSpellCheckingChanged();
    if ([key isEqual:@"RunOnStartup"]) [appDelegate setRunOnStartup:value];
    if ([key isEqual:@"GrayIcon"]) [appDelegate setGrayIcon:value];
    if ([key isEqual:@"vShowIconOnDock"]) [appDelegate showIconOnDock:value];
    if ([key isEqual:@"UseSmartSwitchKey"] || [key isEqual:@"vRememberCode"] || [key isEqual:@"GlobeSwitch"]) OnActiveAppChanged();
    RequestNewSession(); [self refresh];
}

- (void)showWindow:(id)sender {
    [super showWindow:sender];
    BOOL visible = NO;
    for (NSScreen *screen in NSScreen.screens) if (NSIntersectsRect(self.window.frame, screen.visibleFrame)) visible = YES;
    if (!visible) [self.window center];
    [self.window.contentView layoutSubtreeIfNeeded];
    NSRect sidebarFrame = _sidebar.frame;
    sidebarFrame.size.width = _sidebar.enclosingScrollView.contentSize.width;
    _sidebar.frame = sidebarFrame; [_sidebar sizeLastColumnToFit];
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
    NSString *status, *shortStatus;
    if (!trusted) { shortStatus = @"Cần quyền Trợ năng"; status = @"Bật OpenKey trong cài đặt Trợ năng. Kết nối sẽ được kiểm tra lại tự động."; }
    else if (IsSecureEventInputEnabled()) { shortStatus = @"Nhập bảo mật"; status = @"macOS đang bảo vệ trường nhập. OpenKey sẽ tiếp tục khi bạn rời trường mật khẩu hoặc tắt Nhập bảo mật."; }
    else if ([OpenKeyManager isPaused]) { shortStatus = @"Đã tạm dừng"; status = @"Bấm Kết nối lại khi bạn muốn tiếp tục xử lý bàn phím."; }
    else if ([OpenKeyManager isActive]) { shortStatus = @"Đang hoạt động"; status = @"Đã có quyền Trợ năng. Kết nối bàn phím đang hoạt động."; }
    else { shortStatus = @"Chưa kết nối"; status = @"Đã có quyền nhưng chưa kết nối được bàn phím. Hãy thử kết nối lại."; }
    if (_status && ![_status.stringValue isEqual:status]) _status.stringValue = status;
    if (![_sidebarStatus.stringValue isEqual:shortStatus]) _sidebarStatus.stringValue = shortStatus;
    _retry.enabled = trusted && ![OpenKeyManager isActive];
    if (_inputType && _inputType.indexOfSelectedItem != vInputType) [_inputType selectItemAtIndex:MAX(0, MIN(3, vInputType))];
    if (_encoding && _encoding.indexOfSelectedItem != vCodeTable) [_encoding selectItemAtIndex:MAX(0, MIN(4, vCodeTable))];
    _language.selectedSegment = vLanguage ? 0 : 1;
    for (NSString *key in _controls) {
        id control = _controls[key]; BOOL value;
        if ([key hasPrefix:@"input."] || [key hasPrefix:@"convert."]) {
            int shortcut = [key hasPrefix:@"input."] ? vSwitchKeyStatus : convertToolHotKey;
            value = (shortcut & [key.pathExtension integerValue]) != 0;
        } else value = [self optionValue:key];
        if ([control state] != value) [control setState:value];
    }
    for (NSString *key in @[@"RestoreIfInvalidWord", @"vAllowConsonantZFWJ", @"vTempOffSpelling"])
        _controls[key].enabled = vCheckSpelling;
    _controls[@"vFixChromiumBrowser"].enabled = vFixRecommendBrowser;
    if (_inputKey && !_inputKey.currentEditor) [_inputKey setTextByChar:(vSwitchKeyStatus >> 24) & 0xFF];
    if (_convertKey && !_convertKey.currentEditor) [_convertKey setTextByChar:(convertToolHotKey >> 24) & 0xFF];
    NSString *hint = vInputType == 1 ? @"Thử VNI: a1 → á · vie65t → việt" : @"Thử Telex: as → á · tieengs → tiếng";
    if (_testHint && ![_testHint.stringValue isEqual:hint]) _testHint.stringValue = hint;
    if (_fromCode) [_fromCode selectItemAtIndex:MIN(4, convertToolFromCode)];
    if (_toCode) [_toCode selectItemAtIndex:MIN(4, convertToolToCode)];
    if (_caseOption) [_caseOption selectItemAtIndex:convertToolToAllCaps ? 1 : convertToolToAllNonCaps ? 2 : convertToolToCapsFirstLetter ? 3 : convertToolToCapsEachWord ? 4 : 0];
}

- (void)openAccessibility:(id)sender { MJAccessibilityOpenPanel(); }
- (void)openKeyboard:(id)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.keyboard"]];
}
- (void)retry:(id)sender { [OpenKeyManager initEventTap]; [self refresh]; }
- (void)changeLanguage:(id)sender {
    [appDelegate setInputLanguage:_language.selectedSegment == 0]; RequestNewSession(); [self refresh];
}
- (void)changeInputType:(id)sender {
    [appDelegate onInputTypeSelectedIndex:(int)_inputType.indexOfSelectedItem]; RequestNewSession(); [self refresh];
}
- (void)changeEncoding:(id)sender { [appDelegate onCodeTableChanged:(int)_encoding.indexOfSelectedItem]; [self refresh]; }
- (void)changeModifier:(NSButton *)sender {
    int *shortcut = [sender.identifier isEqual:@"input"] ? &vSwitchKeyStatus : &convertToolHotKey;
    *shortcut = (*shortcut & ~(int)sender.tag) | (sender.state == NSControlStateValueOn ? (int)sender.tag : 0);
    [[NSUserDefaults standardUserDefaults] setInteger:*shortcut forKey:shortcut == &vSwitchKeyStatus ? @"SwitchKeyStatus" : @"convertToolHotKey"];
    [appDelegate setQuickConvertString]; [self refresh];
}
- (void)onMyTextFieldKeyChange:(unsigned short)keyCode character:(unsigned short)character {
    int *shortcut = _convertKey.currentEditor ? &convertToolHotKey : &vSwitchKeyStatus;
    *shortcut = (*shortcut & 0x00FFFF00) | keyCode | ((unsigned int)character << 24);
    [[NSUserDefaults standardUserDefaults] setInteger:*shortcut forKey:shortcut == &vSwitchKeyStatus ? @"SwitchKeyStatus" : @"convertToolHotKey"];
    [appDelegate setQuickConvertString];
}
- (void)checkUpdate:(NSButton *)sender {
    sender.enabled = NO; sender.title = @"Đang kiểm tra…";
    __weak NSButton *button = sender;
    [OpenKeyManager checkNewVersion:self.window callbackFunc:^{ button.enabled = YES; button.title = @"Kiểm tra ngay…"; }];
}
- (void)openLink:(NSButton *)sender { [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:sender.identifier]]; }
- (void)resetDefaults:(id)sender {
    NSAlert *alert = [NSAlert new]; alert.messageText = @"Khôi phục cài đặt mặc định?";
    alert.informativeText = @"Giữ nguyên dữ liệu gõ tắt và quy tắc ứng dụng.";
    [alert addButtonWithTitle:@"Hủy"]; [alert addButtonWithTitle:@"Khôi phục"];
    [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse response) {
        if (response == NSAlertSecondButtonReturn) {
            [appDelegate loadDefaultConfig];
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"GlobeSwitch"];
            RequestNewSession(); [self refresh];
        }
    }];
}

- (void)controlTextDidChange:(NSNotification *)notification {
    if (notification.object != _search) return;
    NSString *query = _search.stringValue;
    NSMutableArray *matches = [NSMutableArray new];
    for (NSDictionary *page in _pages) {
        NSString *text = [page[@"title"] stringByAppendingFormat:@" %@", page[@"keywords"]];
        if (!query.length || [text rangeOfString:query options:NSCaseInsensitiveSearch | NSDiacriticInsensitiveSearch].location != NSNotFound)
            [matches addObject:page[@"id"]];
    }
    _visiblePages = matches; [_sidebar reloadData];
    if (matches.count) [self showPage:matches.firstObject];
    else [self renderPage:@"empty"];
}
- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    if (tableView == _sidebar) return _visiblePages.count;
    if (tableView == _macroTable) return _macroNames.size();
    return _ruleIDs.count;
}
- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)column row:(NSInteger)row {
    if (tableView == _sidebar) {
        NSDictionary *page;
        for (NSDictionary *item in _pages) if ([item[@"id"] isEqual:_visiblePages[row]]) page = item;
        NSTableCellView *cell = [NSTableCellView new];
        NSTextField *label = [NSTextField labelWithString:page[@"title"]];
        NSImageView *image = [NSImageView new];
        if (@available(macOS 11.0, *)) image.image = [NSImage imageWithSystemSymbolName:page[@"icon"] accessibilityDescription:nil];
        cell.textField = label; cell.imageView = image;
        [cell addSubview:label]; [cell addSubview:image];
        label.translatesAutoresizingMaskIntoConstraints = image.translatesAutoresizingMaskIntoConstraints = NO;
        [NSLayoutConstraint activateConstraints:@[
            [image.leadingAnchor constraintEqualToAnchor:cell.leadingAnchor constant:8],
            [image.centerYAnchor constraintEqualToAnchor:cell.centerYAnchor],
            [image.widthAnchor constraintEqualToConstant:18], [image.heightAnchor constraintEqualToConstant:18],
            [label.leadingAnchor constraintEqualToAnchor:image.trailingAnchor constant:10],
            [label.centerYAnchor constraintEqualToAnchor:cell.centerYAnchor]
        ]];
        return cell;
    }
    if (tableView == _macroTable) {
        const string &value = [column.identifier isEqual:@"macro"] ? _macroNames[row] : _macroContents[row];
        return [NSTextField labelWithString:[NSString stringWithUTF8String:value.c_str()]];
    }
    NSString *bundleID = _ruleIDs[row];
    if ([column.identifier isEqual:@"app"]) {
        NSURL *url = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:bundleID];
        NSTextField *label = [NSTextField labelWithString:url ? [[NSFileManager defaultManager] displayNameAtPath:url.path] : bundleID];
        label.toolTip = bundleID; return label;
    }
    NSPopUpButton *popup = [self popup:@[@"English", @"Tiếng Việt", @"Nhớ lựa chọn"] action:@selector(changeRule:)];
    id value = [self rules][bundleID];
    NSInteger index = [value isKindOfClass:NSNumber.class] ? [value integerValue] : 2;
    [popup selectItemAtIndex:MAX(0, MIN(2, index))]; popup.identifier = bundleID;
    [popup setAccessibilityLabel:[NSString stringWithFormat:@"Quy tắc %@", bundleID]];
    return popup;
}
- (NSTableRowView *)tableView:(NSTableView *)tableView rowViewForRow:(NSInteger)row {
    return tableView == _sidebar ? [OKSettingsSidebarRow new] : nil;
}
- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    NSTableView *table = notification.object;
    if (table == _sidebar && table.selectedRow >= 0) [self renderPage:_visiblePages[table.selectedRow]];
    if (table == _macroTable && table.selectedRow >= 0 && table.selectedRow < (NSInteger)_macroNames.size()) {
        _macroName.stringValue = [NSString stringWithUTF8String:_macroNames[table.selectedRow].c_str()];
        _macroContent.stringValue = [NSString stringWithUTF8String:_macroContents[table.selectedRow].c_str()];
    }
}

- (NSDictionary *)rules { return [[NSUserDefaults standardUserDefaults] dictionaryForKey:@"AppLanguageRules"] ?: @{}; }
- (void)reloadRules {
    _ruleIDs = [[[self rules] allKeys] sortedArrayUsingSelector:@selector(compare:)];
    [_rulesTable reloadData];
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

- (void)reloadMacros {
    getAllMacro(_macroKeys, _macroNames, _macroContents);
    [_macroTable reloadData];
}
- (void)persistMacros {
    vector<Byte> bytes; getMacroSaveData(bytes);
    [[NSUserDefaults standardUserDefaults] setObject:[NSData dataWithBytes:bytes.data() length:bytes.size()] forKey:@"macroData"];
    [self reloadMacros]; RequestNewSession();
}
- (void)saveMacro:(id)sender {
    if (!_macroName.stringValue.length || !_macroContent.stringValue.length ||
        !addMacro(_macroName.stringValue.UTF8String, _macroContent.stringValue.UTF8String)) {
        [OpenKeyManager showMessage:self.window message:@"Không thể lưu gõ tắt" subMsg:@"Nhập tên và nội dung hợp lệ; tên tối đa 255 byte UTF-8."];
        return;
    }
    [self persistMacros]; _macroName.stringValue = @""; _macroContent.stringValue = @"";
}
- (void)deleteMacro:(id)sender {
    NSInteger row = _macroTable.selectedRow;
    if (row < 0 || row >= (NSInteger)_macroNames.size()) return;
    if (deleteMacro(_macroNames[row])) {
        [self persistMacros]; _macroName.stringValue = @""; _macroContent.stringValue = @"";
    }
}
- (void)importMacros:(id)sender {
    NSOpenPanel *panel = [NSOpenPanel openPanel]; panel.allowedFileTypes = @[@"txt"];
    panel.canChooseDirectories = NO; panel.allowsMultipleSelection = NO;
    [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
        if (result != NSModalResponseOK) return;
        NSAlert *choice = [NSAlert new]; choice.messageText = @"Nhập dữ liệu gõ tắt";
        choice.informativeText = @"Giữ dữ liệu hiện tại hoặc thay bằng nội dung trong tệp.";
        [choice addButtonWithTitle:@"Giữ và thêm"]; [choice addButtonWithTitle:@"Thay thế"]; [choice addButtonWithTitle:@"Hủy"];
        [choice beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse response) {
            if (response == NSAlertThirdButtonReturn) return;
            readFromFile(panel.URL.path.UTF8String, response == NSAlertFirstButtonReturn); [self persistMacros];
        }];
    }];
}
- (void)exportMacros:(id)sender {
    NSSavePanel *panel = [NSSavePanel savePanel]; panel.allowedFileTypes = @[@"txt"];
    panel.nameFieldStringValue = @"OpenKeyMacro.txt";
    [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
        if (result == NSModalResponseOK) saveToFile(panel.URL.path.UTF8String);
    }];
}
- (void)changeConversion:(id)sender {
    convertToolFromCode = (Uint8)_fromCode.indexOfSelectedItem;
    convertToolToCode = (Uint8)_toCode.indexOfSelectedItem;
    NSInteger option = _caseOption.indexOfSelectedItem;
    convertToolToAllCaps = option == 1; convertToolToAllNonCaps = option == 2;
    convertToolToCapsFirstLetter = option == 3; convertToolToCapsEachWord = option == 4;
    NSUserDefaults *prefs = NSUserDefaults.standardUserDefaults;
    [prefs setInteger:convertToolFromCode forKey:@"convertToolFromCode"];
    [prefs setInteger:convertToolToCode forKey:@"convertToolToCode"];
    [prefs setBool:convertToolToAllCaps forKey:@"convertToolToAllCaps"];
    [prefs setBool:convertToolToAllNonCaps forKey:@"convertToolToAllNonCaps"];
    [prefs setBool:convertToolToCapsFirstLetter forKey:@"convertToolToCapsFirstLetter"];
    [prefs setBool:convertToolToCapsEachWord forKey:@"convertToolToCapsEachWord"];
}
- (void)reverseConversion:(id)sender {
    NSInteger code = _fromCode.indexOfSelectedItem;
    [_fromCode selectItemAtIndex:_toCode.indexOfSelectedItem]; [_toCode selectItemAtIndex:code];
    [self changeConversion:nil];
}
- (void)convertClipboard:(id)sender {
    BOOL converted = [OpenKeyManager quickConvert];
    if (!converted || !convertToolDontAlertWhenCompleted)
        [OpenKeyManager showMessage:self.window message:converted ? @"Đã chuyển mã clipboard" : @"Clipboard không có văn bản" subMsg:converted ? @"Dán để sử dụng kết quả." : @"Sao chép một đoạn văn bản rồi thử lại."];
}
@end
