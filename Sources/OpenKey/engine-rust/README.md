# Rust engine — macOS milestone 2

The macOS app now links this dependency-free Rust static library instead of the five legacy C++ engine sources. Objective-C/AppKit still owns the UI and keyboard event tap. `RustBridge.cpp` adapts the existing Objective-C++ callers; composition, spelling, macros, encoding conversion and remembered app data run in Rust. Windows and Linux source/build files are unchanged.

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

This milestone changes the engine and its build integration only. The draft now also includes milestone 3 event-tap recovery, Spotlight/terminal routing and field-boundary resets. Milestone 4 now adds setup/settings, permissions, app rules and optional Globe switching. Signing and notarization remain deferred. No live-app typing or macOS permission grant is claimed by the automated checks.

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


## Milestone 4 user-experience checkpoint

The menu-bar control panel and app reopen now show native AppKit tabs for setup, everyday typing options and application rules. The original settings remain available through **Cài đặt nâng cao**.

- Engine/preferences initialize independently of the keyboard tap, so existing macros and remembered data are available safely before permission. Missing Accessibility permission leaves OpenKey running and opens setup. An explicit button requests permission and opens the system Accessibility panel; the setup window checks status once per second and attempts connection after approval. Revocation stops interception. An intentional pause survives automatic checks and requires the retry button.
- Setup explains system-wide keyboard access, recommends macOS ABC to avoid competing input methods, and offers a physical typing test field with Telex/VNI examples. Status reports permission, secure input, pause, active tap or connection failure; it does not claim that a connected tap proves typing compatibility.
- Everyday controls expose language, input method, encoding and remembering language per app. New installations/reset defaults leave launch-at-login off; existing saved preferences are retained.
- App rules use bundle identifiers in the separate `AppLanguageRules` preference: English (0), Vietnamese (1), or remember (2). Fixed rules override the switch shortcut; remember works even with global remembering off. Removing a rule restores global behavior. Existing macro and smart-switch persistence formats remain unchanged.
- `GlobeSwitch` defaults off. A standalone Fn/Globe release toggles language; Fn plus another key/modifier, repeated down events and interrupted gestures do not toggle. Events retain native delivery. Users must manually set macOS's Globe action to Do Nothing to avoid conflicts; OpenKey never changes that OS shortcut.
- The settings window uses standard window styling and recenters if its saved position is entirely off-screen.

Validation: `sh scripts/check-macos-input.sh` now also runs isolated settings tests for permission approval/revocation, explicit pause/retry and rule persistence. Native callback tests cover fixed/inherited/remembered app languages and Fn gesture boundaries. Xcode 27.2 builds both arm64 and x86_64 with deployment target 12.0. Computer use verified all three tabs and adding, changing and removing a TextEdit rule in the isolated settings preview. The preview uses fake permission/tap adapters and a separate preference domain; it does not establish live typing or real TCC approval behavior.

The milestone-4 app has since been launched and authorized; see the live results below. Physical Globe behavior and the outstanding milestone-3 live checks still need review testing. Updates, login-item API migration, signing/notarization workflow and release verification remain milestone 5.

Platform references: [Apple Accessibility trust check](https://developer.apple.com/documentation/applicationservices/1459186-axisprocesstrustedwithoptions) (permission prompting is asynchronous), [modifier flag documentation](https://developer.apple.com/documentation/appkit/nsevent/modifierflags-swift.struct/function?language=objc). Fn handling also checks the physical `kVK_Function` keycode, so other keys carrying the function flag do not count as standalone Fn taps.


### Milestone 4 live computer-test results

The review artifact at `/Users/hai/Downloads/OpenKey-milestone-4/OpenKey.app` was tested without rebuilding or replacing it.

- **Passed:** missing permission left the app running with setup visible. The setup link opened the system access panel. After user-approved Touch ID and replacing the stale OpenKey permission entry with this artifact, the same process (PID 72189) reported an active keyboard connection without restart; diagnostics confirmed an enabled tap.
- **Passed:** switching English/Vietnamese, Telex/VNI guidance, toggling the optional Globe preference back off, entering/exiting advanced settings, and adding/changing/removing a real TextEdit application rule.
- **Passed with physical user input:** with macOS on ABC, fixed English left `a1` literal and Option-Z did not override English. Fixed Vietnamese produced `á` from `a1` and Option-Z did not override Vietnamese. TextEdit's document also visibly contained accented output.
- **Failed/unresolved:** the user reported that TextEdit's “remember choice” rule did not restore English with global remembering off after selecting Vietnamese outside TextEdit. The user clarified that Option-Z successfully switched TextEdit to English, but returning to TextEdit restored Vietnamese. The restore result is therefore wrong; whether saving or restoring the per-app value is responsible remains unresolved. No production fix is claimed. The expanded native test exercises Option-Z, saving English, leaving for another app, changing the current language and restoring English with global remembering off; it passes with isolated preferences and mocked app identity/delegate.
- **Inconclusive:** injected setup-field VNI `a1` and Telex `as` remained literal. Physical Fn/Globe behavior, real permission revocation/retry and secure-input status were not exercised in this live session; native checks cover those paths.

Cleanup restored VNI, Unicode, Vietnamese, the original Option-Z configuration and global remembering; the temporary TextEdit rule was removed and Globe switching left off. Saved macro bytes were unchanged. macOS ABC was selected by the user for the physical checks; the earlier active system source was not recorded. The authorized milestone-4 app remains running. No production implementation changed during this follow-up, and milestone 5 has not started. The PR remains draft pending review of the remembered-choice failure.
