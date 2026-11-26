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

This milestone changes the engine and its build integration only. The draft now also includes milestone 3 event-tap recovery, Spotlight/terminal routing and field-boundary resets. Permissions/setup UI, app rules, Globe switching, signing and notarization remain deferred. No live-app typing or macOS permission grant is claimed by the automated checks.

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

Live testing on macOS 27.0: the app launches after reapproving the local build and its event tap is present and enabled. Computer-use `pressKey`/`typeText` produced literal `a1`/`vie65t` in TextEdit; the cause has not been established, so live typing is **not marked passed**. A physical-keyboard `a1` check is pending. Original VNI, Unicode, Option-Z, smart switching and browser-workaround settings were restored after the test. Remote-desktop, sleep/wake, terminal and Spotlight end-to-end behavior still need live verification.
