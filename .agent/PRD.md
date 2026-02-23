# PRD: OpenKey Linux Release & 2026 Vision

## Vision
Transform OpenKey into the definitive open-source Vietnamese Input Method Engine (IME) for modern Linux systems. We will build upon the core C++ engine to provide a seamless, zero-latency typing experience integrated with Fcitx5. Crucially, the Graphical User Interface (GUI) will be a 1:1 native port of the original macOS/Windows OpenKey application, preserving the founder's aesthetic and ensuring cross-platform design consistency.

## Problem Statement
Historically, Linux Vietnamese IMEs have suffered from stagnation and disjointed user interfaces compared to their macOS/Windows counterparts. OpenKey possesses a battle-tested C++ engine. The Linux community needs a modern, lightning-fast alternative that looks, feels, and configures exactly like the OpenKey they are familiar with on Mac and Windows.

## Target Users
- Software Engineers and Power Users on Linux (Ubuntu, Arch, Fedora).
- Everyday users migrating to Linux who expect the exact same macOS/Windows OpenKey experience.

## Scope

### Phase 1: Core Foundation (COMPLETED)
- C++ Engine compilation via CMake.
- CLI verification tool (`openkey-cli`).
- Automated CI/CD with `.deb` packaging.

### Phase 2: IME Framework Integration (Fcitx5)
- Create `fcitx5-openkey` wrapper plugin.
- Direct C++ integration for zero-latency keystroke processing.
- Wayland native support via Fcitx5 `text-input-v3`.

### Phase 3: Control Center UI (Native Port)
- 1:1 Graphical User Interface adaptation of the original macOS/Windows OpenKey Preferences.
- **Tech Stack**: Native Linux UI Framework (C++ Qt5/Qt6 or GTK) to match the original C++/Objective-C codebase spirit.
- Ensures zero "design language drift" from the upstream project.

### Phase 4: Widespread Distribution & UX
- **Seamless Out-of-the-Box Experience**: Zero-configuration required. The user double-clicks the `.deb`, and the software is ready to use immediately without manual terminal configuration.
- **Post-Install Hooks**: Automated scripts within packages to register the IME with the OS and trigger an Fcitx5 daemon reload automatically.
- **AUR**: Arch User Repository deployment.
- **Native Packages**: `.deb` and `.rpm` distribution ensuring deeply integrated system tray icons and UI components.

## Success Metrics
1. **Performance**: Keystroke latency < 5ms.
2. **Aesthetic Parity**: The Linux Preferences window must be indistinguishable in layout and functionality from the macOS version.
3. **Compatibility**: Flawlessly works across Wayland compositors.
4. **Reliability**: Pipeline passes 3-Tier Testing (Unit Tests, Environment Smoke Tests, and UI Automation Tests).

## Risks & Mitigations
| Risk | Mitigation |
|------|------------|
| Translating macOS XIB/Storyboard to Qt/GTK | Manually map the layout grid to Qt Designer / Glade while reusing exact iconography and spacing. |
| Owner does not merge upstream PRs | Maintain `tamld/OpenKey` as a faithfully ported, high-quality Linux branch. |
