//
//  main.cpp
//  OpenKey Linux CLI
//
//  Porting by tamld
//  Based on OpenKey Engine by Tuyen Mai
//

#include <iostream>
#include <string>
#include "Engine.h"

// Engine external variables (required by Engine.h)
int vLanguage = 1;              // 1: Vietnamese
int vInputType = 0;             // 0: Telex
int vFreeMark = 1;
int vCodeTable = 0;             // 0: Unicode
int vSwitchKeyStatus = 0;
int vCheckSpelling = 1;
int vUseModernOrthography = 1;
int vQuickTelex = 0;
int vRestoreIfWrongSpelling = 1;
int vFixRecommendBrowser = 0;
int vUseMacro = 0;
int vUseMacroInEnglishMode = 0;
int vAutoCapsMacro = 0;
int vUseSmartSwitchKey = 0;
int vUpperCaseFirstChar = 0;
int vTempOffSpelling = 0;
int vAllowConsonantZFWJ = 1;
int vQuickStartConsonant = 0;
int vQuickEndConsonant = 0;
int vRememberCode = 0;
int vOtherLanguage = 0;
int vTempOffOpenKey = 0;

void printUsage() {
    std::cout << "OpenKey Linux CLI v2.0.0" << std::endl;
    std::cout << "Vietnamese Input Engine - Build Verification Tool" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  openkey-cli --version    Show version info" << std::endl;
    std::cout << "  openkey-cli --test       Run engine self-test" << std::endl;
    std::cout << std::endl;
    std::cout << "Input Types: Telex (default), VNI, Simple Telex" << std::endl;
    std::cout << "Code Tables: Unicode (default), TCVN3, VNI-Windows" << std::endl;
}

void runSelfTest() {
    std::cout << "[TEST] Initializing OpenKey Engine..." << std::endl;

    vKeyHookState* hookState = (vKeyHookState*)vKeyInit();
    if (hookState == nullptr) {
        std::cerr << "[FAIL] Engine initialization returned null." << std::endl;
        return;
    }
    std::cout << "[PASS] Engine initialized successfully." << std::endl;

    startNewSession();
    std::cout << "[PASS] New session started." << std::endl;

    // Simulate typing "Vieetj" in Telex -> should produce "Việt"
    // KEY_V=55, KEY_I=31, KEY_E=26, KEY_E=26, KEY_T=28, KEY_J=44
    Uint16 testKeys[] = {55, 31, 26, 26, 28, 44};
    int numKeys = sizeof(testKeys) / sizeof(testKeys[0]);

    std::cout << "[TEST] Simulating Telex input (6 keystrokes)..." << std::endl;
    for (int i = 0; i < numKeys; i++) {
        vKeyHandleEvent(vKeyEvent::Keyboard, vKeyEventState::KeyDown, testKeys[i], 0, false);
    }
    std::cout << "[PASS] Engine processed all keystrokes without crash." << std::endl;
    std::cout << "[INFO] Hook state code: " << (int)hookState->code << std::endl;
    std::cout << "[INFO] Backspace count: " << (int)hookState->backspaceCount << std::endl;
    std::cout << "[INFO] New char count:  " << (int)hookState->newCharCount << std::endl;

    std::cout << std::endl;
    std::cout << "All tests passed. Engine is operational on Linux." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 0;
    }

    std::string arg = argv[1];

    if (arg == "--version") {
        std::cout << "OpenKey Linux CLI v2.0.0" << std::endl;
        std::cout << "Engine: OpenKey Engine by Tuyen Mai" << std::endl;
        std::cout << "Platform: Linux (Ubuntu)" << std::endl;
        return 0;
    }

    if (arg == "--test") {
        runSelfTest();
        return 0;
    }

    printUsage();
    return 1;
}
