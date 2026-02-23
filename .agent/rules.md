# OpenKey Linux Rules & Governance

**ID**: OPENKEY-LINUX-RULES
**Status**: ACTIVE
**Target**: All Agents and Maintainers

This document defines the strict governance rules for the OpenKey Linux repository (`tamld/OpenKey`). It acts as a living document to ensure no drift occurs from the 2026 Vision Architecture.

---

## 1. The "Design Inheritance" Mandate
- **Rule**: The Linux GUI must strictly remain a 1:1 pixel-perfect port of the original macOS/Windows Preferences application.
- **Forbidden**: Do not introduce web frameworks (Tauri, Electron) or deviate from the established layout structure.
- **Tech Stack**: Must use C++ Native UI (Qt5/Qt6 or GTK) via CMake.

## 2. The "Wayland First" Mandate
- **Rule**: All input method intercept logic must be optimized for Wayland via Fcitx5.
- **Forbidden**: Do not write raw X11 (XLib/XCB) hooks. If X11 support is needed, rely strictly on Fcitx5's internal fallback mechanisms.
- **Performance**: Keystroke processing latency from Fcitx5 -> Engine -> Fcitx5 must remain under 5ms.

## 3. The "Zero Configuration" Mandate
- **Rule**: The installation process must provide a "Seamless Out-of-the-Box Experience."
- **Enforcement**: Packages (`.deb`, `.rpm`) must include `postinst` hooks to automatically register the IME and reload the display daemon without requiring user terminal commands or a system reboot.

## 4. Naming Conventions
- **Format**: All newly created artifacts, scripts, and binaries must use `kebab-case`.
- **Examples**: `openkey-cli`, `openkey-linux-ui`, `fcitx5-openkey`.

## 5. 3-Tier Testing Protocol
No code transitions from `develop` to `master` without passing:
1. **Tier 1 (Unit)**: C++ GTest execution.
2. **Tier 2 (Smoke)**: Dockerized multi-distro installation (`ubuntu`, `fedora`, `archlinux`).
3. **Tier 3 (E2E)**: UI Xvfb graphical smoke test.

## Executable Verification
```javascript
WARN_IF_MATCHES: /ibus|electron|tauri|x11_hook/i
```
