//
//  ViewController.m
//  ModernKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import "ViewController.h"
#import "OpenKeyManager.h"
#import "AppDelegate.h"

extern AppDelegate* appDelegate;

ViewController* viewController;
extern int vFreeMark;
extern int vCheckSpelling;

@implementation ViewController {
    
}

- (void)viewDidLoad {
    [super viewDidLoad];

    viewController = self;
    //[[[self view] window] windowController].window.title = @"ahihi";
    self.appOK.hidden = YES;
    self.permissionWarning.hidden = YES;
    self.retryButton.enabled = NO;
 
    NSArray* inputTypeData = [[NSArray alloc] initWithObjects:@"Telex", @"VNI", nil];
    NSArray* codeData = [[NSArray alloc] initWithObjects:
                         @"Unicode",
                         @"TCVN3 (ABC)",
                         @"VNI Windows",
                         @"Unicode tổ hợp",
                         @"Vietnamese Locale CP 1258", nil];
    
    //preset data
    [_popupInputType removeAllItems];
    [_popupInputType addItemsWithTitles:inputTypeData];
    
    [self.popupCode removeAllItems];
    [self.popupCode addItemsWithTitles:codeData];
    
    [self initKey];
    
    [self fillData];
}

- (void)viewDidAppear {
    [super viewDidAppear];
    NSString* str = @"OpenKey %@ - Bộ gõ Tiếng Việt";
    self.view.window.title = [NSString stringWithFormat:str, [[NSBundle mainBundle] objectForInfoDictionaryKey: @"CFBundleShortVersionString"]];
}

-(void)initKey {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (![OpenKeyManager initEventTap]) {
            self.permissionWarning.hidden = NO;
            self.retryButton.enabled = YES;
        } else {
            self.appOK.hidden = NO;
        }
    });
}

- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
}


- (IBAction)onInputTypeChanged:(NSPopUpButton *)sender {
    [appDelegate onInputTypeSelectedIndex:(int)[self.popupInputType indexOfSelectedItem]];
}

- (IBAction)onCodeTableChanged:(NSPopUpButton *)sender {
    [appDelegate onCodeTableChanged:(int)[self.popupCode indexOfSelectedItem]];
}

- (IBAction)onSwitchKeyChanged:(id)sender {
    [appDelegate onSwitchKeySelectedIndex:(int)((NSButton*)sender).tag];
}

- (IBAction)onLanguageChanged:(id)sender {
    [appDelegate onInputMethodSelected];
}

- (IBAction)onRestart:(id)sender {
    self.appOK.hidden = YES;
    self.permissionWarning.hidden = YES;
    self.retryButton.enabled = NO;
    
    [self initKey];
}

- (IBAction)onFreeMark:(NSButton *)sender {
    NSInteger val = 0;
    if (sender.state == NSControlStateValueOn) {
        val = 1;
    } else {
        val = 0;
    }
    [[NSUserDefaults standardUserDefaults] setInteger:val forKey:@"FreeMark"];
    vFreeMark = (int)val;
}

- (IBAction)onCheckSpelling:(NSButton *)sender {
    NSInteger val = 0;
    if (sender.state == NSControlStateValueOn) {
        val = 1;
    } else {
        val = 0;
    }
    [[NSUserDefaults standardUserDefaults] setInteger:val forKey:@"Spelling"];
    vCheckSpelling = (int)val;
}

- (IBAction)onShowUIOnStartup:(NSButton *)sender {
    NSInteger val = 0;
    if (sender.state == NSControlStateValueOn) {
        val = 1;
    } else {
        val = 0;
    }
    [[NSUserDefaults standardUserDefaults] setInteger:val forKey:@"ShowUIOnStartup"];
}

- (IBAction)onRunOnStartup:(NSButton *)sender {
    NSInteger val = 0;
    if (sender.state == NSControlStateValueOn) {
        val = 1;
    } else {
        val = 0;
    }
    [[NSUserDefaults standardUserDefaults] setInteger:val forKey:@"RunOnStartup"];
    
    [appDelegate setRunOnStartup:val];
}

- (IBAction)onTerminateApp:(id)sender {
    [NSApp terminate:0];
}

-(void)fillData {
    NSInteger intInputMethod = [[NSUserDefaults standardUserDefaults] integerForKey:@"InputMethod"];
    if (intInputMethod == 1) {
        self.VietButton.state = NSControlStateValueOn;
    } else if (intInputMethod == 0) {
        self.EngButton.state = NSControlStateValueOn;
    }
    
    NSInteger intInputType = [[NSUserDefaults standardUserDefaults] integerForKey:@"InputType"];
    [self.popupInputType selectItemAtIndex:intInputType];
    
    NSInteger intCodeTable = [[NSUserDefaults standardUserDefaults] integerForKey:@"CodeTable"];
    [self.popupCode selectItemAtIndex:intCodeTable];
    
    NSInteger intSwitchKey = [[NSUserDefaults standardUserDefaults] integerForKey:@"SwitchKey"];
    if (intSwitchKey == 1) {
        self.OptionZButton.state = NSControlStateValueOn;
    } else if (intSwitchKey == 0) {
        self.CtrlShiftButton.state = NSControlStateValueOn;
    }
    
    //option
    NSInteger showui = [[NSUserDefaults standardUserDefaults] integerForKey:@"ShowUIOnStartup"];
    if (showui == 0) {
        self.ShowUIButton.state = NSControlStateValueOff;
    } else {
        self.ShowUIButton.state = NSControlStateValueOn;
    }
    
    NSInteger freeMark = [[NSUserDefaults standardUserDefaults] integerForKey:@"FreeMark"];
    if (freeMark == 0) {
        self.FreeMarkButton.state = NSControlStateValueOff;
    } else {
        self.FreeMarkButton.state = NSControlStateValueOn;
    }
    
    NSInteger spelling = [[NSUserDefaults standardUserDefaults] integerForKey:@"Spelling"];
    if (spelling == 0) {
        self.CheckSpellingButton.state = NSControlStateValueOff;
    } else {
        self.CheckSpellingButton.state = NSControlStateValueOn;
    }
    
    NSInteger runOnStartup = [[NSUserDefaults standardUserDefaults] integerForKey:@"RunOnStartup"];
    if (runOnStartup == 0) {
        self.RunOnStartupButton.state = NSControlStateValueOff;
    } else {
        self.RunOnStartupButton.state = NSControlStateValueOn;
    }
}

- (IBAction)onOK:(id)sender {
    [self.view.window close];
}

- (IBAction)onDefaultConfig:(id)sender {
    [appDelegate loadDefaultConfig];
    [[NSUserDefaults standardUserDefaults] setInteger:0 forKey:@"ShowUIOnStartup"];
    self.ShowUIButton.state = NSControlStateValueOff;
    
    [[NSUserDefaults standardUserDefaults] setInteger:1 forKey:@"RunOnStartup"];
    self.RunOnStartupButton.state = NSControlStateValueOn;
}

@end
