.PHONY: all build install clean

all: build

build:
	@chmod +x build_macos.sh
	@./build_macos.sh

install:
	@chmod +x build_macos.sh
	@./build_macos.sh install

clean:
	@rm -rf build
	@echo "Cleaned build directory."
