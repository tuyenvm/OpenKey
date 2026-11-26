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
