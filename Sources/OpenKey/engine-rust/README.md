# Rust engine — macOS review checkpoint

The macOS app now links this dependency-free Rust static library instead of the five legacy C++ engine sources. AppKit owns the menu bar, settings window and keyboard event tap; SwiftUI renders grouped settings forms on macOS 13 and later. `RustBridge.cpp` adapts the existing Objective-C++ callers; composition, spelling, macros, encoding conversion and remembered app data run in Rust. Windows and Linux source/build files are unchanged.

## Build and verify

Install Rust through rustup. The repository pins Rust 1.100.0 and both macOS architecture targets in `rust-toolchain.toml`.

From the repository root:

```sh
cargo test --locked --manifest-path Sources/OpenKey/engine-rust/Cargo.toml
cargo clippy --locked --manifest-path Sources/OpenKey/engine-rust/Cargo.toml --all-targets -- -D warnings
python3 scripts/check-rust-engine.py
xcodebuild -project Sources/OpenKey/macOS/OpenKey.xcodeproj -scheme OpenKey \
  -configuration Release -derivedDataPath /tmp/openkey-rust-build \
  CODE_SIGNING_ALLOWED=NO MACOSX_DEPLOYMENT_TARGET=12.0 build
```

Xcode builds each requested architecture with Cargo, combines the libraries with `lipo`, and links Rust statically. There is no Rust runtime to install on users’ Macs. The existing documented Mojave minimum is now reflected by the target’s deployment setting (10.14); the verification build above targets macOS 12 and later. Older systems have not been runtime-tested.

## Behavior and checks

- 7,305 independent legacy C++ traces compare the visible text after **every** key: four input modes, five encodings, both accent-placement modes, backspace, boundaries, restoration, quick typing, capitalization and macros.
- The same corpus exercises the real C++/Rust bridge. `scripts/capture-engine-fixtures.py` regenerates fixtures from the unchanged C++ engine.
- Native Rust tests also cover 500,000 deterministic random editing events, C ABI layout and invalid inputs, transactional parsing of truncated macro/app data, UTF-16 macro content and encoding round-trips.
- Legacy macro/app binary formats are retained. Oversized entries and malformed UTF-8 are rejected before replacing loaded data.
- Conversion intentionally preserves case unless a case option is selected, keeps supplementary Unicode characters intact, and omits the legacy converter’s trailing NUL. These are covered explicitly instead of copying its lowercasing bug.
- Quick consonant expansion is skipped when its output would exceed the 32-character native result buffer. Editing history retains the most recent 256 chunks.

The C ABI owns opaque engine handles and explicitly freed byte buffers. Each handle must be used serially; the existing macOS event loop satisfies this. No Rust references, STL containers or allocator-owned strings cross the ABI. Panics in processing/commands are caught before returning to native callers.

## Regenerate tables

```sh
clang++ -std=c++17 scripts/export-rust-tables.cpp Sources/OpenKey/engine/Vietnamese.cpp -o /tmp/openkey-tables
/tmp/openkey-tables > Sources/OpenKey/engine-rust/src/tables.rs
cargo fmt --manifest-path Sources/OpenKey/engine-rust/Cargo.toml
```

The data and algorithms derive from OpenKey and retain GPL-3.0 licensing and attribution.

## Review checkpoint

The draft includes the Rust engine, milestone 3 reliability work and milestone 4 menu-bar/settings simplification. Milestone 5 has not started. Updater repair, login-item API migration, signing/notarization and release verification remain deferred. Automated native checks do not establish physical typing compatibility.

## Milestone 3 reliability checkpoint

`sh scripts/check-macos-input.sh` tests the actual native callback and manager with event posting intercepted and a local Mach port (no global keyboard input):

- Timeout notification dispatch and automatic recovery; explicit user/system disable stays paused.
- Invalid port recreation without reinitializing the engine; permission revocation and stop release the tap/timer.
- Secure input prevents recovery and bypasses composition.
- Replacement events preserve the original target PID and serial number.
- An external private event source composes VNI `a1` into `á`, while OpenKey’s own private-source events are ignored.
- Spotlight selection replacement requires the event target to match Spotlight, preventing a fading window from changing another app’s editing strategy.
- Terminal apps bypass the empty-character browser workaround; empty encoding-length tracking never dereferences `vector::back()`.
- Rust field resets discard word, macro and editing history while preserving stored macros and application settings.

The tap retains the established session location. Context/source changes refresh cached app and keyboard-language data outside the keystroke callback, and the copied keyboard source is released without releasing its borrowed properties.

Live testing on macOS 27.0: the app launches after reapproving the local build and its event tap is present and enabled. Computer-use `pressKey`/`typeText` produced literal VNI `a1`/`vie65t` and Telex `as` in TextEdit; the cause has not been established, so automated live composition is **not marked passed**. The user confirmed physical VNI `a1` produces `á`. macOS also has ABC and VNI input sources installed; the active source was not established, so that observation alone does not isolate Rust engine behavior.

Additional milestone 3 computer checks:

- **Passed, hook lifecycle only:** after user-confirmed sleep/wake, the same OpenKey process (PID 2039) had a new enabled tap (2143124030 before, 1988714904 after). No app restart was needed. Injected `a1` remained literal after wake; composition recovery is still unverified.
- **Passed, native editing only:** pasted `tiếng Việt` supported backspace and cursor insertion/deletion, restoring the original text. TextEdit search-field input stayed in the field and left the document unchanged. These checks do not verify Rust composition across field boundaries.
- **Inconclusive:** Spotlight did not visibly open through computer-use Command-Space, even after temporarily enabling its disabled shortcut with user approval. The shortcut was restored to disabled and verified. Spotlight dismissal/typing routing remains unverified.
- **Blocked:** the computer-use tool denied Terminal access; no alternate route was used. Remote-desktop input remains untested.

VNI, Unicode, Option-Z, smart switching and browser-workaround settings were restored. The live-authorized app was `/Users/hai/Downloads/OpenKey-milestone-3/OpenKey.app`; it predates the final explicit-pause-on-wake adjustment. That adjustment has native lifecycle coverage, but the final review artifact still needs a live run. Those results remain a draft checkpoint, not a merge-ready live-validation claim.


## Milestone 4: native menu-bar app

The menu-bar app has four native toolbar settings tabs: **Chung**, **Gõ tiếng Việt**, **Ứng dụng** and **Tương thích**. AppKit owns the window/toolbar; SwiftUI grouped forms provide content on macOS 13+, with an AppKit fallback on older systems. There is no sidebar/search or Advanced Settings route.

- General contains login, the accessible language-shortcut recorder and its sound option. Recording preserves the saved sound flag and ends on focus loss, close or permission revocation.
- Typing exposes input method, encoding, spelling/restoration, extra consonants, accent placement, sentence capitalization, quick consonants and temporary Control/Command bypass behavior.
- Applications is a full tab containing language/encoding remembering and fixed/remembered app rules. Fixed rules constrain shortcut/menu language changes. App pickers attach to the settings window.
- Compatibility exposes browser suggestions, Chromium, separate key delivery, keyboard layout and bypass with another macOS input method. Existing saved values are preserved.
- Text Replacements has no UI or route in OpenKey: no link, editor, import or export. OpenKey macro expansion remains disabled; old macro data is retained without modification. macOS owns text replacement and Fn/Globe behavior.
- Missing Accessibility permission gates settings entirely. Approval connects input and dismisses setup; revocation restores the gate. A contextual menu command handles paused/failed connections.
- Language changes live in the status menu and shortcut. Conversion remains an optional sheet; About is native. New/reset installs leave login/remembering off; existing preferences remain.
- Remembered-language saves resolve the current app and its rule together, including explicit remembered rules with global remembering off. Settings activation does not implicitly overwrite the typing app's choice.

### Validation and review limits

`sh scripts/check-macos-input.sh` covers the native callback, lifecycle, actual AppDelegate menu actions and isolated settings. The updated settings check exercises four-tab navigation, restored typing/compatibility preferences, encoding, preserved shortcut sound, app-rule persistence, the permission gate and absence of Text Replacements entry points.

The previous two-pane artifact was computer-tested: permission gate/approval, both panes, VNI → Telex → VNI, spelling dependencies, shortcut recording, app-rule add/change/remove, native About, Command-Comma and utility sheet layouts. It restarted with an enabled keyboard tap. Tested preferences, macro bytes and remembered data were restored. The native macOS replacement import/expansion test passed and its disposable entry was removed, preserving the original 10 replacements; this integration is now removed from OpenKey.

The four-tab revision is a separate artifact at `/Users/hai/Downloads/OpenKey-milestone-4-tabs/OpenKey.app` with matching zip. It is built for arm64/x86_64, locally ad-hoc signed and not notarized. It has not been live-authorized or computer-tested; the previous artifact remains installed/running for comparison. Older macOS fallback remains untested at runtime.

The complete physical remembered-language shortcut/app-switch/typing regression remains unverified. Earlier physical fixed-English/fixed-Vietnamese results apply to an older artifact; injected computer input bypasses the OpenKey tap and is not a physical pass. Earlier sleep/wake established tap recreation only. Live Spotlight composition, secure input and remote desktop remain unverified; computer control denied Terminal access.

Stop for review before milestone 5: updater repair, login-item API migration, signing/notarization and release verification have not started.
