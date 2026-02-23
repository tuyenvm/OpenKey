# Definition of Done: OpenKey Linux (2026 Vision)

## Phase 0-2: Foundation (✅ COMPLETED)

| # | Criteria | Evidence/Method | Status |
|---|----------|-----------------|--------|
| 1 | CMake compiles Engine successfully | `-- Build files have been written` | ✅ |
| 2 | Binary executable generated | `build/openkey-cli` ELF 64-bit | ✅ |
| 3 | Engine logic passes self-tests | `./openkey-cli --test` output | ✅ |
| 4 | GitHub Actions workflow passes | Green checkmark on `ubuntu-latest` | ✅ |
| 5 | `.deb` package automatically generated | Validated via `dpkg -c` | ✅ |
| 6 | PR is 100% clean and English standard | PR #318 reviewed | ✅ |

---

## Phase 3: Fcitx5 Engine Integration (▶ CURRENT TARGET)

| # | Criteria | Evidence/Method | Status |
|---|----------|-----------------|--------|
| 7 | Fcitx5 module compiles | `.so` shared object generated | ⬜ |
| 8 | Module loads into Fcitx5 daemon | Verified in `fcitx5-configtool` | ⬜ |
| 9 | Typing Vietnamese text in Gedit | Visual validation (Telex processing) | ⬜ |
| 10 | Typing Vietnamese text in Wayland apps | Visual validation in native Wayland clients | ⬜ |

---

## Phase 4: Native Control Center UI (GUI Port)

| # | Criteria | Evidence/Method | Status |
|---|----------|-----------------|--------|
| 11 | Qt/GTK App builds via CMake | `make -j` success | ⬜ |
| 12 | App layout mimics macOS perfectly | Side-by-side screenshot validation with macOS repo | ⬜ |
| 13 | App updates Fcitx5 configuration successfully | Changes in UI immediately reflect in engine state | ⬜ |
| 14 | Cross-platform code isolation | Ensure `Sources/OpenKey/linux/ui` holds pure Linux/Qt definitions | ⬜ |

---

## Quality Gates & Rules
- ⬜ = Pending / Not Started
- ✅ = Passed (Evidence required)
- ❌ = Failed
> **Strict Gate Constraint**: No transition to the next Phase is permitted until the current Phase has achieved a 100% pass rate.
