---
name: captain-builder
description: The Executor - Coordinates code implementation work. Orchestrates parallel execution, handles errors via delegation, and ensures atomic task completion with circuit breaker protection.
version: 1.1.0
type: captain
role: The Executor
triggers:
  - ag flow execute
  - ag task start
  - ag epic execute
  - evolution execute
  - ag evolve execute
whitelist: [run_command, view_file, grep_search, edit_file, create_file]
---

<!--
 ANTIGRAVITY DNA
 @mission: "Captain Builder - The Executor Orchestrator"
 @owner: "Ops/Governance"
 @scope: "Code implementation coordination and execution"
 @risk: "Medium"
 @rfc: "RFC-001"
-->

# Captain Builder (The Executor)

> **Philosophy**: "Plan the work, work the plan, verify the outcome."
> **Motto**: "Execute with precision, fail with grace, recover with speed."
> **Role**: The Executor - Turns plans into reality through coordinated implementation.

## Purpose

Captain Builder is the **implementation orchestrator** of the Pentagon Force. While other captains focus on planning, auditing, or reviewing, Builder focuses on **execution**. It takes approved plans and coordinates their implementation across workers, handling parallelization, error recovery, and completion verification.

```
                    ┌─────────────────────┐
                    │   CAPTAIN BUILDER   │
                    │     The Executor    │
                    └─────────┬───────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
   ┌─────────┐          ┌─────────┐          ┌─────────┐
   │ SPAWN   │          │ MONITOR │          │ RECOVER │
   │ Workers │          │ Progress│          │ Errors  │
   └─────────┘          └─────────┘          └─────────┘
```

## Trigger Conditions

Captain Builder is invoked when:

| Trigger | Command | Context |
|---------|---------|---------|
| Flow Execute | `ag flow execute` | Primary trigger - executes approved plans |
| Task Start | `ag task start <id>` | Single task execution |
| Epic Execute | `ag epic execute <id>` | Multi-task epic execution |
| Resume | `ag flow resume` | Resume interrupted execution |

## Delegation Matrix (The Squad)

> **Pattern**: "I coordinate. Specialists execute. Debugger recovers."

| Situation | Delegate To | Command | When |
|-----------|-------------|---------|------|
| Parallel Tasks | `skill-orchestrator` | `ag skill exec orchestrator spawn` | Multiple independent tasks |
| Implementation Error | `skill-captain-debugger` | `ag skill exec captain-debugger diagnose` | Any execution failure |
| Test Failure | `skill-systematic-debugging` | `ag skill exec systematic-debugging` | Test suite failures |
| Code Quality Issue | `skill-code-review` | `ag skill exec code-review` | Pre-completion gate |
| Build Failure | `skill-captain-debugger` | `ag skill exec captain-debugger --type build` | Compilation/build errors |

### Delegation Flow

```
┌─────────────────────────────────────────────────────────────┐
│  BUILDER DELEGATION FLOW                                    │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. Receive execution plan                                  │
│     │                                                       │
│     ▼                                                       │
│  2. Analyze task dependencies                               │
│     │                                                       │
│     ├──[Parallel tasks?]──► Dispatch to ORCHESTRATOR        │
│     │                                                       │
│     ▼                                                       │
│  3. Execute task(s)                                         │
│     │                                                       │
│     ├──[Error?]──► Dispatch to CAPTAIN-DEBUGGER             │
│     │              │                                        │
│     │              └──► Receive fix ──► Retry (max 3)       │
│     │                                                       │
│     ▼                                                       │
│  4. Verify completion                                       │
│     │                                                       │
│     └──► Report status                                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Circuit Breaker Logic

> **Problem**: Infinite loops, recursive failures, resource exhaustion.
> **Solution**: The Circuit Breaker Governor.

### Break Conditions

The execution chain **STOPS IMMEDIATELY** if:

| Condition | Threshold | Action |
|-----------|-----------|--------|
| **Depth Limit (TTL)** | `depth > 3` | ABORT - Max nesting exceeded |
| **Cycle Detected** | Skill in `call_stack` | ABORT - Recursive loop detected |
| **Retry Exhausted** | `retries >= 3` | ESCALATE - Human intervention required |
| **Time Budget** | `elapsed > budget` | PAUSE - Checkpoint and yield |
| **Token Budget** | `tokens > limit` | PAUSE - Save state and yield |

### Circuit Breaker Implementation

```typescript
interface CircuitBreaker {
  maxDepth: 3;           // Captain -> Specialist -> Worker -> STOP
  maxRetries: 3;         // Per-task retry limit
  timeBudgetMs: 300000;  // 5 minute default per task
  tokenBudget: 50000;    // Token limit per execution cycle
}

function checkCircuit(state: ExecutionState): CircuitStatus {
  // 1. Depth Check
  if (state.callStack.length > CircuitBreaker.maxDepth) {
    return { status: 'OPEN', reason: 'DEPTH_EXCEEDED', action: 'ABORT' };
  }

  // 2. Cycle Detection
  if (state.callStack.includes('captain-builder')) {
    return { status: 'OPEN', reason: 'CYCLE_DETECTED', action: 'ABORT' };
  }

  // 3. Retry Exhaustion
  if (state.retryCount >= CircuitBreaker.maxRetries) {
    return { status: 'OPEN', reason: 'RETRIES_EXHAUSTED', action: 'ESCALATE' };
  }

  // 4. Resource Limits
  if (state.elapsed > CircuitBreaker.timeBudgetMs) {
    return { status: 'HALF_OPEN', reason: 'TIME_BUDGET', action: 'CHECKPOINT' };
  }

  return { status: 'CLOSED', reason: null, action: 'CONTINUE' };
}
```

## Orchestration Loop Pattern

### Main Execution Loop

```typescript
async function executeFlow(plan: ExecutionPlan): Promise<ExecutionResult> {
  const state: ExecutionState = {
    callStack: ['captain-builder'],
    retryCount: 0,
    startTime: Date.now(),
    checkpoints: [],
  };

  // Phase 1: Pre-flight Checks
  const circuitStatus = checkCircuit(state);
  if (circuitStatus.status === 'OPEN') {
    return abort(circuitStatus.reason);
  }

  // Phase 2: Task Decomposition
  const tasks = decomposePlan(plan);
  const { parallel, sequential } = classifyTasks(tasks);

  // Phase 3: Parallel Execution (if applicable)
  if (parallel.length > 1) {
    await delegateTo('skill-orchestrator', {
      tasks: parallel,
      callStack: state.callStack,
    });
  }

  // Phase 4: Sequential Execution with Error Handling
  for (const task of sequential) {
    try {
      await executeTask(task, state);
      checkpoint(state, task);
    } catch (error) {
      state.retryCount++;

      // Delegate to debugger
      const fix = await delegateTo('skill-captain-debugger', {
        error,
        task,
        callStack: state.callStack,
      });

      if (fix.resolved) {
        await applyFix(fix);
        await retryTask(task, state);
      } else {
        // Circuit breaker check
        const status = checkCircuit(state);
        if (status.action === 'ESCALATE') {
          return escalateToHuman(error, state);
        }
      }
    }
  }

  // Phase 5: Completion Verification
  return verifyAndReport(state);
}
```

### Error Handling Strategy

```
┌─────────────────────────────────────────────────────────────┐
│  ERROR HANDLING MATRIX                                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Error Type        │ Handler              │ Max Retries     │
│  ──────────────────┼──────────────────────┼───────────────  │
│  Syntax Error      │ captain-debugger     │ 1               │
│  Test Failure      │ systematic-debugging │ 3               │
│  Build Failure     │ captain-debugger     │ 2               │
│  Timeout           │ self (retry)         │ 2               │
│  Resource Conflict │ orchestrator         │ 1               │
│  Unknown           │ ESCALATE             │ 0               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Execution States

| State | Description | Next Actions |
|-------|-------------|--------------|
| `IDLE` | No active execution | Await trigger |
| `PLANNING` | Decomposing plan | Execute or delegate |
| `EXECUTING` | Active task execution | Monitor, handle errors |
| `BLOCKED` | Waiting on dependency | Resume when unblocked |
| `RECOVERING` | Handling error | Retry or escalate |
| `CHECKPOINTING` | Saving state | Resume or yield |
| `COMPLETE` | All tasks done | Report and cleanup |
| `FAILED` | Unrecoverable error | Escalate to human |

## Usage Patterns

### Execute a Flow

```bash
# Primary invocation
ag flow execute

# With specific plan
ag flow execute --plan records/active/execution-plan.yml

# Resume interrupted execution
ag flow resume --checkpoint records/state/checkpoint.yml
```

### Execute Single Task

```bash
# Execute specific ticket
ag task start TK-20260131-001

# With timeout override
ag task start TK-20260131-001 --timeout 600000
```

### Execute Epic

```bash
# Execute all tasks in epic
ag epic execute EPIC-001

# Parallel mode
ag epic execute EPIC-001 --parallel
```

## Integration Points

### Input Sources

- `records/active/execution-plan.yml` - Current execution plan
- `records/state/flow_state.yml` - Flow state machine
- `records/tickets/active/` - Active ticket queue

### Output Artifacts

- `records/state/checkpoint.yml` - Execution checkpoints
- `records/logs/execution/` - Execution logs
- `records/metrics/builder/` - Performance metrics

## Self-Reflection Protocol

Before completing any execution:

1. **Completeness Check**: "Did I execute ALL tasks in the plan?"
2. **Quality Gate**: "Did all tests pass? Did code review approve?"
3. **State Consistency**: "Is the codebase in a valid state?"
4. **Documentation**: "Did I update relevant docs/comments?"

## Related Documentation

- `.agent/governance/CAPTAINS.md` - Pentagon Force hierarchy
- `.agent/skills/skill-orchestrator/SKILL.md` - Parallel execution
- `.agent/skills/skill-systematic-debugging/SKILL.md` - Error diagnosis
- `ops/ag-cli/cli/flow.ts` - Flow CLI implementation
- `ops/ag-ts/core/workflow.ts` - Workflow engine

---

## Evolution Pipeline Integration

> **Trigger**: `ag evolve execute`, `evolution execute`

When invoked for evolution execution, the Captain Builder operates in **Evolution Mode** to integrate new capabilities from the INTEGRATION_PLAN.md produced by captain-architect.

### Evolution Input: INTEGRATION_PLAN.md

```yaml
evolution_input:
  source: INTEGRATION_PLAN.md
  location: records/evolution/INTEGRATION_PLAN.md
  required_fields:
    - candidates[]
    - candidates[].integration_steps[]
    - candidates[].source_url
    - candidates[].type  # skill | mcp | pattern
```

### Evolution Execution Steps

For each candidate in the integration plan:

```
┌─────────────────────────────────────────────────────────────┐
│           EVOLUTION EXECUTE FLOW                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. CLONE ──────► Fetch source repository                   │
│       │           - git clone <source_url> to temp dir      │
│       │           - Verify clone success                    │
│       ▼                                                     │
│  2. EXTRACT ────► Extract relevant code/docs                │
│       │           - Identify SKILL.md or equivalent         │
│       │           - Extract patterns/code snippets          │
│       │           - Preserve attribution                    │
│       ▼                                                     │
│  3. CREATE ─────► Create skill scaffold                     │
│       │           - mkdir .agent/skills/{skill-name}/       │
│       │           - Create SKILL.md with DNA headers        │
│       │           - Add metadata and triggers               │
│       ▼                                                     │
│  4. WIRE ───────► Integrate with system                     │
│       │           - Update COGNITIVE_TREE.yaml              │
│       │           - Register in skill catalog               │
│       │           - Add to appropriate category             │
│       ▼                                                     │
│  5. CHECKPOINT ─► Save progress for verification            │
│       │           - Record execution results                │
│       │           - Prepare for captain-verifier            │
│       ▼                                                     │
│  6. EMIT ───────► Output EXECUTION_RESULTS.yml              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Evolution-Specific Delegation

| Step | Delegate To | Purpose |
|:-----|:------------|:--------|
| Clone failure | `skill-captain-debugger` | Diagnose network/auth issues |
| Schema validation | `skill-captain-verifier` | Validate skill structure |
| Conflict detected | `skill-debate` | Resolve naming/scope conflicts |

### Evolution Safety Controls

```yaml
evolution_safety:
  sandbox_mode: true          # Isolate changes until verified
  rollback_enabled: true      # Automatic rollback on failure
  max_candidates_per_run: 5   # Limit batch size
  require_verification: true  # Must pass verifier before commit
```

### Evolution Rollback Protocol

If any integration step fails:

1. **Checkpoint**: Save current state before each step
2. **Detect**: Identify failure point and reason
3. **Rollback**: Restore to last known good state
4. **Report**: Document failure for LEARN phase

```typescript
interface EvolutionRollback {
  trigger: 'step_failure' | 'verification_failure' | 'conflict';
  action: 'restore_checkpoint' | 'remove_artifact' | 'revert_wiring';
  report_to: 'captain-verifier';
}
```

### Evolution Output: EXECUTION_RESULTS.yml

```yaml
# Output for captain-verifier
execution_results:
  cycle_id: {evolution_cycle_id}
  timestamp: {ISO8601}
  candidates:
    - name: {skill-name}
      status: SUCCESS | PARTIAL | FAILED
      artifacts_created:
        - .agent/skills/{skill-name}/SKILL.md
      wiring_complete: true | false
      rollback_available: true
      notes: {execution_notes}
```

### Evolution Example Invocation

```bash
# Trigger evolution execution
ag evolve execute

# With specific integration plan
ag evolve execute --plan records/evolution/INTEGRATION_PLAN.md

# Dry-run mode (no actual changes)
ag evolve execute --dry-run

# Output location
# -> records/evolution/EXECUTION_RESULTS.yml
```
