---
name: verification-before-completion
description: "Used to verify all C++ code, CMake builds, and testing tiers BEFORE claiming a task is done."
version: 1.0.0
type: execution
role: The Gatekeeper
---

# SKILL: Verification Before Completion (OpenKey Context)

**ID**: SKILL-VERIFICATION-2026

## 1. Mission
Prevent developers and agents from prematurely claiming "I fixed it" without concrete evidence of success within the OpenKey C++/Linux ecosystem.

## 2. The Verification Protocol

Before checking the `[ ]` to `[x]` in `task.md` or `DoD.md`, you MUST provide terminal output proving the fix works.

### Rule 1: Compile Evidence
You cannot say "I added the Fcitx5 bridge." You MUST run:
```bash
cd build && make
```
And verify that the `.so` file or `openkey-linux-ui` binary is generated without GCC errors.

### Rule 2: Test Evidence
You cannot say "The Vietnamese engine passes logic." You MUST run:
```bash
./build/openkey-cli --test
```
And verify `[PASS] New session started` is printed.

### Rule 3: Quality Gate (Rule 6 Compliance)
If you finish a major Phase (e.g., Phase 3), you MUST update `.agent/roadmap.md` and `.agent/DoD.md` to reflect the new state, obeying **Rule 6: Living Document Synchronization** from `rules.md`.

## 3. Forbidden Actions
- **Do not** assume code works because it "looks right."
- **Do not** use mock test runners or Node.js scripts. Run the native C++ binaries directly.
- **Do not** end a task with "I have implemented the code, you can now check it." You must verify it yourself.
