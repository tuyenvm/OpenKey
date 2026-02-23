---
name: openkey-linux-dev
description: Antigravity skill for OpenKey Linux development, enforcing the 2026 Vision architecture (Native UI + Fcitx5 + C++).
version: 1.0.0
type: execution
role: The Linux Maintainer
---

# SKILL: OpenKey Linux Development

**ID**: SKILL-OPENKEY-LINUX-2026

## 1. Mission
This skill provides context and operational procedures for developing the Linux branch of the OpenKey Vietnamese IME. It strictly enforces the mandates laid out in `.agent/rules.md`.

## 2. Context & Boundaries
- **Language Stack**: Strictly C++ (Engine, Fcitx5 Wrapper, Qt/GTK UI).
- **Architecture**: Wayland/Fcitx5 Native. No X11 hooks.
- **Design Philosophy**: 1:1 pixel-perfect port of the macOS/Windows Preferences UI. No design drift. No Electron/Tauri.

## 3. Standard Operating Procedures (SOPs)

### SOP: Building the Engine & CLI
```bash
# Always use an out-of-source build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
# Verify Engine Health
./openkey-cli --test
```

### SOP: Building the Fcitx5 Plugin
```bash
# Requires Fcitx5 development headers
sudo apt install fcitx5 libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev fcitx5-modules-dev
cd build
cmake ..
make -j$(nproc)
# The output is a shared object (.so)
ls -la build/Sources/OpenKey/linux/fcitx5/fcitx5-openkey.so
```

### SOP: CI/CD Multi-Distro Testing
Before submitting a PR, ensure the code compiles on Ubuntu, Fedora, and Arch Linux as defined in `.github/workflows/linux-release.yml`. Use Docker for local verification if needed.

## 4. Forbidden Action Patterns
- **Do not** introduce web-based UI frameworks.
- **Do not** modify `Engine.cpp` or `Vietnamese.cpp` unless resolving a cross-platform compilation bug (e.g., GCC 15 `#include <algorithm>`). The Engine logic is sacred.
- **Do not** name any file or binary using `camelCase` or `snake_case`. Enforce `kebab-case` strictly.

## 5. Artifact Review
Always reference `.agent/PRD.md`, `.agent/DoD.md`, and `.agent/ADR-001-Linux-Architecture.md` before making any structural changes to the `Sources/OpenKey/linux` directory.
