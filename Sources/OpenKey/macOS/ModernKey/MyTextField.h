//
//  MyTextField.h
//  OpenKey
//
//  Created by Tuyen on 7/30/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ViewController.h"

@interface MyTextField : NSTextField<NSTextDelegate>
@property (weak, nonatomic) ViewController* Parent;
@property unsigned short LastKeyCode;
@property unsigned short LastKeyChar;

-(void)setTextByChar:(unsigned short)chr;
@end
