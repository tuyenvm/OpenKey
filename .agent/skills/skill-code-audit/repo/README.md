# Heavy3 Code Audit (`/h3`)

> **[View Landing Page](https://heavy3.ai/code-audit)** | **[heavy3.ai](https://heavy3.ai)**

Multi-model consensus code review for Claude Code, Cursor, Codex CLI, Gemini CLI, Antigravity.

**100% Free and Open Source** (MIT License) - BYOK via OpenRouter.

## Modes

| Mode | Cost | Command |
|------|------|---------|
| Free | $0.00 | `/h3 --free` |
| Single | ~$0.01 | `/h3` |
| Council (3 models) | ~$0.10 | `/h3 --council` |

## Quick Start

```bash
git clone https://github.com/heavy3-ai/code-audit.git
cd code-audit && mkdir -p ~/.claude/skills && ln -sf "$(pwd)/skill" ~/.claude/skills/h3
pip install requests
echo 'OPENROUTER_API_KEY=your-key-here' > ~/.claude/skills/h3/.env
```

Get your free API key at [openrouter.ai/keys](https://openrouter.ai/keys)

**Windows?** See [docs/INSTALL-WINDOWS.md](docs/INSTALL-WINDOWS.md)

## Usage

```bash
/h3                # Smart detect: uncommitted changes → plan → ask
/h3 --council      # 3-model review (GPT + Gemini + Grok)
/h3 --free         # Free model
/h3 pr 123         # Review PR #123
/h3 plan.md        # Review a plan file
/h3 HEAD~3..HEAD   # Review last 3 commits
```

## Council Mode

3 specialized reviewers with web search:

| Role | Model | Focus |
|------|-------|-------|
| Correctness | GPT 5.2 | Bugs, logic, edge cases |
| Performance | Gemini 3 Pro | N+1, memory, scaling |
| Security | Grok 4 | Vulnerabilities, auth |

## Docs

- [Methodology](docs/METHODOLOGY.md) - How it works
- [Configuration](docs/CONFIGURATION.md) - Options
- [Troubleshooting](docs/TROUBLESHOOTING.md) - Common issues

## License

MIT License - see [LICENSE](LICENSE)
