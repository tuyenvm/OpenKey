# Roadmap: OpenKey Linux (2026 Vision)

## Timeline Overview

```
Phase 0: SSoT Infrastructure         ████████████ DONE
Phase 1: Build & CLI Foundation      ████████████ DONE
Phase 2: CI/CD Pipeline & Packaging  ████████████ DONE
Phase 3: Fcitx5 Engine Integration   ░░░░░░░░░░░░ PLANNED
Phase 4: Native Control Center UI    ░░░░░░░░░░░░ PLANNED
Phase 5: Universal Distribution      ░░░░░░░░░░░░ PLANNED
```

---

## Phase 0-2: The Foundation ✅ (Completed v2.0.0-linux-beta)
- [x] Agent `.agent/` SSoT & documentation standard.
- [x] Engine compilation on Linux via CMake & GCC 15 fixes.
- [x] `openkey-cli` for isolated behavior verification.
- [x] GitHub Actions automated Linux builds.
- [x] SHA256 verification and CPack `.deb` generation.

## Phase 3: Fcitx5 Engine Integration 🚀
**Goal**: Connect the OpenKey Engine to the Linux display server natively via Fcitx5.
- [ ] Research Fcitx5 C++ Addon development API.
- [ ] Create `fcitx5-openkey` submodule.
- [ ] Implement keystroke interception bridging to OpenKey Engine.
- [ ] Deliver a functional, headless typing experience across applications.

## Phase 4: Native Control Center UI 🎨
**Goal**: Port the original OpenKey interface to Linux to prevent design drift.
- [ ] Analyze the macOS Settings UI (Layout, Tabs, Toggles, Hotkeys).
- [ ] Setup C++ Native UI framework (Qt5/Qt6 or GTK) within CMake.
- [ ] Replicate the visual UI layout 1:1.
- [ ] Implement inter-process communication (IPC) or shared configuration files to sync UI settings to the Fcitx5 daemon.
- [ ] Ensure system tray integration matches the original app behavior.

## Phase 5: Universal Distribution 🌍
**Goal**: One-click install for every Linux user.
- [ ] Publish `openkey-linux` to the Arch User Repository (AUR).
- [ ] Support widespread `.deb` and `.rpm` deployment.

---

## Decision Log (Architectural Updates)

| # | Action / Decision | Rationale |
|---|-------------------|-----------|
| 1 | C++ & CMake | Standardization across the overarching OpenKey cross-platform project. |
| 2 | Fcitx5 over IBus | Fcitx5 uses modern C++, integrates perfectly with Wayland, and has minimal latency. |
| 3 | Native C++ UI Port | The user mandated avoiding design drift. Porting the original UI natively in Qt/GTK (instead of Tauri/Web) respects the founder's aesthetic and maintains a pure C++ stack mirroring Windows. |
