# Install - Windows

## 1. Clone and Link

**PowerShell (Run as Administrator):**

```powershell
git clone https://github.com/heavy3-ai/code-audit.git
cd code-audit
New-Item -ItemType Directory -Force -Path "$env:USERPROFILE\.claude\skills"
New-Item -ItemType SymbolicLink -Path "$env:USERPROFILE\.claude\skills\h3" -Target "$(Get-Location)\skill" -Force
```

**Can't run as admin?** Use copy instead:

```powershell
Copy-Item -Recurse -Force skill "$env:USERPROFILE\.claude\skills\h3"
```

## 2. Install Dependency

```powershell
pip install requests
```

## 3. Add API Key

Get a free key at [openrouter.ai/keys](https://openrouter.ai/keys)

```powershell
"OPENROUTER_API_KEY=sk-or-xxx" | Out-File -Encoding utf8 "$env:USERPROFILE\.claude\skills\h3\.env"
```

## 4. Done

```bash
/h3              # Review changes
/h3 --council    # 3-model review
/h3 pr 123       # Review PR
```

## Update

**Symlink:** `cd code-audit && git pull`

**Copy:** Re-run the copy command after `git pull`

## Troubleshooting

**Python not found?** Reinstall Python and check "Add Python to PATH"

**Symlink failed?** Run PowerShell as Administrator, or use the copy method
