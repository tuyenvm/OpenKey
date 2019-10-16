//
//  AppDelegate.h
//  ModernKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ViewController.h"

#define OPENKEY_BUNDLE @"com.tuyenmai.openkey"

@interface AppDelegate : NSObject <NSApplicationDelegate>

-(void)onImputMethodChanged:(BOOL)willNotify;
-(void)onInputMethodSelected;

-(void)askPermission;

-(void)onInputTypeSelectedIndex:(int)index;
-(void)onCodeTableChanged:(int)index;

-(void)setRunOnStartup:(BOOL)val;
-(void)loadDefaultConfig;

-(void)setGrayIcon:(BOOL)val;

-(void)onMacroSelected;
-(void)onQuickConvert;
-(void)setQuickConvertString;

-(void)showIconOnDock:(BOOL)val;
@end

