//
//  ViewController.h
//  ModernKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface ViewController : NSViewController

@property (weak) IBOutlet NSPopUpButton *popupInputType;
@property (weak) IBOutlet NSPopUpButton *popupCode;

@property (weak) IBOutlet NSBox *appOK;
@property (weak) IBOutlet NSBox *permissionWarning;
@property (weak) IBOutlet NSButton *retryButton;

@property (weak) IBOutlet NSButton *VietButton;
@property (weak) IBOutlet NSButton *EngButton;

@property (weak) IBOutlet NSButton *CtrlShiftButton;
@property (weak) IBOutlet NSButton *OptionZButton;

@property (weak) IBOutlet NSButton *FreeMarkButton;
@property (weak) IBOutlet NSButton *CheckSpellingButton;

@property (weak) IBOutlet NSButton *RunOnStartupButton;
@property (weak) IBOutlet NSButton *ShowUIButton;
@property (weak) IBOutlet NSImageView *cursorImage;

-(void)fillData;
@end

