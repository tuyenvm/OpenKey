#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$DIR/build"

if [ "$1" == "clean" ]; then
  echo "🧹 Đang dọn dẹp thư mục build..."
  rm -rf "$BUILD_DIR"
  echo "✅ Đã xóa thư mục build."
  exit 0
fi

echo "🔨 Đang chuẩn bị build OpenKey (Release - Universal Binary)..."
# Xóa bản app cũ để đảm bảo luôn tạo ra bản build mới 100%, không bị lẫn file cũ
rm -rf "$BUILD_DIR/OpenKey.app"

xcodebuild -project "$DIR/Sources/OpenKey/macOS/OpenKey.xcodeproj" \
  -scheme OpenKey \
  -configuration Release \
  -derivedDataPath "$BUILD_DIR/DerivedData" \
  build \
  CONFIGURATION_BUILD_DIR="$BUILD_DIR" \
  CODE_SIGN_IDENTITY="-" \
  CODE_SIGN_STYLE="Manual"

echo "Build mới thành công tại: $BUILD_DIR/OpenKey.app"
