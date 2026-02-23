#include <QSettings>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>

#include <cstring>
#include <string>

// OpenKey Engine headers
#include "Engine.h"

// ============================================================
// Engine extern variables (required by Engine.h)
// These mirror the defaults in linux/main.cpp
// ============================================================
int vLanguage = 1;  // 1: Vietnamese
int vInputType = 0; // 0: Telex
int vFreeMark = 1;
int vCodeTable = 0; // 0: Unicode
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

// ============================================================
// Fcitx5 keysym → OpenKey X11 keycode mapping
// OpenKey's linux.h defines X11 hardware keycodes.
// Fcitx5 provides XKB keysyms (e.g., FcitxKey_a = 0x61).
// This function maps the keysym to the X11 keycode that
// OpenKey expects.
// ============================================================
static Uint16 fcitxKeySymToOpenKey(uint32_t sym) {
  // Letters A-Z (lowercase and uppercase share the same keycode)
  if (sym >= 'a' && sym <= 'z')
    sym -= 32; // normalize to uppercase
  switch (sym) {
  case 'A':
    return KEY_A;
  case 'B':
    return KEY_B;
  case 'C':
    return KEY_C;
  case 'D':
    return KEY_D;
  case 'E':
    return KEY_E;
  case 'F':
    return KEY_F;
  case 'G':
    return KEY_G;
  case 'H':
    return KEY_H;
  case 'I':
    return KEY_I;
  case 'J':
    return KEY_J;
  case 'K':
    return KEY_K;
  case 'L':
    return KEY_L;
  case 'M':
    return KEY_M;
  case 'N':
    return KEY_N;
  case 'O':
    return KEY_O;
  case 'P':
    return KEY_P;
  case 'Q':
    return KEY_Q;
  case 'R':
    return KEY_R;
  case 'S':
    return KEY_S;
  case 'T':
    return KEY_T;
  case 'U':
    return KEY_U;
  case 'V':
    return KEY_V;
  case 'W':
    return KEY_W;
  case 'X':
    return KEY_X;
  case 'Y':
    return KEY_Y;
  case 'Z':
    return KEY_Z;
  // Numbers
  case '0':
    return KEY_0;
  case '1':
    return KEY_1;
  case '2':
    return KEY_2;
  case '3':
    return KEY_3;
  case '4':
    return KEY_4;
  case '5':
    return KEY_5;
  case '6':
    return KEY_6;
  case '7':
    return KEY_7;
  case '8':
    return KEY_8;
  case '9':
    return KEY_9;
  // Punctuation & special
  case ' ':
    return KEY_SPACE;
  case '\r':
  case '\n':
    return KEY_RETURN;
  case '\t':
    return KEY_TAB;
  case '[':
    return KEY_LEFT_BRACKET;
  case ']':
    return KEY_RIGHT_BRACKET;
  case '-':
    return KEY_MINUS;
  case '=':
    return KEY_EQUALS;
  case '\\':
    return KEY_BACK_SLASH;
  case ';':
    return KEY_SEMICOLON;
  case '\'':
    return KEY_QUOTE;
  case ',':
    return KEY_COMMA;
  case '.':
    return KEY_DOT;
  case '/':
    return KEY_SLASH;
  case '`':
    return KEY_BACKQUOTE;
  // Fcitx provides these as named keysyms
  case 0xff08:
    return KEY_DELETE; // XK_BackSpace
  case 0xff1b:
    return KEY_ESC; // XK_Escape
  case 0xff0d:
    return KEY_RETURN; // XK_Return
  case 0xff09:
    return KEY_TAB; // XK_Tab
  case 0xff51:
    return KEY_LEFT; // XK_Left
  case 0xff53:
    return KEY_RIGHT; // XK_Right
  case 0xff52:
    return KEY_UP; // XK_Up
  case 0xff54:
    return KEY_DOWN; // XK_Down
  default:
    return KEY_EMPTY;
  }
}

// Convert a Unicode codepoint to a UTF-8 std::string
static std::string unicodeToUtf8(uint32_t cp) {
  std::string result;
  if (cp < 0x80) {
    result += static_cast<char>(cp);
  } else if (cp < 0x800) {
    result += static_cast<char>(0xC0 | (cp >> 6));
    result += static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    result += static_cast<char>(0xE0 | (cp >> 12));
    result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    result += static_cast<char>(0x80 | (cp & 0x3F));
  } else {
    result += static_cast<char>(0xF0 | (cp >> 18));
    result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    result += static_cast<char>(0x80 | (cp & 0x3F));
  }
  return result;
}

// ============================================================
// Fcitx5 OpenKey Engine Implementation
// ============================================================
namespace openkey {

class OpenKeyEngine : public fcitx::InputMethodEngineV2 {
public:
  OpenKeyEngine(fcitx::Instance *instance) : instance_(instance) {
    loadConfig();
    hookState_ = static_cast<vKeyHookState *>(vKeyInit());
  }

  void loadConfig() {
    QSettings settings("OpenKeyLinux", "Config");
    vInputType = settings.value("inputType", 0).toInt();
    vCodeTable = settings.value("codeTable", 0).toInt();
    vFreeMark = settings.value("freeMark", true).toBool() ? 1 : 0;
    vUseModernOrthography = settings.value("modernOrth", true).toBool() ? 1 : 0;
    vCheckSpelling = settings.value("checkSpelling", true).toBool() ? 1 : 0;
    vUpperCaseFirstChar =
        settings.value("upperCaseFirst", false).toBool() ? 1 : 0;

    vUseMacro = settings.value("useMacro", false).toBool() ? 1 : 0;
    vUseMacroInEnglishMode =
        settings.value("useMacroEnglish", false).toBool() ? 1 : 0;
    vAutoCapsMacro = settings.value("autoCapsMacro", true).toBool() ? 1 : 0;
    vQuickTelex = settings.value("quickTelex", false).toBool() ? 1 : 0;
    vQuickStartConsonant = settings.value("quickStart", false).toBool() ? 1 : 0;
    vQuickEndConsonant = settings.value("quickEnd", false).toBool() ? 1 : 0;

    vFixRecommendBrowser = settings.value("fixBrowser", true).toBool() ? 1 : 0;
    vRememberCode = settings.value("runAtStartup", true).toBool() ? 1 : 0;
  }

  void keyEvent(const fcitx::InputMethodEntry &entry,
                fcitx::KeyEvent &keyEvent) override {
    // Only process key-down events, ignore key-up and release
    if (keyEvent.isRelease()) {
      return;
    }

    auto key = keyEvent.key();
    auto *ic = keyEvent.inputContext();

    // Let modifier-only keys pass through (Ctrl+X, Alt+X, etc.)
    if (key.states().test(fcitx::KeyState::Ctrl) ||
        key.states().test(fcitx::KeyState::Alt) ||
        key.states().test(fcitx::KeyState::Super)) {
      return;
    }

    // Map Fcitx5 keysym to OpenKey X11 keycode
    uint32_t sym = static_cast<uint32_t>(key.sym());
    Uint16 openKeyCode = fcitxKeySymToOpenKey(sym);

    // If we can't map the key, pass it through
    if (openKeyCode == KEY_EMPTY) {
      return;
    }

    // Detect caps/shift state
    bool isShift = key.states().test(fcitx::KeyState::Shift);
    bool isCaps = (sym >= 'A' && sym <= 'Z' && !isShift) ||
                  (sym >= 'a' && sym <= 'z' && isShift);
    Uint8 capsStatus = (isShift || isCaps) ? 1 : 0;

    // Word-break keys: commit current state and start new session
    if (openKeyCode == KEY_SPACE || openKeyCode == KEY_RETURN ||
        openKeyCode == KEY_TAB || openKeyCode == KEY_ESC) {
      startNewSession();
      return; // Let Fcitx5 handle the key normally
    }

    // Feed the key to OpenKey Engine
    vKeyHandleEvent(vKeyEvent::Keyboard, vKeyEventState::KeyDown, openKeyCode,
                    capsStatus, false);

    // Read the engine's response
    if (hookState_->code == vDoNothing) {
      // Engine didn't modify anything — commit the raw character
      if (sym >= 0x20 && sym < 0x7F) {
        std::string ch(1, static_cast<char>(sym));
        ic->commitString(ch);
        keyEvent.filterAndAccept();
      }
      return;
    }

    if (hookState_->code == vWillProcess || hookState_->code == vRestore) {
      // Engine wants to replace text:
      // 1. Send backspaces to delete previous chars
      // 2. Commit new chars from charData[]

      // Step 1: Delete previous characters by sending backspace events
      for (int i = 0; i < hookState_->backspaceCount; i++) {
        ic->deleteSurroundingText(-1, 1);
      }

      // Step 2: Build the replacement string from charData[]
      std::string commitStr;
      for (int i = 0; i < hookState_->newCharCount; i++) {
        Uint32 charCode = getCharacterCode(hookState_->charData[i]);
        // Mask out internal engine flags (CHAR_CODE_MASK, etc.) before UTF-8
        // conversion
        commitStr += unicodeToUtf8(charCode & CHAR_MASK);
      }

      if (!commitStr.empty()) {
        ic->commitString(commitStr);
      }

      keyEvent.filterAndAccept();
      return;
    }

    if (hookState_->code == vBreakWord) {
      startNewSession();
      // Re-process the key in the new session
      vKeyHandleEvent(vKeyEvent::Keyboard, vKeyEventState::KeyDown, openKeyCode,
                      capsStatus, false);
      if (hookState_->code == vDoNothing && sym >= 0x20 && sym < 0x7F) {
        std::string ch(1, static_cast<char>(sym));
        ic->commitString(ch);
        keyEvent.filterAndAccept();
      }
      return;
    }

    if (hookState_->code == vReplaceMaro) {
      // Macro replacement
      for (int i = 0; i < hookState_->backspaceCount; i++) {
        ic->deleteSurroundingText(-1, 1);
      }
      std::string macroStr;
      for (size_t i = 0; i < hookState_->macroData.size(); i++) {
        Uint32 charCode = getCharacterCode(hookState_->macroData[i]);
        macroStr += unicodeToUtf8(charCode & CHAR_MASK);
      }
      if (!macroStr.empty()) {
        ic->commitString(macroStr);
      }
      startNewSession();
      keyEvent.filterAndAccept();
      return;
    }
  }

  void activate(const fcitx::InputMethodEntry &entry,
                fcitx::InputContextEvent &event) override {
    startNewSession();
  }

  void deactivate(const fcitx::InputMethodEntry &entry,
                  fcitx::InputContextEvent &event) override {
    startNewSession();
  }

  void reset(const fcitx::InputMethodEntry &entry,
             fcitx::InputContextEvent &event) override {
    startNewSession();
  }

  void save() override {}

private:
  fcitx::Instance *instance_;
  vKeyHookState *hookState_ = nullptr;
};

class OpenKeyFactory : public fcitx::AddonFactory {
public:
  fcitx::AddonInstance *create(fcitx::AddonManager *manager) override {
    return new OpenKeyEngine(manager->instance());
  }
};

} // namespace openkey

FCITX_ADDON_FACTORY(openkey::OpenKeyFactory);
