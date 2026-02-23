# ADR-001: OpenKey Linux Modernization Architecture (2026 Vision)

## Status
ACCEPTED (UPDATED: Native UI Strategy)

## Context
The user requested a strategic approach to open-source contribution for OpenKey on Linux. Following an initial proposal involving a web-based (Tauri) frontend, the user specified a critical design constraint: **The Linux GUI must inherit and respect the original design language of the macOS/Windows versions to prevent design drift.** The architecture must be faithfully ported.

## Decision
We architect the OpenKey Linux experience across three distinct pillars using a unified tech stack:
1. **Engine Layer**: Utilize the existing OpenKey C++ Engine.
2. **System Layer (Input Method)**: Integrate natively with **Fcitx5** using C++.
3. **Control Layer (GUI)**: Utilize a **Native C++ UI Framework (Qt5/Qt6 or GTK)** to perfectly port the macOS UI layout.

## Consequences

### Positive
- **Visual Fidelity**: By using a mature desktop framework like Qt, we can precisely replicate the tabbed layout, specific toggles, and system tray behaviors of the original macOS application.
- **Unified Stack**: The entire repository remains strictly C/C++, eliminating the need to introduce Rust and Node.js toolchains (which Tauri would have required).
- **Fcitx5 Synergy**: Fcitx5 itself is heavily integrated with Qt (especially in KDE). A Qt-based GUI will have first-class support for communicating with Fcitx5 modules.

### Negative
- **UI Development Speed**: C++ Qt/GTK layouts take slightly more time to construct programmatically compared to HTML/TailwindCSS.

### Risks
- *Risk 1: Qt/GTK version fragmentation on Linux.* -> **Mitigation**: Statically link UI libraries where necessary, or rely on ubiquitous base versions (e.g., Qt5) that are standard across Ubuntu 20.04+.

## Alternatives Considered
| Option | Pros | Cons | Verdict |
|--------|------|------|---------|
| Fcitx5 + Tauri (Rust/React) | Fast UI development; modern look. | Violates the mandate to natively port the original design language. Introduces fragmented tech stack. | REJECTED (Overridden by User) |
| Fcitx5 + Native Qt/GTK | Perfect aesthetic port of macOS/Windows. Keeps codebase 100% C++. | Slightly steeper UI development curve. | CHOSEN |

## References
- User Mandate: Inherit original design, avoid aesthetic drift, respect founder's vision.
