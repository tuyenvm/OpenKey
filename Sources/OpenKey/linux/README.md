# OpenKey for Linux
### Open source Vietnamese Input Engine for Linux

## Build Requirements
- Ubuntu 20.04+ / Debian 11+ (or any Linux with GCC and CMake)
- CMake ≥ 3.10
- GCC with C++11 support (`build-essential`)

## Build Instructions

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install -y cmake build-essential

# Build
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Verify
./openkey-cli --version
./openkey-cli --test
```

## Usage

```bash
./openkey-cli --version    # Show version info
./openkey-cli --test       # Run engine self-test
```

## What's Included
- **OpenKey Engine**: Full Vietnamese input processing (Telex, VNI, Simple Telex)
- **CLI Tool**: Build verification and engine self-test
- **CI/CD**: GitHub Actions for automated Linux builds

## Roadmap
- [x] Engine compilation on Linux
- [x] CLI self-test tool
- [x] GitHub Actions CI/CD
- [ ] IBus/Fcitx integration (GUI)
- [ ] `.deb` package
- [ ] Snap/Flatpak support