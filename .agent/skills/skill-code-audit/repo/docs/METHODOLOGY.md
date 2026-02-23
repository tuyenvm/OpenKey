# Code Audit Methodology

> **[View Landing Page](https://heavy3.ai/code-audit)** | **[GitHub](https://github.com/heavy3-ai/code-audit)**

This document explains the research-backed principles behind Heavy3 Code Audit's design.

## Table of Contents

1. [The Synthesis Table](#the-synthesis-table-trademark-feature) - Our trademark feature
2. [The Council](#the-council) - Three specialized reviewers
3. [Why Multi-Model?](#why-multi-model) - Research backing
4. [Plan Review](#planarchitecture-review-the-missing-piece) - Review before you code
5. [Context Engineering](#what-context-to-send-for-high-quality-reviews) - What context to send
6. [Context Positioning](#context-positioning-the-lost-in-the-middle-problem) - Lost in the Middle
7. [Large Change Handling](#large-change-handling) - 50+ files, 10K+ lines
8. [Implementation Details](#our-implementation-verified) - Code references
9. [Academic References](#references) - Peer-reviewed sources

---

## The Synthesis Table (Trademark Feature)

After council review, Claude synthesizes all findings into a **3-column comparison table**—our trademark feature that differentiates Heavy3 Code Audit from other tools.

**Example output:**

| Aspect | Correctness (GPT 5.2) | Performance (Gemini 3) | Security (Grok 4) |
|--------|----------------------|----------------------|---------------------|
| **Focus** | Bugs, Logic, Edge Cases | Scaling, Memory, N+1 | Vulnerabilities, Auth |
| **Findings** | ❌ Null check missing | ⚠️ Potential N+1 query | ✅ No issues found |
| **Verdict** | REQUEST CHANGES | APPROVE WITH NOTES | APPROVE |

*Legend: ✅ = No issues | ⚠️ = Warning | ❌ = Critical issue*

**What you get:**
- **Consensus Issues** - Problems flagged by 2+ reviewers (high confidence)
- **Notable Findings** - Unique insights from each specialist
- **Final Recommendation** - APPROVE / APPROVE WITH CHANGES / REQUEST CHANGES
- **Priority Actions** - Ranked list of fixes

**Why this matters:**
- Shows exactly where reviewers agree and disagree
- Provides transparency no other tool offers
- Helps you make informed decisions about which issues to address

---

## The Council

Three specialized reviewers, each with web search:

| Role | Model | Focus | Search |
|------|-------|-------|--------|
| **Correctness Expert** | GPT 5.2 | Bugs, logic errors, edge cases, race conditions | Bing |
| **Performance Critic** | Gemini 3 Pro | N+1 queries, memory leaks, scaling bottlenecks | Exa |
| **Security Analyst** | Grok 4 | Vulnerabilities, auth issues, data exposure | Exa |

**Why Grok 4 for Security?**

Grok 4 was selected as Security Analyst based on independent security benchmarks:

| Benchmark | Score | Source |
|-----------|-------|--------|
| **Kilo AI Exploit Test** | 100% detection on advanced exploits | [Kilo Blog](https://blog.kilo.ai/p/we-tested-6-ai-models-on-3-advanced) |
| **WMDP-Cyber** | 79-81% accuracy (vulnerability detection, reverse engineering) | [xAI Model Card](https://data.x.ai/2025-11-17-grok-4-1-model-card.pdf) |
| **CyBench CTF** | 43% success on 40 capture-the-flag challenges | [xAI Model Card](https://data.x.ai/2025-11-17-grok-4-1-model-card.pdf) |
| **Veracode Security** | 55% secure code generation (mid-tier) | [Veracode](https://www.veracode.com/blog/ai-code-security-october-update) |

The Kilo AI test specifically evaluated Grok 4 on prototype pollution, agentic AI supply-chain attacks, and OS command injection—achieving fix quality scores of 83-85/100 with references to OWASP AI Top 10 and NIST AI RMF standards.

**Three Pillars of Diversity:**

| Pillar | Implementation | Benefit |
|--------|---------------|---------|
| **Different Models** | GPT 5.2 + Gemini 3 Pro + Grok 4 | Different training data, different blind spots |
| **Specialized Roles** | Correctness + Performance + Security | Forces comprehensive coverage |
| **Different Search Sources** | Bing + Exa | Facts verified across independent indexes |

> *You code with Claude. Our council (GPT + Gemini + Grok) catches what Claude misses.*

---

## Why Multi-Model?

**Single models classify code correctness only ~68% of the time.** Research on LLM code review accuracy shows significant room for improvement:

| Study | Finding |
|-------|---------|
| LLM Code Review (2025) | GPT-4o correctly classifies code correctness 68.50% of the time |
| Gemini Flash Study | Gemini 2.0 Flash achieves 63.89% accuracy |
| False Positive Rate | Up to 24.80% of correct code receives incorrect suggestions |

**Every model has different blind spots.** [Sonar's Dec 2025 analysis](https://www.sonarsource.com/blog/new-data-on-code-quality-gpt-5-2-high-opus-4-5-gemini-3-and-more/) of millions of lines of generated code:

| Model | Strength | Weakness |
|-------|----------|----------|
| GPT-5.2 | Cleanest control flow (22/MLOC) | 2x more concurrency bugs (470/MLOC) |
| Gemini 3 Pro | Highest pass rate (81.7%) | 4x more control flow mistakes (200/MLOC) |
| Opus 4.5 | Best overall accuracy | Lowest error rate (55/MLOC) |

**The insight**: The probability of two different architectures hallucinating the *same* bug is significantly lower than one. Using 3 specialized models catches what any single model misses.

---

## What Context to Send for High-Quality Reviews

**Research Finding**: Quality of context matters more than quantity. Well-selected 8K-32K tokens outperforms noisy 100K+ contexts.

Based on LAURA (Zhang et al., IEEE ASE 2025) and CodeRabbit's context engineering:

| Context Type | What to Include | Priority | Our Implementation |
|--------------|-----------------|----------|--------------------|
| **Conversation History** | Original request, approach notes, 3-5 relevant exchanges | Essential | `conversation_context` dict |
| **Code Diff** | Changed lines + 3-5 surrounding lines | Essential | `git diff HEAD` |
| **Commit Messages** | Fine-grained change documentation | Essential | PR body included |
| **File Paths** | Full paths of changed files | Essential | `changed_files` array |
| **Full File Contents** | Complete current state of changed files | High | `file_contents` dict |
| **Related Tests** | Test files matching changed files | High | `test_files` dict |
| **Problem Description** | PR description, issue context | High | `pr_metadata.body` |
| **Documentation** | CLAUDE.md, architecture docs | Medium | `documentation` dict |
| **Cross-File Dependencies** | Files that import/call changed code | For breaking changes | Not yet implemented |

**Key Research Insight**: "Incorporating problem descriptions into prompts consistently improved performance, highlighting the importance of code comments and pull request descriptions." (Evaluating LLMs for Code Review, 2025)

---

## Context Positioning: The "Lost in the Middle" Problem

**The Problem**: LLMs exhibit a U-shaped attention curve where they best process information at the beginning and end of long contexts, while information in the middle is often neglected.

**Research Source**: Liu et al., "Lost in the Middle: How Language Models Use Long Contexts" (TACL 2024)

> "Performance is often highest when relevant information occurs at the beginning or end of the input context, and significantly degrades when models must access relevant information in the middle of long contexts."

**Psychological Parallel**: This mirrors the "serial-position effect" (Ebbinghaus, 1913) in human psychology.

**Our Implementation** (`build_user_message()` in `review.py` and `council.py`):

```
┌─────────────────────────────────────────┐
│ START: Critical context                  │
│   - Developer intent (conversation)      │  ← conversation_context first
│   - PR description (intent)              │  ← pr_metadata second
│   - Code diff (actual changes)           │  ← diff third
├─────────────────────────────────────────┤
│ MIDDLE: Supporting context               │
│   - Full file contents                   │  ← file_contents
│   - Documentation                        │  ← documentation
│   - Test files                           │  ← test_files
├─────────────────────────────────────────┤
│ END: Instructions (in system prompt)     │
│   - Review focus areas                   │  ← SYSTEM_PROMPT
│   - Output format requirements           │
└─────────────────────────────────────────┘
```

---

## Multi-Model Consensus: Why Three Reviewers

**The Problem with Single-Model Review** (Sonar LLM Leaderboard, Dec 2025):

Even top-tier models have significant blind spots. Per million lines of generated code:

| Model | Pass Rate | Weakness | Error Rate |
|-------|-----------|----------|------------|
| GPT-5.2 High | 80.66% | Concurrency errors | 470/MLOC (2x others) |
| Gemini 3 Pro | 81.72% | Control flow mistakes | 200/MLOC (4x best) |
| Claude Sonnet 4.5 | ~77% | Resource management leaks | 195/MLOC |
| Opus 4.5 Thinking | Best pass rate | Control flow | 55/MLOC (lowest) |

**Key Insight**: Each model excels in different areas and fails in different ways. Multi-model consensus catches what any single model misses.

**Research Support**:

1. **Hashgraph-Inspired Consensus** (2025): Treating each model as a "peer" in a distributed consensus system achieves "blockchain-grade properties of consistency and Byzantine fault tolerance."

2. **Disagreement as Data** (Jan 2025): "LLM agents' semantic reasoning similarity robustly differentiates consensus from disagreement and correlates with human coding reliability."

---

## Plan/Architecture Review: The Missing Piece

**No major code review tool effectively reviews plans before implementation.**

Most tools focus on implementation-level code review:
- Syntax errors, linting violations
- Bug detection after code is written
- Code style compliance

Heavy3 Code Audit uniquely offers **pre-implementation validation**:

| Review Type | Focus | Council Roles |
|-------------|-------|---------------|
| **Architecture Design** | Patterns, SOLID, separation of concerns | Design Expert (GPT 5.2) |
| **Plan Feasibility** | Is this approach realistic? What are the risks? | All three reviewers |
| **Scalability Assessment** | Will this scale? What are the bottlenecks? | Scalability Analyst (Gemini 3 Pro) |
| **Security Architecture** | Threat model, attack surface, auth design | Security Architect (Grok 4) |

**Why this matters for AI-assisted coding (vibe coding):**
- The plan determines 80% of success
- Catching design issues early saves massive rework
- Architecture review is where multi-model consensus shines

**Usage:**
```bash
/h3 plan.md           # Review a specific plan file
/h3                   # Smart detection finds plan.md in cwd or ~/.claude/plans/
```

---

## Our Implementation (Verified)

| Documented Practice | Code Location | Status |
|---------------------|---------------|--------|
| Conversation context at very start | `review.py:350-366` | Verified |
| Diff at start of context | `review.py:385-388` | Verified |
| Full file contents included | `review.py:390-393` | Verified |
| Test files included | `review.py:400-403` | Verified |
| Documentation included | `review.py:395-398` | Verified |
| PR metadata for PR reviews | `review.py:373-383` | Verified |
| Context truncation with marker | `review.py:472-473` | Verified |
| Retry with exponential backoff | `review.py:38-70` | Verified |
| Streaming response | `review.py:531-556` | Verified |
| 3-model parallel execution | `council.py:514-540` | Verified |
| Specialized prompts per role | `council.py:73-226` | Verified |
| Web search integration | `council.py:375-377` | Verified |

**JSON Structure Sent to Models**:

```json
{
  "review_type": "code|plan|pr",
  "conversation_context": {
    "original_request": "Add logout button to navbar",
    "approach_notes": "Using existing Button component",
    "relevant_exchanges": [
      {"role": "user", "content": "Can you add a logout button?"},
      {"role": "assistant", "content": "I'll add it using the existing Button component..."}
    ],
    "previous_review_findings": "Prior review suggested adding confirmation dialog"
  },
  "diff": "--- a/src/foo.ts\n+++ b/src/foo.ts\n...",
  "file_contents": {
    "src/foo.ts": "// Full file content..."
  },
  "test_files": {
    "src/foo.test.ts": "// Test content..."
  },
  "documentation": {
    "CLAUDE.md": "// Project guidelines..."
  },
  "pr_metadata": {
    "number": 123,
    "title": "Add feature X",
    "body": "This PR adds..."
  }
}
```

---

## Context Engineering Best Practices

Based on CodeRabbit's "Context Engineering" blog and our implementation:

1. **Deduplicate Redundant Context**: We use a single-pass context builder that avoids duplication.

2. **Respect Token Budgets**:
   - 200K tokens (`max_context`)
   - Graceful truncation with `[... truncated due to length ...]` marker

3. **Include Developer Intent**: PR descriptions and commit context included via `pr_metadata`.

4. **Streaming for UX**: Single mode streams tokens as they arrive.

5. **Progress Indicators**: Council shows completion status for each model.

---

## Large Change Handling

For changes exceeding context limits (50+ files, 10K+ lines):

1. **Detection**: Count files and lines via git stats
2. **Module Grouping**: Break into logical modules
3. **Sequential Review**: Review each module with progress tracking
4. **Cross-Module Summary**: Final pass for dependencies

---

## References

### Peer-Reviewed Academic Work

Published in peer-reviewed venues (conferences, journals):

| Paper | Venue | Year | Key Finding |
|-------|-------|------|-------------|
| [Lost in the Middle](https://arxiv.org/abs/2307.03172) | TACL | 2024 | U-shaped attention; position info at start/end |
| [ReConcile](https://arxiv.org/abs/2309.13007) | ACL | 2024 | Multi-model consensus improves reasoning 11.4% |
| [AI-powered Code Review](https://arxiv.org/abs/2404.18496) | ICSE | 2024 | Multi-agent systems produce consistent outcomes |
| [LAURA](https://arxiv.org/abs/2512.01356) | IEEE ASE | 2025 | Commit messages + file paths + code context = optimal |

### Academic Preprints

ArXiv preprints (not yet peer-reviewed):

| Paper | Year | Key Finding |
|-------|------|-------------|
| [Combining LLMs with Static Analyzers](https://arxiv.org/html/2502.06633v1) | 2025 | Hybrid approach improves accuracy 16% |
| [Disagreement as Data](https://arxiv.org/html/2601.12618) | 2025 | Model disagreement correlates with reliability |
| [Evaluating LLMs for Code Review](https://arxiv.org/html/2505.20206v1) | 2025 | PR descriptions improve performance |

### Industry Analysis

Reports and analysis from industry sources:

| Source | Organization | Key Finding |
|--------|--------------|-------------|
| [LLM Code Quality Analysis](https://www.sonarsource.com/blog/new-data-on-code-quality-gpt-5-2-high-opus-4-5-gemini-3-and-more/) | Sonar | Each model has distinct blind spots in generated code |
| [Context Engineering](https://www.coderabbit.ai/blog/context-engineering-ai-code-reviews) | CodeRabbit | Quality of context > quantity for reviews |
| [Handling Ballooning Context](https://www.coderabbit.ai/blog/handling-ballooning-context-in-the-mcp-era-context-engineering-on-steroids) | CodeRabbit | Strategic context selection for large codebases |
| [LLM Coding Workflow 2026](https://addyosmani.com/blog/ai-coding-workflow/) | Addy Osmani | Multi-model validation best practices |

---

## Summary

| Principle | Research Basis | Implementation |
|-----------|---------------|----------------|
| **Quality > quantity** | Context quality > size | Smart selection of relevant files |
| **Position matters** | Lost in the Middle | Intent first, diff second, docs in middle |
| **Diversity reduces errors** | Multi-model consensus | 3 models + roles + search sources |
| **Include intent** | Problem descriptions | Conversation context + PR body |
| **Specialized focus** | Role-based prompting | Correctness/Performance/Security |
| **Graceful degradation** | Token limits | Truncation markers, module-by-module |
| **Streaming UX** | Response latency | Single streams, Council shows progress |
| **Retry resilience** | Transient failures | Exponential backoff (2s, 4s, 8s) |
