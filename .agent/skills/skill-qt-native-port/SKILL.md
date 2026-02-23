---
name: qt-native-port
description: Antigravity skill specialized in porting macOS/Windows layouts to native C++ Qt5/Qt6 without design drift.
version: 1.0.0
type: execution
role: The Visual Custodian
---

# SKILL: Qt Native UI Porting

**ID**: SKILL-QT-NATIVE-PORT-2026

## 1. Mission
This skill ensures that the Linux Control Center for OpenKey remains a 1:1 pixel-perfect replica of the original macOS and Windows Preferences applications. It rejects web-based wrappers in favor of high-performance Native C++ (Qt).

## 2. Boundaries & Constraints
- **Framework**: Qt5 or Qt6 (Standardized in `CMakeLists.txt`).
- **Design Philosophy**: No Design Drift. The Layout, Tabs, Toggles, and system tray icons must perfectly map to the upstream project's look and feel.

## 3. Standard Operating Procedures (SOPs)

### SOP: Layout Translation
When porting a UI layout:
1. Examine the upstream macOS `.xib` or Windows `.ui` file structures.
2. Programmatically reconstruct the grid/flex layouts using `QVBoxLayout` and `QHBoxLayout` in Qt.
3. Match font sizes, border radii, and padding exactly.

### SOP: Fcitx5 Inter-Process Communication (IPC)
The Qt UI must save configuration changes to a shared `openkey.ini` file.
After saving, the UI must issue a reload signal to Fcitx5:
```bash
fcitx5-remote -r
```

## 4. Forbidden Action Patterns
- **Do not** use Electron, Tauri, GTK3, or raw X11/Wayland Windows.
- **Do not** introduce new UI features that do not exist on the macOS/Windows versions unless explicitly authorized as a Linux-only necessity.
