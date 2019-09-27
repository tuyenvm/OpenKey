//
//  MacroViewController.mm
//  OpenKey
//
//  Created by Tuyen on 8/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import "MacroViewController.h"
#include "Engine.h"

#define MACRO_ADD_TEXT @"Thêm"
#define MACRO_EDIT_TEXT @"Sửa"

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
    
    self.AutoCapsMacro.state = vAutoCapsMacro ? NSControlStateValueOn : NSControlStateValueOff;
    
    //load data
    getAllMacro(keys, macroText, macroContent);
}

-(void)saveAndReload {
    getAllMacro(keys, macroText, macroContent);
    [self.tableView reloadData];
    
    vector<Byte> macroData;
    getMacroSaveData(macroData);
    NSData* _data = [NSData dataWithBytes:macroData.data() length:macroData.size()];
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    [prefs setObject:_data forKey:@"macroData"];
    [self.buttonAdd setTitle:MACRO_ADD_TEXT];
}

- (IBAction)onDeleteMacro:(id)sender {
    if ([[self.macroName stringValue] compare:@""] == 0) {
        [self showMessage:@"Bạn hãy chọn từ cần xoá!"];
        return;
    }
    string text = [[self.macroName stringValue] UTF8String];
    if (deleteMacro(text)) {
        [self saveAndReload];
        self.macroName.stringValue = @"";
        self.macroContent.stringValue = @"";
        [self.macroName becomeFirstResponder];
    }
}

- (IBAction)onAddMacro:(id)sender {
    if ([[self.macroName stringValue] compare:@""] == 0 || [[self.macroContent stringValue] compare:@""] == 0) {
        [self showMessage:@"Bạn hãy nhập từ cần gõ tắt!"];
        return;
    }
    
    string text = [[self.macroName stringValue] UTF8String];
    string content = [[self.macroContent stringValue] UTF8String];

    addMacro(text, content);
    self.macroName.stringValue = @"";
    self.macroContent.stringValue = @"";
    [self.macroName becomeFirstResponder];
    [self saveAndReload];
}

- (IBAction)onLoadFromFile:(id)sender {
    NSOpenPanel* openPanel = [NSOpenPanel openPanel];
    [openPanel setMessage:@"Chọn file dữ liệu gõ tắt"];
    [openPanel setCanChooseFiles:YES];
    [openPanel setAllowsMultipleSelection:NO];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setAllowedFileTypes:[NSArray arrayWithObjects:@"txt", nil]];
    [openPanel setExtensionHidden:NO];
    [openPanel setNameFieldStringValue:@"OpenKeyMacro"];
    [openPanel makeKeyAndOrderFront:nil];
    [openPanel setLevel:NSStatusWindowLevel];
    if ([openPanel runModal] == NSModalResponseOK ) {
        NSAlert* alert = [[NSAlert alloc] init];
        [alert setInformativeText:@"Bạn có muốn giữ lại các dữ liệu hiện tại không?"];
        [alert addButtonWithTitle:@"Có"];
        [alert addButtonWithTitle:@"Không"];
        [alert setMessageText:@"Dữ liệu gõ tắt"];
        [alert setAlertStyle:NSCriticalAlertStyle];
        [alert beginSheetModalForWindow:self.view.window completionHandler:^(NSModalResponse returnCode) {
            readFromFile(openPanel.URL.path.UTF8String, returnCode == 1000);
            [self saveAndReload];
        }];
    }
}

- (IBAction)onExportToFile:(id)sender {
    NSSavePanel* savePanel = [NSSavePanel savePanel];
    savePanel.canCreateDirectories = YES;
    [savePanel setMessage:@"Chọn nơi lưu dữ liệu gõ tắt"];
    [savePanel setTitle:@"Chọn nơi lưu dữ liệu gõ tắt"];
    [savePanel setAllowedFileTypes:[NSArray arrayWithObjects:@"txt", nil]];
    [savePanel setExtensionHidden:NO];
    [savePanel setNameFieldStringValue:@"OpenKeyMacro"];
    if ([savePanel runModal] == NSModalResponseOK) {
        saveToFile(savePanel.URL.path.UTF8String);
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

- (void)controlTextDidChange:(NSNotification *)notification {
    NSTextField *textField = [notification object];
    if (textField == self.macroName) {
        string text = [[self.macroName stringValue] UTF8String];
        if (hasMacro(text)) {
            [self.buttonAdd setTitle:MACRO_EDIT_TEXT];
        } else {
            [self.buttonAdd setTitle:MACRO_ADD_TEXT];
        }
    }
}

- (IBAction)onAutoCapButton:(NSButton *)sender {
    NSInteger val = sender.state == NSControlStateValueOn ? 1 : 0;
    vAutoCapsMacro = (int)val;
    [[NSUserDefaults standardUserDefaults] setInteger:vAutoCapsMacro forKey:@"vAutoCapsMacro"];
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
    [self.buttonAdd setTitle:MACRO_EDIT_TEXT];
    return YES;
}

@end
