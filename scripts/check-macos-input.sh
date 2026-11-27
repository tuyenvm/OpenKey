#!/bin/sh
set -eu
REPO_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
export PATH="$HOME/.cargo/bin:$PATH"
cd "$REPO_ROOT"
TEST_DIR=$(mktemp -d)
trap 'rm -rf "$TEST_DIR"' EXIT
cargo build --locked --release --manifest-path Sources/OpenKey/engine-rust/Cargo.toml
clang++ -std=c++17 -fobjc-arc -Wno-deprecated-declarations -I Sources/OpenKey/engine \
    scripts/macos-input-tests.mm Sources/OpenKey/engine-rust/RustBridge.cpp \
    Sources/OpenKey/engine-rust/target/release/libopenkey_engine.a \
    -framework Cocoa -framework Carbon -framework Security -liconv -o "$TEST_DIR/input"
"$TEST_DIR/input"
clang -fobjc-arc -Wno-deprecated-declarations scripts/macos-tap-tests.m \
    -framework Cocoa -framework Carbon -o "$TEST_DIR/lifecycle"
"$TEST_DIR/lifecycle"

clang -fobjc-arc -Wno-deprecated-declarations -mmacosx-version-min=12.0 \
    -c Sources/OpenKey/macOS/ModernKey/MyTextField.m -o "$TEST_DIR/keyfield.o"
clang++ -std=c++17 -fobjc-arc -Wno-deprecated-declarations -mmacosx-version-min=12.0 \
    -I Sources/OpenKey/engine -c scripts/macos-settings-tests.mm -o "$TEST_DIR/settings.o"
clang++ -std=c++17 -Wno-deprecated-declarations -I Sources/OpenKey/engine -mmacosx-version-min=12.0 \
    -c Sources/OpenKey/engine-rust/RustBridge.cpp -o "$TEST_DIR/bridge.o"
xcrun swiftc -swift-version 5 -parse-as-library -emit-object -module-name OpenKeySettingsForm \
    -target "$(uname -m)-apple-macos12.0" Sources/OpenKey/macOS/ModernKey/SettingsForm.swift -o "$TEST_DIR/form.o"
xcrun swiftc -target "$(uname -m)-apple-macos12.0" \
    "$TEST_DIR/settings.o" "$TEST_DIR/bridge.o" "$TEST_DIR/keyfield.o" "$TEST_DIR/form.o" \
    Sources/OpenKey/engine-rust/target/release/libopenkey_engine.a \
    -framework Cocoa -framework Carbon -lc++ -liconv -o "$TEST_DIR/settings"
"$TEST_DIR/settings"

clang -fobjc-arc -Wno-deprecated-declarations -c Sources/OpenKey/macOS/ModernKey/AppDelegate.m -o "$TEST_DIR/delegate.o"
clang++ -std=c++17 -fobjc-arc -Wno-incomplete-implementation -Wno-deprecated-declarations \
    scripts/macos-menu-tests.mm "$TEST_DIR/delegate.o" \
    -framework Cocoa -framework Carbon -framework ServiceManagement -o "$TEST_DIR/menu"
"$TEST_DIR/menu"
