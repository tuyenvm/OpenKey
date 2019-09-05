//
//  ConvertToolViewController.m
//  OpenKey
//
//  Created by Tuyen on 9/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import "ConvertToolViewController.h"
#import "OpenKeyManager.h"


@interface ConvertToolViewController ()

@end

@implementation ConvertToolViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    NSArray* codeData = [OpenKeyManager getTableCodes];
    [self.FromCode removeAllItems];
    [self.FromCode addItemsWithTitles:codeData];
    [self.ToCode removeAllItems];
    [self.ToCode addItemsWithTitles:codeData];
}

@end
