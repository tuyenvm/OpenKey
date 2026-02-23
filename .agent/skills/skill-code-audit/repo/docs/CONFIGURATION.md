# Configuration Reference

Edit `~/.claude/skills/h3/config.json` (or `%USERPROFILE%\.claude\skills\h3\config.json` on Windows):

```json
{
  "model": "moonshotai/kimi-k2.5",
  "free_model": "nvidia/nemotron-3-nano-30b-a3b:free",
  "council_models": {
    "correctness": "openai/gpt-5.2",
    "performance": "google/gemini-3-pro-preview",
    "security": "x-ai/grok-4"
  },
  "reasoning": "high",
  "docs_folder": "documents",
  "max_context": 200000,
  "enable_web_search": true
}
```

## Options

| Option | Description | Default |
|--------|-------------|---------|
| `model` | Model for single-model review | `moonshotai/kimi-k2.5` |
| `free_model` | Model for `--free` flag | `nvidia/nemotron-3-nano-30b-a3b:free` |
| `council_models` | Models for council mode (see below) | GPT 5.2, Gemini 3 Pro, Grok 4 |
| `reasoning` | Reasoning level (always `high` for code review) | `high` |
| `docs_folder` | Where your project documentation lives | `documents` |
| `max_context` | Token limit for reviews | `200000` |
| `enable_web_search` | Enable web search for reviews | `true` |

---

## Web Search (Enabled by Default)

Web search is **enabled by default**. This adds the `:online` suffix and Exa search plugin to API calls, allowing models to reference up-to-date documentation, CVEs, and best practices.

To disable, set `"enable_web_search": false` in config.json.

**Notes:**
- Web search adds ~$0.005 per request (Exa pricing)
- Free models (`:free` suffix) do not support web search

---

## Council Models

Customize which models are used for each council role:

```json
{
  "council_models": {
    "correctness": "openai/gpt-5.2",
    "performance": "google/gemini-3-pro-preview",
    "security": "x-ai/grok-4"
  }
}
```

| Role | Default Model | Focus Area |
|------|---------------|------------|
| `correctness` | `openai/gpt-5.2` | Bugs, logic errors, edge cases, type safety |
| `performance` | `google/gemini-3-pro-preview` | N+1 queries, memory leaks, scaling issues |
| `security` | `x-ai/grok-4` | Vulnerabilities, auth, injection, data exposure |

**Notes:**
- You can override individual roles (unspecified roles use defaults)
- The same models are used for both code and plan reviews (with different prompts)
- Web search (`:online` suffix) is automatically added when `enable_web_search: true`

**Example: Use Claude for correctness:**
```json
{
  "council_models": {
    "correctness": "anthropic/claude-sonnet-4"
  }
}
```

---

## Context Limits

| Mode | Limit | Rationale |
|------|-------|-----------|
| **All modes** | 200K tokens | GPT 5.2 (400K), Gemini 3 Pro (1M), Grok 4 (256K), Kimi K2.5 (256K) - 200K as balanced limit |

The context limit includes: diff + full file contents + documentation + test files. For very large PRs, the skill automatically breaks them into module-by-module reviews.

---

## Available Models (via OpenRouter)

| Mode | Model | Config Value | Price | Notes |
|------|-------|--------------|-------|-------|
| **Single** | Kimi K2.5 | `moonshotai/kimi-k2.5` | ~$0.60/$1.80 per 1M | **DEFAULT** - Excellent quality |
| **Free** | Nemotron Nano | `nvidia/nemotron-3-nano-30b-a3b:free` | $0.00 | Rotating free model |
| **Council** | 3-Model Council | See `council_models` | ~$6.75/$41 per 1M | GPT 5.2 + Gemini 3 Pro + Grok 4 |

> The full `council.py` is in the public repo at `scripts/council.py`. You can read, audit, and modify it - it's MIT licensed. Council models are configurable via `council_models` in config.json.

---

## Free Models

Free tiers rotate on OpenRouter. Use the included utility to find current free models:

```bash
python ~/.claude/skills/h3/scripts/list-free-models.py

# Example output:
# FREE MODELS ON OPENROUTER (newest first)
# ------------------------------------------------------------
#  1. nvidia/nemotron-3-nano-30b-a3b:free                             2025-12-09
#  2. deepseek/deepseek-r1:free          [THINKING]            2025-01-20
#  3. google/gemini-2.0-flash:free       [THINKING]            2025-01-10
```

Or browse manually: [openrouter.ai/models](https://openrouter.ai/models?fmt=table&order=newest)

### Free Tier Limits (OpenRouter)

| Account Status | Daily Limit | Rate Limit |
|---------------|-------------|------------|
| New/Low-credit | 50 requests/day | 20 RPM |
| $10+ credits added | 1000 requests/day | 20 RPM |

**Notes:**
- Adding $10+ credits unlocks higher limits but keeps free model access
- Some free models may have stricter limits
- Heavy usage can trigger rate limiting

---

## Cost Comparison

For a typical small code review (~10K chars / ~2.5K tokens input):

| Mode | Estimated Cost | Quality |
|------|---------------|---------|
| **Single** (Kimi K2.5) | ~$0.005 | Excellent |
| **Free** | $0.00 | Very good |
| **Council** (3-Model) | ~$0.12 | Best (multi-perspective) |

For a larger review (~50K chars / ~12.5K tokens input): Single ~$0.01, Council ~$0.19

**Default is Single** (Kimi K2.5) for excellent quality at minimal cost. Use `--free` for zero cost, or `--council` for multi-model consensus.
