#import "SettingsWindowController.h"
#import "SettingsForm.h"
#import "AppDelegate.h"
#import "OpenKeyManager.h"
#import "MJAccessibilityUtils.h"
#import "Engine.h"
#import "ConvertTool.h"
#import <Carbon/Carbon.h>

extern AppDelegate *appDelegate;
extern int vFixChromiumBrowser, vSendKeyStepByStep, vPerformLayoutCompat;
extern "C" void OnActiveAppChanged(void);
extern "C" void RequestNewSession(void);
extern "C" void OnSpellCheckingChanged(void);

@interface OKSettingsDocument : NSView
@end
@implementation OKSettingsDocument
- (BOOL)isFlipped { return YES; }
@end

@interface OKSettingsTabs : NSTabViewController
@end
@implementation OKSettingsTabs
- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)item {
    [super tabView:tabView didSelectTabViewItem:item];
    self.view.window.title = item.label;
    if (self.view.window) [self.view.window setContentSize:item.viewController.preferredContentSize];
    [[NSUserDefaults standardUserDefaults] setObject:item.identifier forKey:@"SettingsPane"];
}
@end

@implementation SettingsWindowController {
    OKSettingsTabs *_panes;
    NSViewController *_gate, *_auxForm;
    NSWindowController *_utilityWC;
    NSStackView *_content;
    NSMapTable<NSView *, NSDictionary *> *_formSpecs;
    NSMutableArray<NSViewController *> *_formControllers;
    NSMutableDictionary<NSString *, NSControl *> *_controls;
    NSPopUpButton *_inputType, *_encoding, *_fromCode, *_toCode;
    MyTextField *_inputKey;
    NSTableView *_rulesTable;
    NSArray<NSString *> *_ruleIDs;
    NSTimer *_timer;
    BOOL _wasTrusted, _gateVisible, _shown;
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
    if (@available(macOS 13.0, *)) {
        [_formSpecs setObject:@{@"title":title, @"detail":detail ?: @"", @"control":control} forKey:row];
        return row;
    }
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
    if (@available(macOS 13.0, *)) {
        NSMutableArray *specs = [NSMutableArray new];
        for (NSView *row in rows) [specs addObject:[_formSpecs objectForKey:row]];
        NSView *section = [NSView new];
        [_formSpecs setObject:@{@"title":title, @"rows":specs} forKey:section];
        [_content addArrangedSubview:section];
        return;
    }
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
    if (@available(macOS 13.0, *)) {} else [note.widthAnchor constraintEqualToAnchor:_content.widthAnchor constant:-4].active = YES;
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
    NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,640,460)
        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];
    self = [super initWithWindow:window]; if (!self) return nil;
    window.delegate=self; window.releasedWhenClosed=NO; [window center];
    _controls=[NSMutableDictionary new]; _formControllers=[NSMutableArray new];
    _panes=[OKSettingsTabs new]; _panes.tabStyle=NSTabViewControllerTabStyleToolbar;
    NSString *saved=[[NSUserDefaults standardUserDefaults] stringForKey:@"SettingsPane"];
    for (NSArray *pane in @[@[@"general",@"Chung",@"gearshape"], @[@"typing",@"Gõ tiếng Việt",@"keyboard"], @[@"apps",@"Ứng dụng",@"app.badge"], @[@"compatibility",@"Tương thích",@"slider.horizontal.3"]]) {
        NSTabViewItem *item=[NSTabViewItem tabViewItemWithViewController:[self formForPage:pane[0]]];
        item.identifier=pane[0]; item.label=pane[1];
        if (@available(macOS 11.0,*)) item.image=[NSImage imageWithSystemSymbolName:pane[2] accessibilityDescription:nil];
        [_panes addTabViewItem:item];
    }
    for (NSUInteger i=0;i<_panes.tabViewItems.count;i++) if ([_panes.tabViewItems[i].identifier isEqual:saved]) _panes.selectedTabViewItemIndex=i;
    [self refresh];
    return self;
}

- (NSViewController *)formForPage:(NSString *)page {
    _formSpecs=[NSMapTable strongToStrongObjectsMapTable]; _content=[self stack];
    if ([page isEqual:@"general"]) [self generalPage];
    else if ([page isEqual:@"typing"]) [self typingPage];
    else if ([page isEqual:@"apps"]) [self appsPage];
    else if ([page isEqual:@"compatibility"]) [self compatibilityPage];
    else [self conversionPage];
    NSViewController *controller;
    if (@available(macOS 13.0,*)) {
        NSMutableArray *entries=[NSMutableArray new];
        for (NSView *view in _content.arrangedSubviews)
            [entries addObject:[_formSpecs objectForKey:view] ?: @{@"view":view}];
        controller=[[OKSettingsFormController alloc] initWithEntries:entries];
    } else {
        controller=[NSViewController new]; NSScrollView *scroll=[NSScrollView new];
        scroll.hasVerticalScroller=YES; scroll.drawsBackground=NO; controller.view=scroll;
        OKSettingsDocument *document=[OKSettingsDocument new]; scroll.documentView=document;
        [document addSubview:_content]; document.translatesAutoresizingMaskIntoConstraints=NO;
        _content.translatesAutoresizingMaskIntoConstraints=NO;
        [NSLayoutConstraint activateConstraints:@[
            [document.widthAnchor constraintEqualToAnchor:scroll.contentView.widthAnchor],
            [_content.leadingAnchor constraintEqualToAnchor:document.leadingAnchor constant:20],
            [_content.trailingAnchor constraintEqualToAnchor:document.trailingAnchor constant:-20],
            [_content.topAnchor constraintEqualToAnchor:document.topAnchor constant:20],
            [_content.bottomAnchor constraintEqualToAnchor:document.bottomAnchor constant:-20]]];
    }
    controller.preferredContentSize=NSMakeSize(640,[page isEqual:@"typing"] ? 540 : [page isEqual:@"apps"] ? 420 : 340);
    [_formControllers addObject:controller]; return controller;
}

- (void)generalPage {
    _inputKey=[MyTextField new]; _inputKey.Parent=self; _inputKey.recordsShortcut=YES; _inputKey.editable=NO; _inputKey.selectable=NO;
    [_inputKey.widthAnchor constraintEqualToConstant:180].active=YES;
    [_inputKey setAccessibilityLabel:@"Phím chuyển ngôn ngữ"];
    [_inputKey setAccessibilityRole:NSAccessibilityButtonRole];
    [_inputKey setAccessibilityHelp:@"Bấm rồi nhấn tổ hợp phím. Delete để xóa, Escape để hủy."];
    [self group:@"" rows:@[
        [self option:@"Mở cùng macOS" key:@"RunOnStartup" detail:nil],
        [self row:@"Phím chuyển ngôn ngữ" detail:nil control:_inputKey],
        [self option:@"Âm báo khi chuyển ngôn ngữ" key:@"SwitchBeep" detail:nil]]];
}
- (void)typingPage {
    _inputType=[self popup:@[@"Telex",@"VNI",@"Simple Telex 1",@"Simple Telex 2"] action:@selector(changeInputType:)];
    _encoding=[self popup:[OpenKeyManager getTableCodes] action:@selector(changeEncoding:)];
    [_inputType setAccessibilityLabel:@"Kiểu gõ"]; [_encoding setAccessibilityLabel:@"Bảng mã"];
    [self group:@"" rows:@[
        [self row:@"Kiểu gõ" detail:nil control:_inputType],
        [self row:@"Bảng mã" detail:@"Unicode dùng cho hầu hết ứng dụng." control:_encoding]]];
    [self group:@"Chính tả và dấu" rows:@[
        [self option:@"Kiểm tra âm tiết" key:@"Spelling" detail:nil],
        [self option:@"Khôi phục từ không hợp lệ" key:@"RestoreIfInvalidWord" detail:nil],
        [self option:@"Cho phép z, w, j, f làm phụ âm" key:@"vAllowConsonantZFWJ" detail:nil],
        [self option:@"Đặt dấu oà, uý" key:@"ModernOrthography" detail:@"Tắt để dùng òa, úy."],
        [self option:@"Viết hoa đầu câu" key:@"UpperCaseFirstChar" detail:nil]]];
    [self group:@"Gõ nhanh" rows:@[
        [self option:@"Telex nhanh" key:@"QuickTelex" detail:@"cc → ch, gg → gi, kk → kh, nn → ng"],
        [self option:@"Phụ âm đầu nhanh" key:@"vQuickStartConsonant" detail:@"f → ph, j → gi, w → qu"],
        [self option:@"Phụ âm cuối nhanh" key:@"vQuickEndConsonant" detail:@"g → ng, h → nh, k → ch"]]];
    [self group:@"Tạm bỏ qua khi gõ" rows:@[
        [self option:@"Chạm Control để tắt kiểm tra chính tả" key:@"vTempOffSpelling" detail:@"Chỉ áp dụng cho từ đang gõ."],
        [self option:@"Chạm Command để tạm tắt OpenKey" key:@"vTempOffOpenKey" detail:@"Tự bật lại ở từ tiếp theo."]]];
}
- (void)compatibilityPage {
    [self group:@"" rows:@[
        [self option:@"Sửa lỗi gợi ý từ" key:@"FixRecommendBrowser" detail:@"Dành cho trình duyệt, Excel và ô tự hoàn thành."],
        [self option:@"Tương thích Chromium" key:@"vFixChromiumBrowser" detail:nil],
        [self option:@"Gửi từng phím riêng" key:@"SendKeyStepByStep" detail:nil],
        [self option:@"Theo bố cục bàn phím hiện tại" key:@"vPerformLayoutCompat" detail:nil],
        [self option:@"Bỏ qua khi macOS dùng bộ gõ khác" key:@"vOtherLanguage" detail:@"Tránh hai bộ gõ cùng xử lý tiếng Việt."]]];
}
- (NSViewController *)permissionGate {
    NSViewController *controller=[NSViewController new]; controller.view=[NSView new];
    NSStackView *stack=[self stack]; stack.alignment=NSLayoutAttributeCenterX; stack.spacing=18;
    [stack addArrangedSubview:[NSImageView imageViewWithImage:NSApp.applicationIconImage]];
    NSImageView *icon=(NSImageView *)stack.arrangedSubviews.firstObject;
    [icon.widthAnchor constraintEqualToConstant:64].active=YES; [icon.heightAnchor constraintEqualToConstant:64].active=YES;
    NSTextField *title=Text(@"Cho phép OpenKey hỗ trợ nhập liệu",20,NO); title.font=[NSFont boldSystemFontOfSize:20];
    title.alignment=NSTextAlignmentCenter; [stack addArrangedSubview:title];
    NSTextField *message=Text(@"OpenKey cần quyền Trợ năng để nhận và thay thế phím khi bạn gõ tiếng Việt.",13,YES);
    message.alignment=NSTextAlignmentCenter; [stack addArrangedSubview:message];
    [message.widthAnchor constraintLessThanOrEqualToConstant:440].active=YES;
    NSButton *open=[self button:@"Mở Cài đặt hệ thống…" action:@selector(openAccessibility:)]; open.keyEquivalent=@"\r";
    [stack addArrangedSubview:open]; [stack addArrangedSubview:[self button:@"Thoát OpenKey" action:@selector(quit:)]];
    [controller.view addSubview:stack]; stack.translatesAutoresizingMaskIntoConstraints=NO;
    [NSLayoutConstraint activateConstraints:@[
        [stack.centerXAnchor constraintEqualToAnchor:controller.view.centerXAnchor],
        [stack.centerYAnchor constraintEqualToAnchor:controller.view.centerYAnchor],
        [stack.leadingAnchor constraintGreaterThanOrEqualToAnchor:controller.view.leadingAnchor constant:32],
        [stack.trailingAnchor constraintLessThanOrEqualToAnchor:controller.view.trailingAnchor constant:-32]]];
    return controller;
}
- (void)showPage:(NSString *)page {
    if (![@[@"general",@"typing",@"apps",@"compatibility"] containsObject:page]) return;
    for (NSUInteger i=0;i<_panes.tabViewItems.count;i++) if ([_panes.tabViewItems[i].identifier isEqual:page]) _panes.selectedTabViewItemIndex=i;
    [self refresh];
}
- (void)showWindow:(id)sender {
    _shown=YES; [self refresh]; [super showWindow:sender];
    BOOL visible=NO;
    for (NSScreen *screen in NSScreen.screens) if (NSIntersectsRect(self.window.frame,screen.visibleFrame)) visible=YES;
    if (!visible) [self.window center];
    if (!_timer) {
        __weak SettingsWindowController *weakSelf=self;
        _timer=[NSTimer timerWithTimeInterval:1 repeats:YES block:^(NSTimer *timer){[weakSelf refresh];}];
        [[NSRunLoop mainRunLoop] addTimer:_timer forMode:NSRunLoopCommonModes];
    }
}
- (void)windowDidResignKey:(NSNotification *)note {
    if (self.window.firstResponder==_inputKey) [self.window makeFirstResponder:nil];
}
- (void)windowWillClose:(NSNotification *)note { [self.window makeFirstResponder:nil]; _shown=NO; [_timer invalidate]; _timer=nil; }
- (void)dealloc { [_timer invalidate]; }
- (void)refresh {
    BOOL trusted=MJAccessibilityIsEnabled();
    if (trusted && !_wasTrusted && ![OpenKeyManager isInited] && [OpenKeyManager resumeEventTap]) OnActiveAppChanged();
    if (!trusted && [OpenKeyManager isInited]) [OpenKeyManager stopEventTap];
    if (!trusted && !_gateVisible) {
        [self.window makeFirstResponder:nil];
        if (_utilityWC.window.sheetParent) [self.window endSheet:_utilityWC.window];
        [_utilityWC close];
        _gate=[self permissionGate]; self.window.toolbar=nil; self.window.contentViewController=_gate;
        self.window.title=@"Thiết lập OpenKey"; [self.window setContentSize:NSMakeSize(640,380)]; _gateVisible=YES;
    } else if (trusted && (_gateVisible || !self.window.contentViewController)) {
        BOOL dismiss=_gateVisible && _shown;
        self.window.contentViewController=_panes; (void)_panes.view;
        self.window.title=_panes.tabViewItems[_panes.selectedTabViewItemIndex].label;
        [self.window setContentSize:_panes.tabViewItems[_panes.selectedTabViewItemIndex].viewController.preferredContentSize]; _gate=nil; _gateVisible=NO;
        if (dismiss) [self close];
    }
    _wasTrusted=trusted;
    if (_inputType.indexOfSelectedItem!=vInputType) [_inputType selectItemAtIndex:MAX(0,MIN(3,vInputType))];
    if (_encoding.indexOfSelectedItem!=vCodeTable) [_encoding selectItemAtIndex:MAX(0,MIN(4,vCodeTable))];
    for (NSString *key in _controls) { id control=_controls[key]; [control setState:[self optionValue:key]]; }
    _controls[@"RestoreIfInvalidWord"].enabled=vCheckSpelling;
    if (self.window.firstResponder!=_inputKey) [_inputKey displayShortcut:vSwitchKeyStatus];
    if (@available(macOS 13.0,*)) for (NSViewController *form in _formControllers) [(OKSettingsFormController *)form refreshControls];
}
- (NSDictionary<NSString *,NSValue *> *)optionBindings {
    return @{@"Spelling":[NSValue valueWithPointer:&vCheckSpelling], @"ModernOrthography":[NSValue valueWithPointer:&vUseModernOrthography],
        @"QuickTelex":[NSValue valueWithPointer:&vQuickTelex], @"RestoreIfInvalidWord":[NSValue valueWithPointer:&vRestoreIfWrongSpelling],
        @"UseSmartSwitchKey":[NSValue valueWithPointer:&vUseSmartSwitchKey], @"vRememberCode":[NSValue valueWithPointer:&vRememberCode],
        @"vAllowConsonantZFWJ":[NSValue valueWithPointer:&vAllowConsonantZFWJ], @"UpperCaseFirstChar":[NSValue valueWithPointer:&vUpperCaseFirstChar],
        @"vQuickStartConsonant":[NSValue valueWithPointer:&vQuickStartConsonant], @"vQuickEndConsonant":[NSValue valueWithPointer:&vQuickEndConsonant],
        @"vTempOffSpelling":[NSValue valueWithPointer:&vTempOffSpelling], @"vTempOffOpenKey":[NSValue valueWithPointer:&vTempOffOpenKey],
        @"FixRecommendBrowser":[NSValue valueWithPointer:&vFixRecommendBrowser], @"vFixChromiumBrowser":[NSValue valueWithPointer:&vFixChromiumBrowser],
        @"SendKeyStepByStep":[NSValue valueWithPointer:&vSendKeyStepByStep], @"vPerformLayoutCompat":[NSValue valueWithPointer:&vPerformLayoutCompat],
        @"vOtherLanguage":[NSValue valueWithPointer:&vOtherLanguage]};
}
- (BOOL)optionValue:(NSString *)key {
    if ([key isEqual:@"SwitchBeep"]) return (vSwitchKeyStatus & 0x8000)!=0;
    NSValue *binding=[self optionBindings][key];
    return binding ? *(int *)binding.pointerValue!=0 : [[NSUserDefaults standardUserDefaults] boolForKey:key];
}
- (void)changeOption:(id)sender {
    NSString *key=[sender identifier]; BOOL value=[sender state]==NSControlStateValueOn;
    if ([key isEqual:@"SwitchBeep"]) {
        vSwitchKeyStatus=(vSwitchKeyStatus & ~0x8000) | (value ? 0x8000 : 0);
        [[NSUserDefaults standardUserDefaults] setInteger:vSwitchKeyStatus forKey:@"SwitchKeyStatus"];
        [self refresh]; return;
    }
    NSValue *binding=[self optionBindings][key]; if (binding) *(int *)binding.pointerValue=value;
    [[NSUserDefaults standardUserDefaults] setBool:value forKey:key];
    if ([key isEqual:@"Spelling"]) OnSpellCheckingChanged();
    if ([key isEqual:@"RunOnStartup"]) [appDelegate setRunOnStartup:value];
    if ([key isEqual:@"UseSmartSwitchKey"] || [key isEqual:@"vRememberCode"]) OnActiveAppChanged();
    RequestNewSession(); [self refresh];
}
- (void)onShortcutRecorded:(int)shortcut {
    vSwitchKeyStatus=shortcut | (vSwitchKeyStatus & 0x8000); [[NSUserDefaults standardUserDefaults] setInteger:vSwitchKeyStatus forKey:@"SwitchKeyStatus"];
    RequestNewSession(); [self refresh];
}
- (void)changeInputType:(id)sender {
    [appDelegate onInputTypeSelectedIndex:(int)_inputType.indexOfSelectedItem]; RequestNewSession(); [self refresh];
}
- (void)openAccessibility:(id)sender { MJAccessibilityOpenPanel(); }
- (void)quit:(id)sender { [NSApp terminate:nil]; }
- (void)changeEncoding:(id)sender { [appDelegate onCodeTableChanged:(int)_encoding.indexOfSelectedItem]; RequestNewSession(); [self refresh]; }
- (void)showUtility:(NSString *)page title:(NSString *)title {
    if (!MJAccessibilityIsEnabled()) { [self showWindow:nil]; return; }
    if (_utilityWC.window.sheetParent) return;
    if (_auxForm) [_formControllers removeObject:_auxForm];
    _auxForm=[self formForPage:page];
    NSWindow *window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,640,420) styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];
    window.title=title;
    NSViewController *root=[NSViewController new]; root.view=[NSView new];
    [root addChildViewController:_auxForm]; window.contentViewController=root;
    NSView *form=_auxForm.view; [root.view addSubview:form]; form.translatesAutoresizingMaskIntoConstraints=NO;
    _utilityWC=[[NSWindowController alloc] initWithWindow:window];
    NSButton *done=[self button:@"Xong" action:@selector(closeUtility:)]; done.keyEquivalent=@"\r";
    [root.view addSubview:done]; done.translatesAutoresizingMaskIntoConstraints=NO;
    [NSLayoutConstraint activateConstraints:@[
        [form.topAnchor constraintEqualToAnchor:root.view.topAnchor],
        [form.leadingAnchor constraintEqualToAnchor:root.view.leadingAnchor],
        [form.trailingAnchor constraintEqualToAnchor:root.view.trailingAnchor],
        [form.bottomAnchor constraintEqualToAnchor:done.topAnchor constant:-12],
        [done.trailingAnchor constraintEqualToAnchor:root.view.trailingAnchor constant:-20],
        [done.bottomAnchor constraintEqualToAnchor:root.view.bottomAnchor constant:-12]]];
    [window setContentSize:NSMakeSize(640,420)];
    [self.window beginSheet:window completionHandler:nil];
}
- (void)closeUtility:(id)sender { [self.window endSheet:_utilityWC.window]; [_utilityWC close]; }
- (void)showConversion { [self showUtility:@"conversion" title:@"Chuyển mã văn bản"]; }
- (void)appsPage {
    [self group:@"" rows:@[
        [self option:@"Nhớ ngôn ngữ theo ứng dụng" key:@"UseSmartSwitchKey" detail:nil],
        [self option:@"Nhớ bảng mã theo ứng dụng" key:@"vRememberCode" detail:nil]]];
    _rulesTable=[self tableWithColumns:@[@"Ứng dụng",@"Ngôn ngữ"] identifiers:@[@"app",@"rule"] height:200];
    [self reloadRules];
    [_content addArrangedSubview:[NSStackView stackViewWithViews:@[[self button:@"Thêm…" action:@selector(addApp:)],
        [self button:@"Xóa quy tắc" action:@selector(removeApp:)]]]];
    [self note:@"Quy tắc cố định được ưu tiên hơn phím chuyển ngôn ngữ."];
}
- (void)conversionPage {
    _fromCode=[self popup:[OpenKeyManager getTableCodes] action:@selector(changeConversion:)];
    _toCode=[self popup:[OpenKeyManager getTableCodes] action:@selector(changeConversion:)];
    [self group:@"" rows:@[[self row:@"Từ bảng mã" detail:nil control:_fromCode], [self row:@"Sang bảng mã" detail:nil control:_toCode]]];
    [_fromCode selectItemAtIndex:MIN(4,convertToolFromCode)]; [_toCode selectItemAtIndex:MIN(4,convertToolToCode)];
    [_content addArrangedSubview:[self button:@"Chuyển mã clipboard" action:@selector(convertClipboard:)]];
    [self note:@"Sao chép văn bản trước khi chuyển. Kết quả sẽ thay thế nội dung clipboard."];
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
    [_content addArrangedSubview:scroll]; if (@available(macOS 13.0, *)) {} else [scroll.widthAnchor constraintEqualToAnchor:_content.widthAnchor].active = YES;
    return table;
}


- (NSInteger)numberOfRowsInTableView:(NSTableView *)table { return _ruleIDs.count; }
- (NSView *)tableView:(NSTableView *)table viewForTableColumn:(NSTableColumn *)column row:(NSInteger)row {
    NSString *bundleID=_ruleIDs[row];
    if ([column.identifier isEqual:@"app"]) {
        NSURL *url=[[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:bundleID];
        return [NSTextField labelWithString:url ? [[NSFileManager defaultManager] displayNameAtPath:url.path] : bundleID];
    }
    NSPopUpButton *popup=[self popup:@[@"English",@"Tiếng Việt",@"Nhớ lựa chọn"] action:@selector(changeRule:)];
    id value=[self rules][bundleID];
    NSInteger selected=[value isKindOfClass:NSNumber.class] ? [value integerValue] : 2;
    [popup selectItemAtIndex:MAX(0,MIN(2,selected))]; popup.identifier=bundleID;
    [popup setAccessibilityLabel:[NSString stringWithFormat:@"Quy tắc %@",bundleID]]; return popup;
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


- (void)changeConversion:(id)sender {
    convertToolFromCode=(Uint8)_fromCode.indexOfSelectedItem; convertToolToCode=(Uint8)_toCode.indexOfSelectedItem;
    NSUserDefaults *prefs=NSUserDefaults.standardUserDefaults;
    [prefs setInteger:convertToolFromCode forKey:@"convertToolFromCode"]; [prefs setInteger:convertToolToCode forKey:@"convertToolToCode"];
}
- (void)convertClipboard:(id)sender {
    convertToolToAllCaps=convertToolToAllNonCaps=convertToolToCapsFirstLetter=convertToolToCapsEachWord=convertToolRemoveMark=false;
    [self changeConversion:nil]; BOOL converted=[OpenKeyManager quickConvert];
    [OpenKeyManager showMessage:_utilityWC.window message:converted ? @"Đã chuyển mã clipboard" : @"Clipboard không có văn bản"
        subMsg:converted ? @"Dán để sử dụng kết quả." : @"Sao chép một đoạn văn bản rồi thử lại."];
}
@end
