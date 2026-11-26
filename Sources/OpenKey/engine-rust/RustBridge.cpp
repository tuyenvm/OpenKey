// Compatibility glue for the existing Objective-C++ UI. Typing and data logic live in Rust.
#include "../engine/Engine.h"
#include "OpenKeyRust.h"
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>

namespace {
struct EngineDeleter { void operator()(OkEngine *p) const { ok_engine_destroy(p); } };
std::unique_ptr<OkEngine, EngineDeleter> engine(ok_engine_create());
vKeyHookState hook;
bool spellingBefore=false;
OkConfig config() {
    return {vInputType, vCodeTable, vCheckSpelling, vQuickTelex, vUseModernOrthography,
        vRestoreIfWrongSpelling, vUseMacro, vAutoCapsMacro, vFreeMark, vUpperCaseFirstChar,
        vAllowConsonantZFWJ, vQuickStartConsonant, vQuickEndConsonant};
}
struct Bytes {
    OkBuffer value;
    explicit Bytes(uint32_t op, const void *input=nullptr, size_t size=0, uint32_t flags=0) :
        value(ok_engine_command(engine.get(), op, static_cast<const uint8_t *>(input), size,
                                static_cast<uint32_t>(vCodeTable), flags)) {}
    ~Bytes() { ok_buffer_free(value); }
    Bytes(const Bytes&)=delete;
    Bytes& operator=(const Bytes&)=delete;
    std::string string() const { return value.len ? std::string(reinterpret_cast<char *>(value.data),value.len) : std::string(); }
    void words(std::vector<Uint32>& out) const {
        out.clear();
        if (!value.status || value.len%4) return;
        out.resize(value.len/4);
        if (value.len) std::memcpy(out.data(),value.data,value.len);
    }
    void bytes(std::vector<Byte>& out) const {
        out.clear();
        if (value.len) out.assign(value.data,value.data+value.len);
    }
};
void process(uint32_t event,uint32_t state,uint32_t key,uint32_t caps,bool control,bool english) {
    OkResult result{}; auto options=config();
    if (!ok_engine_process(engine.get(),&options,event,state,key,caps,control,english,&result)) {
        hook.code=vDoNothing;hook.backspaceCount=hook.newCharCount=0;hook.extCode=1;return;
    }
    hook.code=result.code;hook.backspaceCount=result.backspaces;
    hook.newCharCount=result.count;hook.extCode=result.ext;
    std::memcpy(hook.charData,result.chars,sizeof result.chars);
    vCheckSpelling=result.spelling;
    if (hook.code==vReplaceMaro) Bytes(14).words(hook.macroData);
}
}

Uint16 _unicodeCompoundMark[]={0x0301,0x0300,0x0309,0x0303,0x0323};
bool convertToolDontAlertWhenCompleted=false;
bool convertToolToAllCaps=false,convertToolToAllNonCaps=false;
bool convertToolToCapsFirstLetter=false,convertToolToCapsEachWord=false,convertToolRemoveMark=false;
Uint8 convertToolFromCode=0,convertToolToCode=0;
int convertToolHotKey=0;

void* vKeyInit() { spellingBefore=vCheckSpelling;auto options=config();ok_engine_reset(engine.get(),&options,1);return &hook; }
void startNewSession() { auto options=config();ok_engine_reset(engine.get(),&options,0); }
Uint32 getCharacterCode(const Uint32& data) { return ok_character_code(data,vCodeTable); }
Uint16 keyCodeToCharacter(const Uint32& key) { return static_cast<Uint16>(ok_key_character(key)); }
void vKeyHandleEvent(const vKeyEvent& event,const vKeyEventState& state,const Uint16& data,const Uint8& caps,const bool& control) {
    process(event,state,data,caps,control,false);
}
void vEnglishMode(const vKeyEventState& state,const Uint16& data,const bool& caps,const bool& control) {process(0,state,data,caps,control,true);}
void vTempOffSpellChecking() {if(spellingBefore)vCheckSpelling=!vCheckSpelling;}
void vSetCheckSpelling() {spellingBefore=vCheckSpelling;ok_engine_option(engine.get(),0,vCheckSpelling);}
void vTempOffEngine(const bool& off) {ok_engine_option(engine.get(),1,off);}
wstring utf8ToWideString(const string& str) {std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;return converter.from_bytes(str);}
string wideStringToUtf8(const wstring& str) {std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;return converter.to_bytes(str);}

void initMacroMap(const Byte* data,const int& size) {if(size>=0) Bytes(0,data,size);}
void getMacroSaveData(vector<Byte>& out) {Bytes(1).bytes(out);}
bool findMacro(vector<Uint32>& key,vector<Uint32>& content) {
    for (auto& value:key) value=getCharacterCode(value);
    Bytes bytes(13,key.data(),key.size()*sizeof(Uint32),vAutoCapsMacro);
    if(!bytes.value.status) return false;bytes.words(content);return true;
}
bool hasMacro(const string& name) {return Bytes(4,name.data(),name.size()).value.status;}
bool addMacro(const string& name,const string& content) {
    string data=name;data.push_back(0);data+=content;return Bytes(2,data.data(),data.size()).value.status;
}
bool deleteMacro(const string& name) {return Bytes(3,name.data(),name.size()).value.status;}
void onTableCodeChange() {} // Rust derives encoded macro content for the current table on demand.
void getAllMacro(vector<vector<Uint32>>& keys,vector<string>& names,vector<string>& contents) {
    keys.clear();names.clear();contents.clear();Bytes bytes(5);
    // Rust returns the same length-prefixed format as persisted macroData.
    if(!bytes.value.status||bytes.value.len<2) return;
    size_t cursor=2;auto data=bytes.value.data;auto size=bytes.value.len;
    unsigned count=data[0]|(unsigned(data[1])<<8);
    for(unsigned i=0;i<count;++i) {
        if(cursor>=size) return;size_t n=data[cursor++];if(n>size-cursor) return;
        string name(reinterpret_cast<char*>(data+cursor),n);cursor+=n;
        if(size-cursor<2) return;size_t m=data[cursor]|(size_t(data[cursor+1])<<8);cursor+=2;
        if(m>size-cursor) return;string content(reinterpret_cast<char*>(data+cursor),m);cursor+=m;
        vector<Uint32> key;Bytes(15,name.data(),name.size()).words(key);
        keys.push_back(key);names.push_back(name);contents.push_back(content);
    }
}
void saveToFile(const string& path) {Bytes bytes(7);if(bytes.value.status){std::ofstream file(path,std::ios::binary);file<<bytes.string();}}
void readFromFile(const string& path,const bool& append) {
    std::ifstream file(path,std::ios::binary);if(!file)return;
    string data((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    Bytes bytes(6,data.data(),data.size(),append);
}
void initSmartSwitchKey(const Byte* data,const int& size) {if(size>=0) Bytes(8,data,size);}
void getSmartSwitchKeySaveData(vector<Byte>& out) {Bytes(9).bytes(out);}
int getAppInputMethodStatus(const string& bundle,const int& current) {
    Bytes bytes(10,bundle.data(),bundle.size(),current);int32_t result=-1;
    if(bytes.value.status&&bytes.value.len==sizeof result)std::memcpy(&result,bytes.value.data,sizeof result);return result;
}
void setAppInputMethodStatus(const string& bundle,const int& language) {Bytes bytes(11,bundle.data(),bundle.size(),language);}
string convertUtil(const string& source) {
    uint32_t flags=uint32_t(convertToolFromCode)<<24;
    flags|=convertToolToAllCaps?1:0;flags|=convertToolToAllNonCaps?2:0;
    flags|=convertToolToCapsFirstLetter?4:0;flags|=convertToolToCapsEachWord?8:0;flags|=convertToolRemoveMark?16:0;
    OkBuffer result=ok_engine_command(engine.get(),12,reinterpret_cast<const uint8_t*>(source.data()),source.size(),convertToolToCode,flags);
    string out=result.status?string(reinterpret_cast<char*>(result.data),result.len):source;ok_buffer_free(result);return out;
}

#ifdef OPENKEY_FIXTURE_TEST
void OpenKeyRustResetForTests() {engine.reset(ok_engine_create());hook=vKeyHookState{};}
#endif

extern "C" void OpenKeyResetComposition() {
    vCheckSpelling=spellingBefore;
    auto options=config();ok_engine_clear(engine.get(),&options);
    hook=vKeyHookState{};
}
