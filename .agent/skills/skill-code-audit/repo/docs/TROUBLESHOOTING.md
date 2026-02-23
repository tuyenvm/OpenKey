# Troubleshooting

## Common Issues

### "OPENROUTER_API_KEY environment variable not set"

**macOS/Linux:**
```bash
# Add to ~/.zshrc or ~/.bashrc
export OPENROUTER_API_KEY="your-key-here"

# Then reload
source ~/.zshrc
```

**Windows PowerShell:**
```powershell
# Set permanently
[System.Environment]::SetEnvironmentVariable("OPENROUTER_API_KEY", "your-key", "User")

# Restart terminal
```

**Windows Command Prompt:**
```cmd
setx OPENROUTER_API_KEY "your-key-here"
:: Restart terminal
```

### "requests module not found"

```bash
pip install requests
```

Or if you have multiple Python versions:
```bash
python3 -m pip install requests
```

### Skill not appearing in Claude Code

1. Verify the skill is in the correct location:
   - macOS/Linux: `~/.claude/skills/h3/`
   - Windows: `%USERPROFILE%\.claude\skills\h3\`

2. Check the SKILL.md file exists and has valid YAML frontmatter

3. Restart Claude Code

### "Permission denied" when running script (macOS/Linux)

```bash
chmod +x ~/.claude/skills/h3/scripts/review.py
chmod +x ~/.claude/skills/h3/scripts/list-free-models.py
```

### API request timeout

The default timeout is 120 seconds. For very large reviews, the model may need more time. This is a limitation of reasoning models.

Try:
1. Reduce the amount of code being reviewed
2. Use a faster model (e.g., `deepseek/deepseek-v3.2`)
3. Set `"reasoning": "medium"` or `"none"` in config.json

### Invalid API key error

1. Verify your key at https://openrouter.ai/keys
2. Check for extra spaces or quotes in the environment variable
3. Ensure you have credits in your OpenRouter account

### Free model not working

Free models rotate on OpenRouter. The model in your config may no longer be available.

1. Run the list utility to see current free models:
   ```bash
   python ~/.claude/skills/h3/scripts/list-free-models.py
   ```

2. Update `free_model` in your config.json with a current free model

3. Check OpenRouter status: https://openrouter.ai/models
