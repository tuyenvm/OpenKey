//
//  MacroViewController.mm
//  OpenKey
//
//  Created by Tuyen on 8/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import "MacroViewController.h"
#include "Macro.h"
#include "Engine.h"



@interface MacroViewController ()

@end

@implementation MacroViewController{
    vector<vector<Uint32>> keys;
    vector<string> macroText;
    vector<string> macroContent;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.tableView.dataSource = self;
    self.tableView.delegate = self;
    
    self.macroName.delegate = self;
    self.macroContent.delegate = self;
    
    //load data
    getAllMacro(keys, macroText, macroContent);
}

-(void)saveAndReload {
    getAllMacro(keys, macroText, macroContent);
    [self.tableView reloadData];
    
    vector<Byte> macroData;
    getSaveData(macroData);
    NSData* _data = [NSData dataWithBytes:macroData.data() length:macroData.size()];
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    [prefs setObject:_data forKey:@"macroData"];
}

- (IBAction)onDeleteMacro:(id)sender {
    if ([[self.macroName stringValue] compare:@""] == 0) {
        [self showMessage:@"Bạn hãy chọn từ cần xoá!"];
        return;
    }
    string text = [[self.macroName stringValue] UTF8String];
    if (deleteMacro(text)) {
        [self saveAndReload];
    }
}

- (IBAction)onAddMacro:(id)sender {
    if ([[self.macroName stringValue] compare:@""] == 0 || [[self.macroContent stringValue] compare:@""] == 0) {
        [self showMessage:@"Bạn hãy nhập từ cần gõ tắt!"];
        return;
    }
    
    string text = [[self.macroName stringValue] UTF8String];
    string content = [[self.macroContent stringValue] UTF8String];
    if (addMacro(text, content)) {
        self.macroName.stringValue = @"";
        self.macroContent.stringValue = @"";
        [self saveAndReload];
    } else {
        [self showMessage:@"Bạn đã thêm từ này rồi!"];
    }
}

- (void)showMessage:(NSString*)msg {
    NSAlert* alert = [[NSAlert alloc] init];
    [alert setInformativeText:msg];
    [alert addButtonWithTitle:@"OK"];
    [alert setMessageText:@"Gõ tắt"];
    [alert setAlertStyle:NSCriticalAlertStyle];
    [alert beginSheetModalForWindow:self.view.window completionHandler:^(NSModalResponse returnCode) {
        
    }];
}

#pragma mark TableView
- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return keys.size();
}

- (nullable NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(nullable NSTableColumn *)tableColumn row:(NSInteger)row {
    NSString* cellId;
    NSTableCellView* v = nil;
    if (tableColumn == tableView.tableColumns[0]) {
        cellId = @"MacroCell";
        v = [tableView makeViewWithIdentifier:cellId owner:self];
        [v.textField setStringValue:[NSString stringWithUTF8String:macroText[row].c_str()]];
    } else if (tableColumn == tableView.tableColumns[1]) {
        cellId = @"ContentCell";
        v = [tableView makeViewWithIdentifier:cellId owner:self];
        [v.textField setStringValue:[NSString stringWithUTF8String:macroContent[row].c_str()]];
    }
    return v;
}

- (BOOL)tableView:(NSTableView *)tableView shouldSelectRow:(NSInteger)row {
    [self.macroName setStringValue:[NSString stringWithUTF8String:macroText[row].c_str()]];
    [self.macroContent setStringValue:[NSString stringWithUTF8String:macroContent[row].c_str()]];
    return YES;
}

@end
