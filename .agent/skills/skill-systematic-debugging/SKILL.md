---
name: systematic-debugging
description: "Use when encountering any C++ bug, CMake failure, Fcitx5 crash, or unexpected typing behavior before proposing fixes."
version: 1.0.0
type: execution
role: The Debugger
---

# SKILL: Systematic Debugging (OpenKey Context)

**ID**: SKILL-SYSTEMATIC-DEBUGGING-2026

## Overview
Random fixes waste time and create new bugs, especially in a low-level Systems Language (C++) integrated with Display Servers (Wayland/Fcitx5). Quick patches mask underlying memory corruption or latency issues.

**Core principle:** ALWAYS find root cause before attempting fixes. Symptom fixes are failure.

## The Iron Law
```
NO FIXES WITHOUT ROOT CAUSE INVESTIGATION (GDB/Valgrind/Logs) FIRST
```
If you haven't completed Phase 1, you cannot propose fixes.

## The Four Phases

### Phase 1: Root Cause Investigation
**BEFORE attempting ANY fix:**
1. **Read Error Messages Carefully**: Read CMake errors, GCC compiler outputs, or Fcitx5 daemon logs (`journalctl --user -xe | grep fcitx5`).
2. **Reproduce Consistently**: Does the bug happen only on Wayland? X11? GTK apps? KDE apps?
3. **Trace Data Flow**: Use the `openkey-cli` tool to map the Vietnamese keystroke logic outside of the UI:
   ```bash
   ./build/openkey-cli -k "t-o-o-i"
   ```

### Phase 2: Pattern Analysis
1. Compare against working C++ patterns in `Engine.cpp`.
2. Compare against working Fcitx5 module templates (`fcitx5-module-template`).

### Phase 3: Hypothesis and Testing
1. Form a single hypothesis (e.g., "The Wayland text-input-v3 buffer is not receiving the commit string fast enough").
2. Test minimally: compile a 1-line change using `make -j`.

### Phase 4: Implementation
1. Fix the root cause in C++ or CMake.
2. Verify using `openkey-cli --test` and the 3-Tier Multi-Distro tests defined in `rules.md`.

## Forbidden Actions
- **Do not** propose "just checking if this works" fixes.
- **Do not** write Python or Node.js scripts to debug a native C++ application unless parsing logs.
- **Do not** guess memory leaks. Use AddressSanitizer (ASan) or Valgrind.
