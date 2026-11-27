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


## Milestone 4: smaller native menu-bar app

The current interface replaces the earlier sidebar and language selector with a small menu-bar menu and two native toolbar settings panes: **Chung** and **Gõ tiếng Việt**. AppKit owns the window and toolbar; SwiftUI `Form`/`Section`/`Toggle`/`Picker` provide grouped content on macOS 13+, with an AppKit fallback on older systems. There is no Advanced Settings route or disconnected layout preview.

- Language changes live in the menu bar and existing shortcut. The menu reflects fixed app rules, permission and tap availability. Settings no longer expose a language segment whose foreground application differs from the typing application.
- General contains launch-at-login, one shortcut recorder, remembering per app, optional application-rule sheet and a link directly to macOS Text Replacements. Typing contains input method, spelling, accent placement and quick consonants. Existing saved preferences remain; new/reset installs leave remembering and launch-at-login off.
- Missing Accessibility permission gates the whole settings window. Approval connects the tap and dismisses setup; revocation stops interception and restores the gate. Connection failures/intentional pauses have a contextual menu action. No separate permission page, typing field or OS-keyboard shortcut page remains.
- macOS owns text replacement and Fn/Globe behavior. OpenKey macro expansion is disabled at runtime, while existing macro preferences/data remain intact. Existing records can be exported as Apple's importable shortcut/phrase plist; there is no macro editor or private OS preference write.
- Application rules and clipboard encoding conversion remain optional sheets. About uses the native panel. Legacy source/storyboard definitions remain for build compatibility but have no active legacy-window route.
- Remembered-language saves resolve the current app and its rule together, including when global remembering is off. Settings activation does not implicitly replace the typing app's remembered choice. Native regressions cover remembered/fixed transitions and the actual AppDelegate language menu actions.
- Shortcut recording temporarily bypasses the keyboard tap; Escape cancels, Delete clears, and leaving/closing the window or losing permission ends recording. The same native field is exposed as an accessible button.

### Validation and review limits

`sh scripts/check-macos-input.sh` passes native callback, lifecycle, settings and real AppDelegate menu checks. Coverage includes permission approval/revocation, explicit pause, app rules, shortcut recording, both settings panes, native replacement export, utility sheets and spelling dependencies. The Release build contains arm64 and x86_64, targeting macOS 12 for verification. Older-system runtime behavior remains untested.

Computer use verified the real permission gate and the first smaller build's two panes. It opened macOS Text Replacements through the app, imported a disposable plist and observed its shortcut expand to “Kiểm tra thay thế văn bản” in TextEdit. The temporary entry was removed; all 10 original native replacements remained. The disposable TextEdit document was restored. The final build was authorized and computer-tested: both panes, VNI → Telex → VNI, spelling-dependent controls, Option-X recording followed by restored Option-Z, application-rule add/change/remove through a nested native picker, full application/conversion sheet layouts, native About and Command-Comma all passed. The final nested picker/alert parent correction is included. The app restarted successfully with an enabled keyboard tap. Original tested preferences, macro bytes and remembered-app bytes were preserved; only pane/file-picker UI state changed.

The earlier user-confirmed physical fixed-English/fixed-Vietnamese checks apply to the previous artifact. The complete physical remembered-language shortcut/app-switch/typing sequence remains unverified; injected input bypasses the OpenKey tap and is not a physical pass. Earlier sleep/wake proved tap recreation only. Spotlight composition, secure-input behavior and remote-desktop input remain unverified live; computer control denied Terminal access.

Review artifact: `/Users/hai/Downloads/OpenKey-milestone-4-small/OpenKey.app` (local ad-hoc signature, not notarized). Stop for user review before milestone 5.

Apple references: [Settings](https://developer.apple.com/design/human-interface-guidelines/settings), [Text Replacements](https://support.apple.com/guide/mac-help/replace-text-punctuation-documents-mac-mh35735/mac), [import/export replacements](https://support.apple.com/en-gb/guide/mac-help/mchl2a7bd795/mac).
