# Makefile for building and installing NKey (OpenKey macOS fork)
#
#   make            # build Release (arm64, Apple Silicon)
#   make install    # build, sign, then copy to /Applications (quits a running copy first)
#   make run        # install, then launch
#   make clean      # remove build output
#   make uninstall  # quit and remove from /Applications
#
# Signing strategy: xcodebuild produces an ad-hoc build (reliable, no provisioning
# hassle), then we RE-SIGN the .app with a stable Apple Development certificate via
# codesign. A stable signature is what lets macOS keep the Accessibility grant across
# rebuilds. Override the identity with `make install SIGN_IDENTITY="..."`, or pass
# SIGN_IDENTITY= (empty) to stay ad-hoc (macOS will then re-prompt every rebuild).

APP           := NKey
PROJECT       := OpenKey.xcodeproj
SCHEME        := OpenKey
CONFIG        := Release
ARCH          := arm64
DERIVED       := build
BUILT_APP     := $(DERIVED)/Build/Products/$(CONFIG)/$(APP).app
INSTALL_DIR   := /Applications
INSTALLED_APP := $(INSTALL_DIR)/$(APP).app

# Stable code-signing identity (keychain). Empty => leave the ad-hoc signature.
SIGN_IDENTITY ?= Apple Development: Duy Nhat Hoang (QCK566ZXTF)

.PHONY: all build sign install run clean uninstall

all: build

build:
	xcodebuild -project $(PROJECT) -scheme $(SCHEME) -configuration $(CONFIG) \
		-derivedDataPath $(DERIVED) \
		-arch $(ARCH) ONLY_ACTIVE_ARCH=NO \
		CODE_SIGN_IDENTITY="-" CODE_SIGN_STYLE=Manual DEVELOPMENT_TEAM="" \
		build
	@echo "Built: $(BUILT_APP)"

# Re-sign with the stable identity so the Accessibility grant survives rebuilds.
sign: build
ifneq ($(strip $(SIGN_IDENTITY)),)
	codesign --force --deep -s "$(SIGN_IDENTITY)" "$(BUILT_APP)"
	@echo "Signed with: $(SIGN_IDENTITY)"
else
	@echo "SIGN_IDENTITY empty -> keeping ad-hoc signature"
endif

install: sign
	@echo "Quitting any running $(APP)..."
	-osascript -e 'quit app "$(APP)"' >/dev/null 2>&1 || true
	-pkill -x $(APP) >/dev/null 2>&1 || true
	rm -rf "$(INSTALLED_APP)"
	cp -R "$(BUILT_APP)" "$(INSTALLED_APP)"
	-xattr -dr com.apple.quarantine "$(INSTALLED_APP)"
	@echo "Installed -> $(INSTALLED_APP)"
	@echo "Grant access: System Settings > Privacy & Security > Accessibility > $(APP)"

run: install
	open "$(INSTALLED_APP)"

clean:
	rm -rf $(DERIVED)

uninstall:
	-osascript -e 'quit app "$(APP)"' >/dev/null 2>&1 || true
	-pkill -x $(APP) >/dev/null 2>&1 || true
	rm -rf "$(INSTALLED_APP)"
	@echo "Removed $(INSTALLED_APP)"
