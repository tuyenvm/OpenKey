// Compile against either the original C++ core or RustBridge.cpp; compare visible text after every key.
#include "../Sources/OpenKey/engine/Engine.h"
#include <iostream>
#include <sstream>
#ifdef OPENKEY_REFERENCE
#include "../Sources/OpenKey/engine/Engine.cpp"
#endif
int vLanguage=1,vInputType=0,vCodeTable=0,vFreeMark=0,vCheckSpelling=1;
int vSwitchKeyStatus=0,vUseModernOrthography=0,vQuickTelex=0,vRestoreIfWrongSpelling=0;
int vFixRecommendBrowser=0,vUseMacro=0,vUseMacroInEnglishMode=0,vAutoCapsMacro=0,vUseSmartSwitchKey=0;
int vUpperCaseFirstChar=0,vTempOffSpelling=0,vAllowConsonantZFWJ=0,vQuickStartConsonant=0,vQuickEndConsonant=0;
int vRememberCode=0,vOtherLanguage=0,vTempOffOpenKey=0;
uint32_t rendered(uint32_t value) {return value&(CHAR_CODE_MASK|PURE_CHARACTER_MASK)?value&0xffff:keyCodeToCharacter(value);}
int main() {
    std::string line;
    while(std::getline(std::cin,line)) {
        auto split=line.find('\t');if(split==std::string::npos)return 2;
        std::istringstream config(line.substr(0,split));int profile;
        config>>vInputType>>vCodeTable>>vUseModernOrthography>>profile;
        vCheckSpelling=!(profile&1);vRestoreIfWrongSpelling=!!(profile&2);vQuickTelex=!!(profile&4);
        vQuickStartConsonant=!!(profile&8);vQuickEndConsonant=!!(profile&16);vAllowConsonantZFWJ=!!(profile&32);
        vUpperCaseFirstChar=!!(profile&64);vUseMacro=!!(profile&128);vAutoCapsMacro=!!(profile&256);
#ifdef OPENKEY_REFERENCE
        _spaceCount=0;_specialChar.clear();_upperCaseStatus=0;_willTempOffEngine=false;
        _hasHandledMacro=false;_hasHandleQuickConsonant=false;
        std::fill(std::begin(TypingWord),std::end(TypingWord),0);
        HookState=vKeyHookState{};
#else
        // An independent Rust handle per case prevents history leaking between fixtures.
        extern void OpenKeyRustResetForTests();OpenKeyRustResetForTests();
#endif
        auto* hook=static_cast<vKeyHookState*>(vKeyInit());
        vKeyHandleEvent(vKeyEvent::Mouse,vKeyEventState::MouseDown,0);startNewSession();
        initMacroMap(nullptr,0);addMacro("btw","by the way");addMacro("ms","một giây 😀");addMacro("đc","địa chỉ");
        vector<uint32_t> text;std::string input=line.substr(split+1);std::ostringstream trace;
        for(char c:input) {
            uint32_t key=0;bool caps=false;
            if(c=='<')key=KEY_DELETE;
            else if(c=='|'){vKeyHandleEvent(vKeyEvent::Mouse,vKeyEventState::MouseDown,0);trace<<"|;";continue;}
            else if(c=='^')key=KEY_LEFT;
            else {
                for(uint32_t k=0;k<128;++k) {
                    if(keyCodeToCharacter(k)==static_cast<unsigned char>(c)){key=k;break;}
                    if(keyCodeToCharacter(k|CAPS_MASK)==static_cast<unsigned char>(c)){key=k;caps=true;break;}
                }
            }
            vKeyHandleEvent(vKeyEvent::Keyboard,vKeyEventState::KeyDown,key,caps?1:0);
            if(hook->code==vDoNothing){if(key==KEY_DELETE){if(!text.empty())text.pop_back();}else if(key!=KEY_LEFT){text.push_back(rendered(key|(caps?CAPS_MASK:0)));}}
            else {
                for(int i=0;i<hook->backspaceCount&&!text.empty();++i)text.pop_back();
                if(hook->code==vReplaceMaro) {for(auto value:hook->macroData)text.push_back(rendered(value));text.push_back(rendered(key|(caps?CAPS_MASK:0)));}
                else for(int i=hook->newCharCount-1;i>=0;--i)text.push_back(rendered(hook->charData[i]));
                if(hook->code==vRestore||hook->code==vRestoreAndStartNewSession){if(key!=KEY_LEFT)text.push_back(rendered(key|(caps?CAPS_MASK:0)));}
                if(hook->code==vRestoreAndStartNewSession)startNewSession();
            }
            for(auto x:text)trace<<x<<',';trace<<';';
        }
        std::cout<<line<<'\t'<<trace.str()<<'\n';
    }
}
