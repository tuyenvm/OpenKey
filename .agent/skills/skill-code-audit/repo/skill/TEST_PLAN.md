# Heavy3 Code Audit - Test Plans

## Quick Manual Test Checklist (~10 min)

### Setup (2 min)

```bash
# Verify setup
ls -la ~/.claude/skills/h3           # Symlink exists
echo $OPENROUTER_API_KEY             # API key set
python -c "import requests"          # Dependency installed
```

### Core Tests (5 min)

| Test | Command | Pass? |
|------|---------|-------|
| Smart detection (changes) | `/h3` | ☐ Confirms, then reviews |
| Staged only | `/h3 --staged` | ☐ |
| Last commit | `/h3 --commit` | ☐ |
| Commit range | `/h3 HEAD~3..HEAD` | ☐ Shows summary, reviews |
| Free model | `/h3 --free` | ☐ |
| Plan explicit | `/h3 plan.md` | ☐ Reviews directly |
| PR review | `/h3 pr <number>` | ☐ |
| No changes | `/h3` (clean repo) | ☐ Asks: commit/range/cancel |
| Streaming | `/h3` | ☐ Text streams word-by-word |

### Council Tests (2 min)

| Test | Command | Pass? |
|------|---------|-------|
| Council | `/h3 --council` | ☐ 3 models run |
| Council order | Check output | ☐ Correctness → Performance → Security |
| Council output | Check synthesis | ☐ 3-column table |

### Error Handling (1 min)

| Test | Action | Pass? |
|------|--------|-------|
| No API key | `unset OPENROUTER_API_KEY && /h3` | ☐ Clear error |
| Invalid range | `/h3 abc..xyz` | ☐ "Invalid commit range" |

### Automated Tests (1 min)

```bash
cd /Users/anh/Documents/Projects/code-audit/skill/scripts
python -m pytest tests/ -v --tb=short
# Expected: 131 passed
```

### Critical Path (if only 3 min)

1. `/h3` - smart detection works
2. `/h3 HEAD~3..HEAD` - commit range works
3. `/h3 --council` - council works
4. `pytest tests/ -v` - automated tests pass

---

## Medium Test Plan (~55 min)

### 1. Environment & Config (5 min)

| ID | Test | Command | Expected | ☐ |
|----|------|---------|----------|---|
| E1 | API key via env | `echo $OPENROUTER_API_KEY` | Key displayed | ☐ |
| E2 | API key via .env | `cat ~/.claude/skills/h3/.env` | Contains key | ☐ |
| E3 | Config valid | `python -m json.tool < ~/.claude/skills/h3/config.json` | Valid JSON | ☐ |
| E4 | Config structure | Check config.json | Has model, enable_web_search | ☐ |
| E5 | Scripts executable | `ls -la ~/.claude/skills/h3/scripts/*.py` | Execute perms | ☐ |

### 2. Smart Detection (10 min)

| ID | Test | Command | Expected | ☐ |
|----|------|---------|----------|---|
| S1 | Changes detected | `/h3` (with changes) | Confirms: "Review all changes?" | ☐ |
| S2 | Staged only detected | `/h3` (only staged) | Confirms: "Review staged changes?" | ☐ |
| S3 | Plan detected (cwd) | `/h3` (plan.md exists) | Confirms: "Review this plan?" | ☐ |
| S4 | Plan detected (plans folder) | `/h3` (file in ~/.claude/plans/) | Confirms plan found | ☐ |
| S5 | No changes, no plan | `/h3` (clean repo) | Asks: commit/range/cancel | ☐ |
| S6 | User selects latest commit | Answer "1" to S5 | Reviews HEAD~1..HEAD | ☐ |
| S7 | User specifies range | Answer "2" then "HEAD~5..HEAD" | Reviews specified range | ☐ |

### 3. Explicit Targets - No Confirmation (8 min)

| ID | Test | Command | Expected | ☐ |
|----|------|---------|----------|---|
| X1 | PR number | `/h3 pr 123` | Executes directly | ☐ |
| X2 | Plan file | `/h3 plan.md` | Executes directly | ☐ |
| X3 | Any .md file | `/h3 myplan.md` | Treats as plan, executes directly | ☐ |
| X4 | Commit range | `/h3 HEAD~3..HEAD` | Shows summary, executes directly | ☐ |
| X5 | Staged flag | `/h3 --staged` | Executes directly | ☐ |
| X6 | Commit flag | `/h3 --commit` | Executes directly | ☐ |

### 4. Commit Range Support (5 min)

| ID | Test | Command | Expected | ☐ |
|----|------|---------|----------|---|
| R1 | Last N commits | `/h3 HEAD~3..HEAD` | Reviews 3 commits | ☐ |
| R2 | Specific range | `/h3 abc123..def456` | Reviews between commits | ☐ |
| R3 | To HEAD | `/h3 abc123..HEAD` | Reviews from commit to HEAD | ☐ |
| R4 | Invalid range | `/h3 invalid..range` | "Invalid commit range" error | ☐ |
| R5 | Range summary | `/h3 HEAD~3..HEAD` | Shows commit table before review | ☐ |

### 5. Scope Modifiers (5 min)

| ID | Test | Command | Expected | ☐ |
|----|------|---------|----------|---|
| L1 | Staged only | `/h3 --staged` | Only git diff --cached | ☐ |
| L2 | Last commit | `/h3 --commit` | Reviews HEAD~1..HEAD | ☐ |
| L3 | Free model | `/h3 --free` | Uses free model | ☐ |
| L4 | No staged changes | `/h3 --staged` (nothing staged) | "No staged changes" error | ☐ |
| L5 | Streaming | `/h3` | Text appears incrementally | ☐ |

### 6. Plan & PR Review (5 min)

| ID | Test | Command | Expected | ☐ |
|----|------|---------|----------|---|
| P1 | Plan with path | `/h3 plan path/to/plan.md` | Reviews specified file | ☐ |
| P2 | Plan shorthand | `/h3 myplan.md` | Reviews as plan | ☐ |
| P3 | PR valid | `/h3 pr <real-pr-number>` | Fetches & reviews PR | ☐ |
| P4 | PR invalid | `/h3 pr 99999` | Error message | ☐ |

### 7. Model Override (5 min)

| ID | Test | Command | Expected Model | ☐ |
|----|------|---------|----------------|---|
| M1 | Shortcut: kimi | `/h3 --model kimi` | moonshotai/kimi-k2.5 | ☐ |
| M2 | Shortcut: gpt | `/h3 --model gpt` | openai/gpt-5.2 | ☐ |
| M3 | Shortcut: deepseek | `/h3 --model deepseek` | deepseek/deepseek-v3.2 | ☐ |
| M4 | Shortcut: free | `/h3 --model free` | Config free_model | ☐ |
| M5 | Full slug | `/h3 --model anthropic/claude-3-haiku` | Exact model | ☐ |

### 8. Council Mode (10 min)

| ID | Test | Command | Expected | ☐ |
|----|------|---------|----------|---|
| C1 | Council code | `/h3 --council` | 3 models in parallel | ☐ |
| C2 | Council plan | `/h3 plan.md --council` | Architecture review | ☐ |
| C3 | Council PR | `/h3 pr <num> --council` | PR with 3 models | ☐ |
| C4 | Council range | `/h3 HEAD~3..HEAD --council` | Range with 3 models | ☐ |
| C5 | Progress display | `/h3 --council` | "[1/3] ✓ Correctness Expert" | ☐ |
| C6 | Council order | Check output | Correctness → Performance → Security | ☐ |
| C7 | 3-column table | Check output | Comparison table shown | ☐ |

### 9. Error Handling (5 min)

| ID | Test | Scenario | Expected | ☐ |
|----|------|----------|----------|---|
| E1 | No API key | Unset key, no .env | Clear error with instructions | ☐ |
| E2 | Invalid API key | Set wrong key | "Invalid API key" | ☐ |
| E3 | Retry on timeout | Slow response | Retries with backoff (2s,4s,8s) | ☐ |
| E4 | Empty response | API returns empty | "Empty content" error | ☐ |
| E5 | Invalid commit range | `/h3 bad..range` | "Invalid commit range" | ☐ |

### 10. Context Handling (5 min)

| ID | Test | Scenario | Expected | ☐ |
|----|------|----------|----------|---|
| X1 | Small diff | <10K tokens | Full context sent | ☐ |
| X2 | Truncation | Large diff | Shows [... truncated ...] | ☐ |
| X3 | Large change | >50 files | Module selection UI | ☐ |

### 11. Automated Tests (2 min)

```bash
cd /Users/anh/Documents/Projects/code-audit/skill/scripts
python -m pytest tests/ -v --tb=short
```

| File | Expected | Actual | ☐ |
|------|----------|--------|---|
| test_context.py | 20 pass | | ☐ |
| test_review.py | 32 pass | | ☐ |
| test_streaming.py | 16 pass | | ☐ |
| test_council.py | 23 pass | | ☐ |
| test_free_models.py | 40 pass | | ☐ |
| **Total** | **131 pass** | | ☐ |

---

## Test Coverage Summary

### Python Files and Test Coverage

| Script | Test File | Tests | Coverage Areas |
|--------|-----------|-------|----------------|
| `review.py` | `test_review.py` | 32 | API calls, config, model resolution, cost estimation |
| `council.py` | `test_council.py` | 23 | Models, prompts, parallel execution, web search |
| `list-free-models.py` | `test_free_models.py` | 40 | Model detection, date parsing, API fetch |
| (context building) | `test_context.py` | 20 | Message building, truncation, prompts |
| (streaming) | `test_streaming.py` | 16 | SSE parsing, malformed JSON, output |

### Test Types

- **Unit Tests**: 131 (all in `tests/` directory)
- **Manual Tests**: 57 (across 11 sections)
- **Integration Tests**: Included in manual checklist

---

## Summary

| Section | Tests | Time |
|---------|-------|------|
| Environment | 5 | 5 min |
| Smart Detection | 7 | 10 min |
| Explicit Targets | 6 | 8 min |
| Commit Range | 5 | 5 min |
| Scope Modifiers | 5 | 5 min |
| Plan/PR | 4 | 5 min |
| Model Override | 5 | 5 min |
| Council | 7 | 10 min |
| Error Handling | 5 | 5 min |
| Context | 3 | 5 min |
| Automated | 6 | 2 min |
| **Total** | **58** | **~55 min** |

---

## Sign-Off

| Tester | Date | Pass/Fail | Notes |
|--------|------|-----------|-------|
| | | | /58 |
