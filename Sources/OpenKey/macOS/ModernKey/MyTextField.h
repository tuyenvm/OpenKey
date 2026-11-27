//
//  MyTextField.h
//  OpenKey
//
//  Created by Tuyen on 7/30/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@protocol MyTextFieldDelegate
@optional
-(void)onShortcutRecorded:(int)shortcut;
-(void)onMyTextFieldKeyChange:(unsigned short)keyCode character:(unsigned short)character;
@end

@interface MyTextField : NSTextField<NSTextDelegate>
@property (weak, nonatomic) id<MyTextFieldDelegate> Parent;
@property BOOL recordsShortcut;
-(void)displayShortcut:(int)shortcut;
@property unsigned short LastKeyCode;
@property unsigned short LastKeyChar;

-(void)setTextByChar:(unsigned short)chr;
@end
