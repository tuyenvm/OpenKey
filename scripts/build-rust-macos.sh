#!/bin/sh
set -eu
REPO_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
export PATH="$HOME/.cargo/bin:$PATH"
cd "$REPO_ROOT"
command -v cargo >/dev/null 2>&1 || { echo "Install Rust with rustup before building OpenKey." >&2; exit 1; }
RUST_TARGET_DIR="${DERIVED_FILE_DIR:?}/rust"
mkdir -p "$RUST_TARGET_DIR"
RUST_LIBRARIES=""
for arch in ${ARCHS:?}; do
    case "$arch" in
        arm64) target=aarch64-apple-darwin ;;
        x86_64) target=x86_64-apple-darwin ;;
        *) echo "Unsupported macOS architecture: $arch" >&2; exit 1 ;;
    esac
    cargo build --locked --release --manifest-path Sources/OpenKey/engine-rust/Cargo.toml \
        --target-dir "$RUST_TARGET_DIR" --target "$target"
    # Paths are passed as separate quoted arguments below, not evaluated as shell code.
    RUST_LIBRARIES="$RUST_LIBRARIES $target"
done
set --
for target in $RUST_LIBRARIES; do
    set -- "$@" "$RUST_TARGET_DIR/$target/release/libopenkey_engine.a"
done
xcrun lipo -create "$@" -output "${BUILT_PRODUCTS_DIR:?}/libopenkey_engine.a"
