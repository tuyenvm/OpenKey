---
name: fcitx5-cpp-integration
description: Antigravity skill specialized in integrating C++ engines with the Fcitx5 Wayland Input Method Framework.
version: 1.0.0
type: execution
role: The Fcitx5 Specialist
---

# SKILL: Fcitx5 C++ Integration

**ID**: SKILL-FCITX5-CPP-2026

## 1. Mission
This skill provides the domain expertise required to build a C++ input method module for `fcitx5`. It bridges raw keystrokes from the Wayland/X11 display server into the core C++ engine.

## 2. Boundaries & Constraints
- **Language**: Strictly Modern C++ (C++17).
- **Libraries**: `Fcitx5Core`, `Fcitx5Module`.
- **Latency Target**: Input processing must remain under 5ms.

## 3. Standard Operating Procedures (SOPs)

### SOP: Compiling the Module
```bash
# Verify headers are present
dpkg -l | grep fcitx5-dev
# Compile
mkdir -p build && cd build
cmake .. && make
```

### SOP: Fcitx5 Input Context Hooking
When implementing `fcitx::InputMethodEngineV2`:
1. Intercept `keyEvent` before it reaches the application.
2. If the user typing requires Vietnamese (Telex/VNI) processing, capture the input and return `keyEvent.filterAndAccept()`.
3. Commit the processed string via `inputContext->commitString(...)`.

## 4. Forbidden Action Patterns
- **Do not** use D-Bus for keystroke transmission. Fcitx5 provides native memory-level C++ bindings which are infinitely faster.
- **Do not** modify the `fcitx5` daemon itself; only compile `.so` addons.
