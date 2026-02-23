# Contributing to Heavy3 Code Audit

Thank you for your interest in contributing!

## How to Contribute

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Test locally by installing the skill
5. Commit your changes (`git commit -m 'Add amazing feature'`)
6. Push to the branch (`git push origin feature/amazing-feature`)
7. Open a Pull Request

## Development Setup

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/code-audit.git
cd code-audit

# Symlink the skill for testing (recommended)
mkdir -p ~/.claude/skills
ln -sf "$(pwd)/skill" ~/.claude/skills/h3

# Make scripts executable
chmod +x skill/scripts/*.py

# Install Python dependency
pip install requests

# Make changes and test with /h3 code
```

## Guidelines

- Keep changes focused and minimal
- Test on both macOS/Linux and Windows if possible
- Update documentation for any new features
- Follow existing code style
- Ensure all scripts work cross-platform

## Project Structure

```
code-audit/
├── skill/
│   ├── SKILL.md          # Skill manifest (YAML frontmatter + instructions)
│   ├── config.json       # User configuration
│   └── scripts/
│       ├── review.py     # Lite mode: single model review
│       ├── council_stub.py  # Upgrade prompt for Pro
│       ├── license.py    # License activation
│       └── list-free-models.py  # Free model discovery
├── docs/                 # Installation guides
├── examples/             # Example configurations
└── README.md
```

## Ideas for Contributions

- Improved error messages and diagnostics
- Additional review type templates
- Better context gathering heuristics
- Performance optimizations
- Documentation improvements
