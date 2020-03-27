//
//  ViewController.h
//  ModernKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "MyTextField.h"

@interface ViewController : NSViewController<MyTextFieldDelegate>
@property (strong) IBOutlet NSView *viewParent;
@property (weak) IBOutlet NSButton *tabbuttonPrimary;
@property (weak) IBOutlet NSButton *tabbuttonMacro;
@property (weak) IBOutlet NSButton *tabbuttonSystem;
@property (weak) IBOutlet NSButton *tabbuttonInfo;
@property (weak) IBOutlet NSBox *tabviewPrimary;
@property (weak) IBOutlet NSBox *tabviewMacro;
@property (weak) IBOutlet NSBox *tabviewSystem;
@property (weak) IBOutlet NSBox *tabviewInfo;

@property (weak) IBOutlet NSPopUpButton *popupInputType;
@property (weak) IBOutlet NSPopUpButton *popupCode;

@property (weak) IBOutlet NSBox *appOK;
@property (weak) IBOutlet NSBox *permissionWarning;
@property (weak) IBOutlet NSButton *retryButton;

@property (weak) IBOutlet NSButton *VietButton;
@property (weak) IBOutlet NSButton *EngButton;

@property (weak) IBOutlet NSButton *FreeMarkButton;
@property (weak) IBOutlet NSButton *UseModernOrthography;

@property (weak) IBOutlet NSButton *CheckSpellingButton;

@property (weak) IBOutlet NSButton *RunOnStartupButton;
@property (weak) IBOutlet NSButton *ShowUIButton;

@property (weak) IBOutlet NSButton *UseGrayIcon;
@property (weak) IBOutlet NSButton *QuickTelex;

@property (weak) IBOutlet NSButton *RestoreIfInvalidWord;
@property (weak) IBOutlet NSButton *FixRecommendBrowser;
@property (weak) IBOutlet NSButton *AllowZWJF;
@property (weak) IBOutlet NSButton *TempOffSpellChecking;

@property (weak) IBOutlet NSButton *UseMacro;
@property (weak) IBOutlet NSButton *UseMacroInEnglishMode;

@property (weak) IBOutlet NSButton *SendKeyStepByStep;
@property (weak) IBOutlet NSButton *AutoRememberSwitchKey;
@property (weak) IBOutlet NSButton *UpperCaseFirstChar;
@property (weak) IBOutlet NSButton *QuickStartConsonant;
@property (weak) IBOutlet NSButton *QuickEndConsonant;

@property (weak) IBOutlet NSButton *RememberTableCode;
@property (weak) IBOutlet NSButton *TempOffOpenKey;
@property (weak) IBOutlet NSButton *AutoCapsMacro;
@property (weak) IBOutlet NSButton *ShowIconOnDock;
@property (weak) IBOutlet NSButton *CheckNewVersionOnStartup;
@property (weak) IBOutlet NSButton *FixChromiumBrowser;
@property (weak) IBOutlet NSButton *PerformLayoutCompat;

@property (weak) IBOutlet NSButton *CheckNewVersionButton;
@property (weak) IBOutlet NSTextField *VersionInfo;

@property (weak) IBOutlet NSImageView *cursorImage;

-(void)fillData;
@end

