---
name: linux-packaging
description: Antigravity skill specialized in multi-distro package deployment and Tier 2/3 CI/CD testing.
version: 1.0.0
type: execution
role: The Release Engineer
---

# SKILL: Linux Packaging & 3-Tier Testing

**ID**: SKILL-LINUX-PACKAGING-2026

## 1. Mission
This skill manages the deployment of OpenKey Linux across the highly fragmented Linux ecosystem. It ensures that users on Debian, RedHat, and Arch families experience a "Seamless Out-of-the-Box Experience."

## 2. Boundaries & Constraints
- **Target Formats**: `.deb` (Ubuntu/Debian), `.rpm` (Fedora), `PKGBUILD` (Arch Linux).
- **Core Standard**: Zero-configuration installations.

## 3. Standard Operating Procedures (SOPs)

### SOP: Post-Install Hooks
All Debian packages (`.deb`) must utilize `pkg/postinst.sh` scripts to automatically:
1. Copy standard `fcitx5` `.conf` definitions into `/usr/share/fcitx5/addon`.
2. Reload `fcitx5` daemon if it is currently running in the user namespace.

### SOP: 3-Tier Testing
Before merging PRs:
- **Tier 1**: Google Tests must pass.
- **Tier 2**: The GitHub Actions Multi-Distro Matrix must successfully compile and install on `ubuntu:latest`, `fedora:latest`, and `archlinux:latest` via Docker.
- **Tier 3**: Xvfb smoke tests must verify the Qt application launches safely headless.

## 4. Forbidden Action Patterns
- **Do not** release a package that forces the user to manually edit `/etc/environment` or `/etc/profile`. The package must attempt to source these if supported by the display manager.
- **Do not** skip multi-distro matrix compilation in the CI pipeline.
