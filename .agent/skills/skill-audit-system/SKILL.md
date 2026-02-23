---
name: system-auditor
description: The Universal Review Engine. Audits systems through multiple lenses (Operational, Structural, Business, Semantic) to ensure they are not just 'running' but 'meaningful' and 'compliant'.
version: 2.0.0
whitelist: [run_command, view_file, grep_search]
---

# System Auditor (The Governance Nexus)

> **Philosophy**: "Kaizen (改善) - Continuous Improvement."
> **Motto**: "There is no best, only better."
> **Origin**: Standing on the shoulders of giants (Borrowed Intelligence Layers from `code-review`).

## Purpose
To act as the **Meta-Auditor** of the system. Instead of reinventing the wheel, it acts as a central hub that:
1.  **Directly Audits** Governance & Operations (its core specialty).
2.  **Orchestrates** deep-dives by delegating to specialized skills (`code-review`, `security-scanner`).
3.  **Synthesizes** findings into a holistic Health Report.

## The Fractal Orchestration Strategy

> **Pattern**: "I do not work. I make sure work happens."
> **Mechanism**: The Auditor acts as a **Micro-Orchestrator**, dispatching "Sharp Spearhead" skills to do the heavy lifting.

### 1. Delegation Matrix (The "Squad")

| Aspect | Delegate Skill | Command |
| :--- | :--- | :--- |
| **Law & Order** | `governance-guard` | `ag skill exec governance-guard check <target>` |
| **Meaning & Value** | `value-inspector` | `ag skill exec value-inspector inspect <target>` |
| **Code Quality** | `code-review` | `ag skill exec code-review review <target>` |
| **Evolution** | `system-enhancer` | `ag skill exec system-enhancer enhance <target>` |

### 2. The Circuit Breaker (The "Break Point")

> **Problem**: "Skill A calls Skill B, which calls Skill A..." (Infinite Loop).
> **Solution**: The **Call Stack Governor**.

**The Break Point Definition:**
The chain STOPS immediately if:
1.  **Depth Limit (TTL)**: Current Depth > 3. (Captain -> Specialist -> Worker -> STOP).
2.  **Cycle Detected**: Target Skill is already in the `call_stack`.
3.  **Low Information Gain**: The previous step returned "No new issues".

### 3. The Orchestration Loop (Safe Mode)

1.  **Intake**: Receive Request + `call_stack`.
2.  **Circuit Check**: 
    *   `if "system-auditor" in call_stack`: **ABORT (Cycle Detected)**.
    *   `if len(call_stack) > 3`: **ABORT (Depth Exceeded)**.
3.  **Dispatch**: Trigger the Squad using `ag skill exec`.
4.  **Synthesize**: Collect reports.

### 3. Recursive Self-Correction
Before finalizing, the Auditor asks `system-enhancer`:
*   "Based on these findings, what is the one thing we should improve?"

## Usage Patterns

### Full Spectrum Audit (The "Big Scan")
```bash
# Conceptual: Auditor triggers the swarm
ag skill exec system-auditor audit --mode full <ticket_id>
```


## Usage Patterns

### Audit a specific Ticket (Micro-Kaizen)
```bash
python .agent/skills/system-auditor/scripts/audit_ticket.py <ticket_id>
```

### Audit the entire System Health (Macro-Kaizen)
```bash
python .agent/skills/system-auditor/scripts/audit_system.py
```

## Recursive Self-Reflection
*   "Am I creating value or just noise?"
*   "Can I leverage `code-review` for this specific file instead of guessing?"


## Usage Patterns

### Audit a specific Ticket
```bash
python .agent/skills/system-auditor/scripts/audit_ticket.py <ticket_id>
```

### Audit the entire System Health
```bash
python .agent/skills/system-auditor/scripts/audit_system.py
```

## Self-Reflection Protocol
When running this skill, you must recursively ask:
*   "Am I checking the box, or checking the meaning?"
*   "If I were a new employee, would I understand this Ticket?"
