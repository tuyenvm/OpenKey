# Install - macOS / Linux

## 1. Clone and Link

```bash
git clone https://github.com/heavy3-ai/code-audit.git
cd code-audit
mkdir -p ~/.claude/skills
ln -sf "$(pwd)/skill" ~/.claude/skills/h3
```

## 2. Install Dependency

```bash
pip install requests
```

## 3. Add API Key

Get a free key at [openrouter.ai/keys](https://openrouter.ai/keys)

```bash
echo 'OPENROUTER_API_KEY=sk-or-xxx' > ~/.claude/skills/h3/.env
```

## 4. Done

```bash
/h3              # Review changes
/h3 --council    # 3-model review
/h3 pr 123       # Review PR
```

## Update

```bash
cd /path/to/code-audit && git pull
```

## Uninstall

```bash
rm ~/.claude/skills/h3
```
