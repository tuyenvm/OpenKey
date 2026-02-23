---
name: captain-verifier
description: Captain skill that orchestrates verification workflows. Validates implementations, runs tests, checks compliance, and ensures quality gates are met before completion.
version: 1.0.0
type: captain
role: The Validator
triggers:
  - ag flow verify
  - ag verify
  - verification request
  - evolution verify
  - ag evolve verify
dispatches_to:
  - skill-systematic-debugging
  - skill-code-review
  - skill-verification-before-completion
---

<!--
 ANTIGRAVITY DNA
 @mission: "Captain Verifier - The Validator"
 @owner: "Ops/Governance"
 @scope: "Quality assurance and verification orchestration"
 @risk: "Low"
 @rfc: "RFC-001"
-->

# Captain Verifier (The Validator)

```text
 __     __        _  __ _
 \ \   / /__ _ __(_)/ _(_) ___ _ __
  \ \ / / _ \ '__| | |_| |/ _ \ '__|
   \ V /  __/ |  | |  _| |  __/ |
    \_/ \___|_|  |_|_| |_|\___|_|
       CAPTAIN - The Validator
```

> **Philosophy**: "Trust, but verify. Then verify again."
> **Motto**: "No assumption survives contact with verification."
> **Role**: The Validator - Ensures quality and correctness before completion.

## Purpose

Captain Verifier is the **quality gate orchestrator** of the Pentagon Force. While other captains focus on planning or building, Verifier focuses on **validation**. It takes completed work and ensures it meets all quality standards, passes tests, and complies with governance requirements.

```
                    ┌─────────────────────┐
                    │  CAPTAIN VERIFIER   │
                    │    The Validator    │
                    └─────────┬───────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
   ┌─────────┐          ┌─────────┐          ┌─────────┐
   │  TEST   │          │ REVIEW  │          │ COMPLY  │
   │ Runner  │          │ Quality │          │ Check   │
   └─────────┘          └─────────┘          └─────────┘
```

## Trigger Conditions

Captain Verifier is invoked when:

| Trigger | Command | Context |
|---------|---------|---------|
| Flow Verify | `ag flow verify` | Primary trigger - verifies completed work |
| Manual Verify | `ag verify <target>` | Verify specific artifact |
| Pre-Completion | Automatic | Gate before marking task DONE |
| Evolution Verify | `ag evolve verify` | Verify evolution integrations |

## Delegation Matrix

> **Pattern**: "I orchestrate verification. Specialists validate. I synthesize results."

| Situation | Delegate To | Command | When |
|-----------|-------------|---------|------|
| Test failures | `skill-systematic-debugging` | `ag skill exec systematic-debugging` | Test suite failures |
| Code quality | `skill-code-review` | `ag skill exec code-review` | Pre-completion gate |
| Completeness | `skill-verification-before-completion` | `ag skill exec verification-before-completion` | Final check |

## Verification Checklist

### Standard Verification

```yaml
verification_gates:
  - name: tests_pass
    command: npm test
    required: true

  - name: lint_clean
    command: npm run lint
    required: true

  - name: build_success
    command: npm run build
    required: true

  - name: type_check
    command: npx tsc --noEmit
    required: true

  - name: documentation
    check: README updated if API changed
    required: false
```

### Verification Flow

```
┌─────────────────────────────────────────────────────────────┐
│           VERIFICATION FLOW                                  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. RECEIVE ────► Verification request                      │
│       │                                                     │
│       ▼                                                     │
│  2. IDENTIFY ───► Determine verification scope              │
│       │           - What needs verification?                │
│       │           - Which gates apply?                      │
│       ▼                                                     │
│  3. EXECUTE ────► Run verification checks                   │
│       │           - Tests, lint, build, types               │
│       │           - Custom project checks                   │
│       ▼                                                     │
│  4. ANALYZE ────► Interpret results                         │
│       │           - Pass/fail determination                 │
│       │           - Root cause for failures                 │
│       ▼                                                     │
│  5. REPORT ─────► Generate verification report              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Circuit Breaker Logic

```yaml
circuit_breaker:
  max_depth: 3          # Maximum delegation depth
  max_retries: 2        # Verification retry limit
  timeout_ms: 180000    # 3 minute timeout per check
```

## Output Artifacts

### Verification Report

```markdown
# Verification Report

## Summary
- **Status**: PASS | FAIL | PARTIAL
- **Timestamp**: {ISO8601}
- **Target**: {what was verified}

## Gates

| Gate | Status | Details |
|------|--------|---------|
| Tests | PASS | 42/42 passed |
| Lint | PASS | No issues |
| Build | PASS | Compiled successfully |
| Types | PASS | No errors |

## Issues Found
{list of any issues}

## Recommendation
{PROCEED | FIX_REQUIRED | ESCALATE}
```

---

## Evolution Pipeline Integration

> **Trigger**: `ag evolve verify`, `evolution verify`

When invoked for evolution verification, the Captain Verifier operates in **Evolution Mode** to validate newly integrated capabilities from the EXECUTE phase.

### Evolution Input: EXECUTION_RESULTS.yml

```yaml
evolution_input:
  source: EXECUTION_RESULTS.yml
  location: records/evolution/EXECUTION_RESULTS.yml
  required_fields:
    - candidates[]
    - candidates[].status
    - candidates[].artifacts_created[]
```

### Evolution Verification Checks

For each integrated capability:

```
┌─────────────────────────────────────────────────────────────┐
│           EVOLUTION VERIFY FLOW                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. DNA HEADERS ─► Validate ANTIGRAVITY DNA present         │
│       │            - @mission defined                       │
│       │            - @owner defined                         │
│       │            - @scope defined                         │
│       ▼                                                     │
│  2. SKILL SCHEMA ► Validate SKILL.md structure              │
│       │            - YAML frontmatter valid                 │
│       │            - Required fields present                │
│       │            - Triggers defined                       │
│       ▼                                                     │
│  3. INTEGRATION ─► Verify system wiring                     │
│       │            - COGNITIVE_TREE.yaml updated            │
│       │            - No orphan references                   │
│       │            - Skill discoverable                     │
│       ▼                                                     │
│  4. REGRESSION ──► Check existing functionality             │
│       │            - No broken skills                       │
│       │            - No conflicts detected                  │
│       ▼                                                     │
│  5. SOP COMPLIANCE► Verify governance rules                 │
│       │            - Naming conventions                     │
│       │            - File locations                         │
│       │            - Documentation requirements             │
│       ▼                                                     │
│  6. EMIT ────────► Output VERIFICATION_REPORT.yml           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Evolution-Specific Checks

#### 1. DNA Headers Check

```yaml
dna_headers_check:
  required:
    - "@mission"
    - "@owner"
    - "@scope"
  optional:
    - "@risk"
    - "@rfc"
  validation: |
    Each header must be non-empty and meaningful
    @mission should describe the skill's purpose
    @owner should reference a valid team/area
```

#### 2. Skill Schema Validation

```yaml
skill_schema:
  frontmatter:
    required:
      - name: string
      - description: string
      - version: semver
      - type: enum[skill, captain, passive]
      - triggers: array[string]
    optional:
      - role: string
      - dispatches_to: array[string]
      - whitelist: array[string]
  content:
    required:
      - Purpose/Mission section
      - Usage/Trigger section
    recommended:
      - Examples section
      - Error handling section
```

#### 3. Integration Test Placeholder

```yaml
integration_test:
  method: skill_discovery
  command: ag skill list | grep {skill-name}
  expected: Skill appears in catalog
  fallback: Manual verification
```

#### 4. SOP Compliance

```yaml
sop_compliance:
  naming:
    pattern: "skill-{descriptive-name}"
    case: kebab-case
  location:
    path: ".agent/skills/{skill-name}/SKILL.md"
  documentation:
    required: SKILL.md exists
    recommended: Examples included
```

### Evolution Rollback Signal

If verification fails:

```yaml
rollback_signal:
  trigger: verification_failure
  action:
    - Emit ROLLBACK_REQUIRED to captain-builder
    - Document failure reason
    - Preserve failed artifacts for analysis
  report_to:
    - captain-builder (for rollback)
    - LEARN phase (for feedback loop)
```

### Evolution Output: VERIFICATION_REPORT.yml

```yaml
# Output for LEARN phase
verification_report:
  cycle_id: {evolution_cycle_id}
  timestamp: {ISO8601}
  overall_status: PASS | FAIL | PARTIAL
  candidates:
    - name: {skill-name}
      status: VERIFIED | FAILED | NEEDS_REVIEW
      checks:
        dna_headers: PASS | FAIL
        skill_schema: PASS | FAIL
        integration: PASS | FAIL
        regression: PASS | FAIL
        sop_compliance: PASS | FAIL
      issues: []
      recommendation: ACCEPT | ROLLBACK | MANUAL_REVIEW
```

### Evolution Example Invocation

```bash
# Trigger evolution verification
ag evolve verify

# With specific execution results
ag evolve verify --input records/evolution/EXECUTION_RESULTS.yml

# Verbose mode
ag evolve verify --verbose

# Output location
# -> records/evolution/VERIFICATION_REPORT.yml
```

---

## Related Documentation

- `.agent/governance/CAPTAINS.md` - Pentagon Force hierarchy
- `.agent/skills/skill-systematic-debugging/SKILL.md` - Debug failures
- `.agent/skills/skill-code-review/SKILL.md` - Quality review
- `.agent/skills/skill-verification-before-completion/SKILL.md` - Completeness check

---

> **"Quality is not an act, it is a habit. Verification makes that habit visible."**
