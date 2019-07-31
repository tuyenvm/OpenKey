//
//  MyTextField.h
//  OpenKey
//
//  Created by Tuyen on 7/30/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ViewController.h"

@interface MyTextField : NSTextField
@property (weak, nonatomic) ViewController* Parent;

-(void)setTextByChar:(unsigned short)chr;
@end
