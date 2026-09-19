#!/usr/bin/env bash
set -e

# Build script for OpenKey on macOS (Apple Silicon arm64 / Intel)
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
APP_BUNDLE="$BUILD_DIR/OpenKey.app"
CONTENTS_DIR="$APP_BUNDLE/Contents"
MACOS_DIR="$CONTENTS_DIR/MacOS"
RESOURCES_DIR="$CONTENTS_DIR/Resources"

echo "=================================================="
echo "   Building OpenKey for macOS (Optimized Native)   "
echo "=================================================="

SDK_PATH=$(xcrun --show-sdk-path)
CXX_INC="$SDK_PATH/usr/include/c++/v1"
ARCH=$(uname -m)

echo "-> Detected Architecture: $ARCH"
echo "-> Using macOS SDK: $SDK_PATH"

mkdir -p "$BUILD_DIR/objs"
mkdir -p "$MACOS_DIR"
mkdir -p "$RESOURCES_DIR"

echo "-> Compiling C++ Core Engine (LTO)..."
clang++ -O3 -flto -arch "$ARCH" -isysroot "$SDK_PATH" -isystem "$CXX_INC" \
  -I "$PROJECT_DIR/Sources/OpenKey/engine" \
  -I "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey" \
  -std=c++14 -c \
  "$PROJECT_DIR/Sources/OpenKey/engine/Engine.cpp" \
  "$PROJECT_DIR/Sources/OpenKey/engine/Vietnamese.cpp" \
  "$PROJECT_DIR/Sources/OpenKey/engine/Macro.cpp" \
  "$PROJECT_DIR/Sources/OpenKey/engine/SmartSwitchKey.cpp" \
  "$PROJECT_DIR/Sources/OpenKey/engine/ConvertTool.cpp"

echo "-> Compiling Objective-C++ sources (LTO)..."
clang++ -O3 -flto -arch "$ARCH" -isysroot "$SDK_PATH" -isystem "$CXX_INC" \
  -I "$PROJECT_DIR/Sources/OpenKey/engine" \
  -I "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey" \
  -std=c++14 -fobjc-arc -c \
  "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/ConvertToolViewController.mm" \
  "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/MacroViewController.mm" \
  "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/OpenKey.mm"

echo "-> Compiling Objective-C sources (LTO)..."
clang -O3 -flto -arch "$ARCH" -isysroot "$SDK_PATH" \
  -I "$PROJECT_DIR/Sources/OpenKey/engine" \
  -I "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey" \
  -fobjc-arc -c \
  "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/main.m" \
  "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/AppDelegate.m" \
  "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/ViewController.m" \
  "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/AboutViewController.m" \
  "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/MyTextField.m" \
  "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/MJAccessibilityUtils.m" \
  "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/OpenKeyManager.m"

echo "-> Linking executable with LTO & Dead Code Strip..."
clang++ -O3 -flto -Wl,-dead_strip -arch "$ARCH" -isysroot "$SDK_PATH" *.o \
  -framework Cocoa -framework Carbon -framework ServiceManagement -lproc \
  -o "$MACOS_DIR/OpenKey"

mv *.o "$BUILD_DIR/objs/"

echo "-> Assembling application bundle..."
# Copy resources from existing /Applications/OpenKey.app if available
if [ -d "/Applications/OpenKey.app" ]; then
  cp -R /Applications/OpenKey.app/Contents/Resources/* "$RESOURCES_DIR/" 2>/dev/null || true
  cp /Applications/OpenKey.app/Contents/Info.plist "$CONTENTS_DIR/" 2>/dev/null || true
  cp /Applications/OpenKey.app/Contents/PkgInfo "$CONTENTS_DIR/" 2>/dev/null || true
  if [ -d "/Applications/OpenKey.app/Contents/Library" ]; then
    cp -R /Applications/OpenKey.app/Contents/Library "$CONTENTS_DIR/" 2>/dev/null || true
  fi
fi

# Overlay repository resources
if [ -d "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/Resources" ]; then
  cp -R "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/Resources/"* "$RESOURCES_DIR/" 2>/dev/null || true
fi

# Ensure Info.plist exists
if [ ! -f "$CONTENTS_DIR/Info.plist" ] && [ -f "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/Info.plist" ]; then
  cp "$PROJECT_DIR/Sources/OpenKey/macOS/ModernKey/Info.plist" "$CONTENTS_DIR/"
fi

# Clean extended attributes and sign app bundle ad-hoc
echo "-> Cleaning attributes and signing ad-hoc..."
find "$APP_BUNDLE" -exec xattr -c {} + 2>/dev/null || true
codesign --force --deep --sign - "$APP_BUNDLE"

echo "=================================================="
echo "   Build completed: $APP_BUNDLE"
echo "=================================================="

# Handle install argument
if [ "$1" = "install" ]; then
  echo "-> Installing to /Applications/OpenKey.app..."
  
  if pgrep -x "OpenKey" > /dev/null; then
    echo "-> Stopping currently running OpenKey..."
    killall OpenKey 2>/dev/null || true
    sleep 1
  fi

  if [ -d "/Applications/OpenKey.app" ] && [ ! -d "/Applications/OpenKey.app.backup" ]; then
    echo "-> Creating backup at /Applications/OpenKey.app.backup..."
    cp -R /Applications/OpenKey.app /Applications/OpenKey.app.backup
  fi

  echo "-> Updating binary in /Applications/OpenKey.app/Contents/MacOS/OpenKey..."
  cp "$MACOS_DIR/OpenKey" "/Applications/OpenKey.app/Contents/MacOS/OpenKey"
  find /Applications/OpenKey.app -exec xattr -c {} + 2>/dev/null || true
  codesign --force --deep --sign - /Applications/OpenKey.app

  echo "-> Launching updated OpenKey..."
  open /Applications/OpenKey.app
  echo "-> Done! OpenKey is running with your personalized optimizations."
fi
