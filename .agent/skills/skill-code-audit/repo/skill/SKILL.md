---
name: h3
description: Heavy3 Code Audit - Multi-model code review for coding agents (Sponsored by Heavy3.ai)
argument-hint: "[pr <number>] [plan <file>] [<file>.md] [<range>] [--council] [--staged] [--commit] [--free] [--model MODEL]"
allowed-tools: Read, Bash, Glob, Grep, Write
disable-model-invocation: true
---

# Heavy3 Code Audit - The Multi-Model Code Review for Coding Agents

**Sponsored by [Heavy3.ai](https://heavy3.ai) - Multi-model AI for high-stakes decisions**

You are helping the user get AI-powered code reviews via OpenRouter.

**All features are free and open source:**
- Single model review with Kimi K2.5 (best price/performance)
- 3-model council with GPT 5.2 + Gemini 3 Pro + Grok 4
- Web search integration
- Up to 200K token context

## Arguments

`$ARGUMENTS` can contain:

**Explicit targets (no confirmation needed):**
- `pr <number>` - Review a GitHub pull request by number
- `plan <path>` - Review a specific plan file
- `<file>.md` - Shorthand for plan review (any .md file)
- `<range>` - Review a commit range (e.g., `HEAD~3..HEAD`, `abc123..def456`)

**Scope modifiers:**
- `--staged` - Force review of only staged changes
- `--commit` - Force review of the last commit only

**Mode options:**
- `--council` - Use 3-model council (GPT 5.2 + Gemini 3 Pro + Grok 4)
- `--free` - Use rotating free model from config
- `--model <name>` - Override model (shortcuts: gpt, kimi, deepseek, free)

---

## Smart Detection

**When `/h3` is invoked without explicit targets, automatically detect intent and confirm with user.**

### Detection Priority

| Priority | Condition | Action |
|----------|-----------|--------|
| 1 | Explicit argument provided | Execute directly, no confirmation |
| 2 | Uncommitted changes exist | Confirm: review changes? |
| 3 | No changes + plan detected | Confirm: review the plan? |
| 4 | No changes + no plan | Ask: review commits or specify target? |

### Step-by-Step Smart Detection Workflow

**Step 1: Check for explicit arguments**

If `$ARGUMENTS` contains any of these, skip detection and execute directly:
- `pr <number>` → PR review
- `plan <path>` → Plan review of specific file
- `<file>.md` (any markdown file path) → Plan review
- `<range>` (commit range like `HEAD~3..HEAD`) → Code review of range
- `--staged` → Staged changes review
- `--commit` → Last commit review

**Step 2: Check for uncommitted changes**

Run: `git status --porcelain`

If output is NOT empty (changes exist):
```markdown
## Review Scope

I detected uncommitted changes:
- **Staged**: [X] files
- **Unstaged**: [Y] files

**Review all changes?** (y/n)
```

If user confirms, proceed with code review of all changes (`git diff HEAD`).

**Step 3: Check for plan (if no changes)**

Check these locations in order:
1. **Conversation context**: Did Claude just create or discuss a plan in this session?
2. **Current directory**: Does `plan.md`, `PLAN.md`, or `*.plan.md` exist?
3. **Plans folder**: Most recent `.md` file in `~/.claude/plans/`

If plan found:
```markdown
## Plan Detected

Found plan: `[path/to/plan.md]`
Last modified: [date]

**Review this plan?** (y/n)
```

If user confirms, proceed with plan review.

**Step 4: No changes and no plan - ask user**

```markdown
## No Changes Detected

No uncommitted changes or plans found.

**What would you like to review?**
1. Latest commit (`HEAD~1..HEAD`)
2. Recent commits (specify range, e.g., `HEAD~3..HEAD`)
3. Specific file or folder
4. Cancel
```

Wait for user response and proceed accordingly.

### Commit Range Support

For reviewing features/bug fixes spanning multiple commits:

| Input | Git Command | Description |
|-------|-------------|-------------|
| `HEAD~1..HEAD` | `git diff HEAD~1..HEAD` | Last 1 commit |
| `HEAD~3..HEAD` | `git diff HEAD~3..HEAD` | Last 3 commits |
| `abc123..HEAD` | `git diff abc123..HEAD` | From specific commit to HEAD |
| `abc123..def456` | `git diff abc123..def456` | Between two commits |

When user specifies a range, show commit summary before the review:
```markdown
## Reviewing Commit Range: HEAD~3..HEAD

| Commit | Date | Author | Message |
|--------|------|--------|---------|
| abc123 | 2025-01-28 | John | feat: Add login |
| def456 | 2025-01-29 | John | fix: Handle edge case |
| ghi789 | 2025-01-30 | John | test: Add unit tests |

**3 commits, +150/-30 lines across 8 files**
```

## Configuration
Read the config from: `~/.claude/skills/h3/config.json`
```json
{
  "model": "moonshotai/kimi-k2.5",
  "free_model": "nvidia/nemotron-3-nano-30b-a3b:free",
  "reasoning": "high",
  "docs_folder": "documents",
  "max_context": 200000,
  "enable_web_search": true
}
```

API key is stored in `~/.claude/skills/h3/.env`:
```
OPENROUTER_API_KEY=your-key-here
```

## Preprocessed Context

### Git Status
!`git status --short 2>/dev/null || echo "Not a git repo"`

### Changed Files
!`git diff HEAD --name-only 2>/dev/null || echo "No changes"`

### Git Diff (truncated to 10000 chars)
!`git diff HEAD 2>/dev/null | head -c 10000 || echo "No diff"`

---

## Mode Routing

### IF ARGUMENTS contains "--council":
1. Run council.py instead of review.py
2. After council reviews, YOU synthesize findings with comparison table

### IF ARGUMENTS contains "--free":
1. Read free_model from config.json
2. Warn user: "Note: Free models rotate on OpenRouter. Your configured model may be unavailable."
3. If API call fails with model error:
   - Run: `python3 ~/.claude/skills/h3/scripts/list-free-models.py --json`
   - Show available free models
   - Ask: "Pick a new free model?"
   - If user selects, UPDATE config.json with new free_model
   - Retry with new model

---

## Scope Options

| Scope | Git Command | Use Case |
|-------|-------------|----------|
| Smart (default) | Auto-detected | Let `/h3` figure out what to review |
| `--staged` | `git diff --cached` | Force review of only staged changes |
| `--commit` | `git diff HEAD~1..HEAD` | Force review of the last commit |
| `<range>` | `git diff <range>` | Review multiple commits (e.g., `HEAD~3..HEAD`) |

**Error messages:**
- No staged changes (--staged): "No staged changes detected. Stage your changes first with `git add`."
- No commits (--commit): "No commits found. Make a commit first."
- Invalid range: "Invalid commit range. Check that both commits exist."

---

## Context Limits

| Max Context | Chars (approx) |
|-------------|----------------|
| 200K tokens | ~800K chars |

---

## Cost Estimation

**Before running a review**, estimate and display the cost to the user.

### OpenRouter Pricing (per 1M tokens, approximate)

| Model | Input | Output | Typical Review Cost |
|-------|-------|--------|---------------------|
| Kimi K2.5 (default) | $0.50 | $2.80 | ~$0.008-0.02 |
| GPT 5.2 (council) | $1.75 | $14.00 | ~$0.05-0.20 |
| Gemini 3 Pro (council) | $2.00 | $12.00 | ~$0.05-0.18 |
| Grok 4 (council) | $3.00 | $15.00 | ~$0.06-0.22 |

### Estimation Formula

```
input_tokens = total_context_chars / 4
output_tokens = ~2500 (typical review length)

# Single model mode (Kimi K2.5)
single_cost = (input_tokens * 0.50 + output_tokens * 2.80) / 1_000_000

# Council mode (all 3 models in parallel)
council_cost = (input_tokens * (1.75 + 2.00 + 3.00) + output_tokens * (14 + 12 + 15)) / 1_000_000
             ≈ input_tokens * 6.75/M + output_tokens * 41/M
```

### Display Cost Estimate and Confirm

**IMPORTANT: Show cost estimate BEFORE submitting to OpenRouter and wait for user confirmation.**

After gathering context but BEFORE calling the review API:

```markdown
## Cost Estimate

| Metric | Value |
|--------|-------|
| Context size | ~[X]K chars |
| Est. input tokens | ~[X]K |
| Model(s) | [model name(s)] |
| **Est. cost** | **~$[X.XX]** |

**Proceed with review?** (y/n)
```

**Wait for user to confirm before submitting.**

If user declines, exit gracefully: "Review cancelled."

**Examples:**
- Small review (10K chars / 2.5K tokens): Single ~$0.002, Council ~$0.12
- Medium review (50K chars / 12.5K tokens): Single ~$0.004, Council ~$0.19
- Large review (200K chars / 50K tokens): Single ~$0.015, Council ~$0.44

---

## Handle Large Changes First

**Before executing any review workflow**, check if changes are too large.

### Step 0: Estimate Context Size
- Count characters in: diff + file contents + docs + tests
- **Rule of thumb**: 1 token ≈ 4 characters
- Check against 200K token limit

**Quick size indicators (likely too large):**
- More than 50 changed files
- More than 10,000 additions + deletions
- Total content exceeds tier limit

### Step 1: If Large, Stop and Present Module Options

If estimated context exceeds limit, **DO NOT proceed automatically**. Present module options to user:

```markdown
## Large Change Detected - Module Selection Required

| Metric | Value | Limit |
|--------|-------|-------|
| Changed files | [X] | ~50 |
| Lines changed | +[X]/-[Y] | ~10,000 |
| Est. tokens | ~[X]K | 200K |

I found [X] changed files across these areas:

| # | Module | Files | Est. Tokens | Description |
|---|--------|-------|-------------|-------------|
| 1 | src/components | 18 | ~25K | UI components |
| 2 | src/utils | 12 | ~15K | Utility functions |
| 3 | src/api | 10 | ~20K | API handlers |
| 4 | tests | 5 | ~8K | Test files |

**How would you like to proceed?**
1. Review modules separately (4 reviews, ~$X.XX total)
2. Combine modules 2+3 into one review (3 reviews)
3. Review all together (will truncate to fit limit)
4. Custom grouping (tell me which modules to combine)
```

### Step 2: Module-by-Module Review Workflow

After user selects grouping:

1. **Create progress tracking table**:
```markdown
## Review Progress

| Module | Files | Status | Key Findings |
|--------|-------|--------|--------------|
| src/components | 18 | ⏳ In Progress | - |
| src/utils + src/api | 22 | ⏸️ Pending | - |
| tests | 5 | ⏸️ Pending | - |
```

2. **Review each module group**:
   - Run the review for current module
   - Update the progress table with status and key findings
   - After each review, ask: "Continue to next module? (y/n)"
   - If user says no, offer to save progress and resume later

3. **Update progress after each module**:
```markdown
## Review Progress (Updated)

| Module | Files | Status | Key Findings |
|--------|-------|--------|--------------|
| src/components | 18 | ✅ Complete | 2 security, 1 perf issue |
| src/utils + src/api | 22 | ⏳ In Progress | - |
| tests | 5 | ⏸️ Pending | - |
```

4. **Final cross-module synthesis** (after all modules reviewed):
```markdown
## Cross-Module Summary

### All Issues by Category

**Security Issues (across all modules):**
- [src/components:42] XSS vulnerability in user input
- [src/api:15] Missing authentication check

**Performance Issues (across all modules):**
- [src/components:88] N+1 query in list render

**Correctness Issues (across all modules):**
- [src/utils:23] Off-by-one error in pagination

### Recommended Fix Priority
1. **CRITICAL**: [Security issue from module 1]
2. **HIGH**: [Performance issue from module 2]
3. **MEDIUM**: [Other issues...]

### Cross-Module Concerns
- [Any issues that span multiple modules]
- [Architectural concerns from combined view]
```

---

## Your Task

**Follow the Smart Detection workflow, then execute the appropriate review.**

### Step 0: Parse Arguments and Apply Smart Detection

1. **Check for explicit targets first** (skip detection if found):
   - `pr <number>` → Go to PR Review workflow
   - `plan <path>` or `<file>.md` → Go to Plan Review workflow
   - `<range>` (e.g., `HEAD~3..HEAD`) → Go to Commit Range Review workflow
   - `--staged` → Go to Staged Changes Review workflow
   - `--commit` → Go to Last Commit Review workflow

2. **If no explicit target, run Smart Detection**:
   - Check `git status --porcelain` for uncommitted changes
   - If changes exist → Confirm with user, then Code Review workflow
   - If no changes → Check for plan (conversation context, `plan.md` in cwd, `~/.claude/plans/`)
   - If plan found → Confirm with user, then Plan Review workflow
   - If no plan → Ask user what to review (latest commit, range, or cancel)

### Code Review Workflow (uncommitted changes)

1. Determine scope based on arguments:
   - Default: `git diff HEAD` (all changes)
   - With `--staged`: `git diff --cached` (staged only)
2. Get full diff using appropriate git command
3. Get changed files list
4. Read FULL content of each changed file
5. Find relevant documentation (CLAUDE.md, docs folder)
6. Include related test files
7. Include conversation context (see below)
8. Compile context JSON to temp file
9. **Calculate and display cost estimate, wait for user confirmation** (see Cost Estimation section)
10. If user confirms, run review script:
    - Default: `python3 ~/.claude/skills/h3/scripts/review.py --type code --context-file <path>`
    - With --council: `python3 ~/.claude/skills/h3/scripts/council.py --type code --context-file <path>`

### Last Commit Review Workflow (`--commit`)

1. Check if commits exist: `git log -1 --oneline 2>/dev/null`
2. If no commits, report: "No commits found. Make a commit first." and exit
3. Get last commit metadata:
   - `git log -1 --pretty=format:"%H|%s|%an|%ad" --date=short` for hash, subject, author, date
4. Get the diff: `git diff HEAD~1..HEAD`
5. Get changed files: `git diff HEAD~1..HEAD --name-only`
6. Read FULL content of each changed file
7. Find relevant documentation (CLAUDE.md, docs folder)
8. Include related test files
9. Include conversation context (see below)
10. Add commit metadata to context JSON:
    ```json
    "commit_metadata": {
      "hash": "abc123...",
      "subject": "feat: Add user authentication",
      "author": "John Doe",
      "date": "2025-01-25"
    }
    ```
11. **Calculate and display cost estimate, wait for user confirmation** (see Cost Estimation section)
12. If user confirms, run review script:
    - Default: `python3 ~/.claude/skills/h3/scripts/review.py --type code --context-file <path>`
    - With --council: `python3 ~/.claude/skills/h3/scripts/council.py --type code --context-file <path>`

### Commit Range Review Workflow (`<range>` like `HEAD~3..HEAD`)

1. Parse the range from arguments (e.g., `HEAD~3..HEAD`, `abc123..def456`)
2. Validate range: `git rev-parse <start> <end> 2>/dev/null`
3. If invalid, report error and exit
4. Show commit summary (informational):
   ```bash
   git log --oneline --reverse <range>
   git diff <range> --stat
   ```
5. Get the diff: `git diff <range>`
6. Get changed files: `git diff <range> --name-only`
7. Read FULL content of each changed file
8. Find relevant documentation (CLAUDE.md, docs folder)
9. Include related test files
10. Include conversation context (see below)
11. Add commit range metadata to context JSON:
    ```json
    "commit_range": {
      "range": "HEAD~3..HEAD",
      "commits": [
        {"hash": "abc123", "subject": "feat: Add login", "date": "2025-01-28"},
        {"hash": "def456", "subject": "fix: Edge case", "date": "2025-01-29"}
      ],
      "total_commits": 2
    }
    ```
12. **Calculate and display cost estimate, wait for user confirmation** (see Cost Estimation section)
13. If user confirms, run review script (single model or council)

### Plan Review Workflow (`plan <path>` or `<file>.md` or detected plan)

1. Find the plan file:
   - If explicit path provided → Use that path
   - If `<file>.md` provided → Use that file
   - If detected via Smart Detection → Use detected path
   - If none → Check most recent in `~/.claude/plans/`
2. **Read the FULL plan content** and include it in the context JSON under `plan_content` field
3. Parse plan for file paths and read those files into `file_contents`
4. Find relevant documentation (CLAUDE.md, docs folder)
5. Include conversation context (see below)
6. Compile context JSON to temp file with `plan_content` included:
   ```json
   {
     "review_type": "plan",
     "plan_content": "# Full plan content here\n\n## Overview\n...",
     "file_contents": { ... },
     "documentation": { ... },
     "conversation_context": { ... }
   }
   ```
7. **Calculate and display cost estimate, wait for user confirmation** (see Cost Estimation section)
8. If user confirms, run review script:
   - Default: `python3 ~/.claude/skills/h3/scripts/review.py --type plan --context-file <path>`
   - With --council: `python3 ~/.claude/skills/h3/scripts/council.py --type plan --context-file <path>`

**IMPORTANT**: Do NOT pass `--plan-file` as a separate argument. The plan content MUST be included directly in the context JSON file under the `plan_content` key.

### PR Review Workflow (`pr <number>`)

1. Extract PR number from arguments
2. Fetch PR info: `gh pr view <number> --json title,body,author,baseRefName,headRefName,files,additions,deletions`
3. Get PR diff: `gh pr diff <number>`
4. Read full content of changed files
5. Find relevant documentation
6. Include conversation context (see below)
7. Compile context JSON with pr_metadata
8. **Calculate and display cost estimate, wait for user confirmation** (see Cost Estimation section)
9. If user confirms, run review script (single model or council)

---

## Include Conversation Context

Review the conversation history and include relevant context that explains the developer's intent:

1. **Original Request** - What did the user ask you to do? (1-2 sentences)
2. **Approach Notes** - Key decisions, constraints, or tradeoffs mentioned (bullet points)
3. **Relevant Exchanges** - 3-5 most relevant user messages and your responses that explain the changes:
   - Why this approach was chosen
   - Constraints or requirements mentioned
   - Errors encountered and how they were addressed
4. **Previous Review Findings** - If `/h3` was run earlier in this session, summarize key findings

**Selection criteria for relevant exchanges:**
- Messages that explain WHY changes were made
- Messages discussing tradeoffs or alternatives
- Messages mentioning constraints, requirements, or edge cases
- Messages about errors or bugs being fixed
- Skip: casual messages, unrelated topics, raw tool outputs

**Limits:**
- Maximum 3-5 exchanges (user message + your response = 1 exchange)
- Keep each message under 500 characters (truncate if needed)
- Total conversation context should not exceed ~2K tokens

---

## Context JSON Format

**IMPORTANT**: All content must be included IN the context JSON file. Do NOT pass separate file arguments.

```json
{
  "review_type": "plan" | "code" | "pr",
  "conversation_context": {
    "original_request": "Brief summary of what user originally asked for",
    "approach_notes": "Key decisions made during implementation",
    "relevant_exchanges": [
      {"role": "user", "content": "Can you add validation to the form?"},
      {"role": "assistant", "content": "I'll add Zod validation. Using inline validation rather than form-level because..."}
    ],
    "previous_review_findings": "Summary of any prior /h3 review in this session"
  },
  "plan_content": "# Full plan markdown content (REQUIRED for plan reviews)",
  "diff": "git diff output (for code/pr reviews)",
  "changed_files": ["path1", "path2"],
  "file_contents": {
    "path1": "full file content...",
    "path2": "full file content..."
  },
  "documentation": {
    "CLAUDE.md": "...",
    "documents/feature.md": "..."
  },
  "test_files": {
    "path1.test.ts": "..."
  },
  "pr_metadata": {
    "number": 123,
    "title": "...",
    "body": "...",
    "author": "...",
    "base_branch": "main",
    "head_branch": "feature",
    "additions": 100,
    "deletions": 50
  },
  "commit_metadata": {
    "hash": "abc123...",
    "subject": "feat: Add user authentication",
    "author": "John Doe",
    "date": "2025-01-25"
  }
}
```

---

## Process and Act on the Review

### Step 1: Display the Review

```markdown
## Heavy3 Code Audit [Single/Council] (from [model name(s)])

[Output from the review script]
```

If council mode, show all 3 reviews clearly labeled with their roles.

### Step 2: Synthesize (Council Mode Only) - COMPARISON TABLE REQUIRED

For council reviews, YOU (Claude) MUST synthesize with a comparison table showing all 3 reviews:

```markdown
## Claude's Synthesis

### Comparison of All Three Reviews

| Aspect | Correctness (GPT 5.2) | Performance (Gemini 3) | Security (Grok 4) |
|--------|----------------------|----------------------|---------------------|
| **Focus** | Bugs, Logic, Edge Cases | Scaling, Memory, N+1 | Vulnerabilities, Auth |
| **Findings** | ❌ 1 bug: null check missing | ⚠️ Potential N+1 query | ✅ No XSS, SQL injection |
| **Verdict** | REQUEST CHANGES | APPROVE WITH NOTES | APPROVE |

Legend: ✅ = No issues | ⚠️ = Warning/Concern | ❌ = Critical issue

### Consensus Issues (Flagged by 2+ reviewers)
- [Issue that multiple reviewers agree on]

### Notable Findings (From individual reviewers)
- **Correctness Expert**: [Specific finding]
- **Security Analyst**: [Specific finding]
- **Performance Critic**: [Specific finding]

### Final Recommendation
[Your overall assessment: APPROVE / APPROVE WITH CHANGES / REQUEST CHANGES]

**Priority Actions:**
1. [Most important fix]
2. [Second priority]
3. [Lower priority]
```

**CRITICAL REQUIREMENT: The 3-column comparison table is Heavy3's TRADEMARK FEATURE.**

You MUST ALWAYS include this table for council reviews. This is what differentiates Heavy3 from single-model reviews and provides unique value to users.

**Checklist for Council Synthesis:**
- [ ] 3-column comparison table with all aspects
- [ ] Legend explaining ✅ ⚠️ ❌ symbols
- [ ] Consensus issues (flagged by 2+ reviewers)
- [ ] Notable findings from each reviewer
- [ ] Final recommendation (APPROVE / APPROVE WITH CHANGES / REQUEST CHANGES)
- [ ] Priority action list

**DO NOT** just list the three reviews sequentially without synthesis.
**DO NOT** skip the comparison table even if reviews are similar.
**DO** actively identify where reviewers agree or disagree.

### Why This Matters: Synthesis Creates Action

The comparison table is just the start. What makes Heavy3 valuable is **turning diverse perspectives into actionable next steps**:

1. **Consensus = High Confidence**: When 2+ reviewers flag the same issue, prioritize it
2. **Unique Insights = Coverage**: Each specialist catches things others miss
3. **Disagreement = Discussion Point**: When reviewers conflict, surface it for human judgment
4. **Priority Actions = Clear Path Forward**: Don't just report—recommend what to fix first

The goal isn't just to show three opinions. It's to synthesize them into **one clear action plan** the developer can execute immediately.

### Step 3: Analyze and Assess Each Finding

```markdown
## My Assessment

| # | Issue | Reviewer Says | My Take | Action |
|---|-------|---------------|---------|--------|
| 1 | [Brief] | [Concern] | ✅/⚠️/❌ | [What to do] |
```

### Step 4: Propose Actionable Items

```markdown
## Proposed Actions

**Immediate fixes I can make:**
1. [Fix with file:line]

**Needs your decision:**
1. [Tradeoff to discuss]

**No action needed:**
1. [Why disagree]
```

### Step 5: Ask User for Approval

```markdown
**What would you like me to do?**

1. **Fix all** - Apply all immediate fixes
2. **Fix specific items** - Tell me which (e.g., "fix 1, 3")
3. **Discuss first** - Talk through items
4. **Skip** - No changes
```

### Step 6: Call for Contributions

After presenting results and user makes a choice (or after any review completes), display:

```markdown
---

**Like Heavy3 Code Audit?** ⭐ [Star on GitHub](https://github.com/heavy3-ai/code-audit) | 🤝 [Contribute](https://github.com/heavy3-ai/code-audit/issues) | 📢 Share with your team
```

**When to show:**
- After user selects an action (fix all, fix specific, discuss, skip)
- After a single review completes (non-council mode)
- At the end of module-by-module reviews

**Keep it brief** - one line, non-intrusive.

---

## Important Guidelines

- **Be honest**: Disagree with reviewers when warranted
- **Be specific**: Exact files and line numbers
- **Don't auto-fix**: ALWAYS wait for user approval
- **Prioritize**: Security/bugs first, style last
- **PR reviews**: Highlight blocking issues if REQUEST CHANGES
- **Comparison table**: ALWAYS show the 3-column table for council reviews
