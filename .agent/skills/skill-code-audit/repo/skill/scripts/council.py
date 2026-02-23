#!/usr/bin/env python3
"""
Heavy3 Code Audit - Council (Open Source)
Runs 3 parallel reviews via OpenRouter with specialized roles + web search.

This file is MIT licensed and fully open source.
Sponsored by Heavy3.ai - Multi-model AI for high-stakes decisions.
"""
__version__ = "2.0.0"
__version_date__ = "2025-01-31"

import argparse
import json
import os
import sys
import time
import concurrent.futures
from pathlib import Path

try:
    import requests
except ImportError:
    print("ERROR: 'requests' module not found. Install with: pip install requests")
    sys.exit(1)

OPENROUTER_URL = "https://openrouter.ai/api/v1/chat/completions"

# Retry configuration
MAX_RETRIES = 3
INITIAL_BACKOFF_SECONDS = 2


def retry_with_backoff(func, role_name="API", max_retries=MAX_RETRIES):
    """Execute function with exponential backoff on transient failures."""
    last_error = None
    for attempt in range(max_retries):
        try:
            return func()
        except requests.exceptions.Timeout as e:
            last_error = e
            if attempt < max_retries - 1:
                wait = INITIAL_BACKOFF_SECONDS * (2 ** attempt)
                print(f"[{role_name}] Timeout. Retrying in {wait}s... ({attempt + 2}/{max_retries})")
                time.sleep(wait)
        except requests.exceptions.HTTPError as e:
            if hasattr(e, 'response') and e.response is not None:
                status = e.response.status_code
                if status >= 500 or status == 429:
                    last_error = e
                    if attempt < max_retries - 1:
                        wait = INITIAL_BACKOFF_SECONDS * (2 ** attempt)
                        print(f"[{role_name}] Server error ({status}). Retrying in {wait}s... ({attempt + 2}/{max_retries})")
                        time.sleep(wait)
                        continue
            raise
        except requests.exceptions.ConnectionError as e:
            last_error = e
            if attempt < max_retries - 1:
                wait = INITIAL_BACKOFF_SECONDS * (2 ** attempt)
                print(f"[{role_name}] Connection error. Retrying in {wait}s... ({attempt + 2}/{max_retries})")
                time.sleep(wait)
    raise last_error


# Default council models (can be overridden via config.json "council_models")
# Web search is controlled by config.json "enable_web_search"
DEFAULT_COUNCIL_MODELS = {
    "correctness": "openai/gpt-5.2",
    "performance": "google/gemini-3-pro-preview",
    "security": "x-ai/grok-4",
}

CODE_PROMPTS = {
    "correctness": """You are "The Correctness Expert" reviewing code changes.
Your Role: Provide accurate, direct review of the code changes.

If developer intent is provided, use it to understand the purpose and constraints.

Focus ONLY on:
1. **Bugs**: Actual bugs or likely runtime errors
2. **Logic Issues**: Incorrect logic, edge cases, race conditions
3. **Edge Cases**: Boundary conditions, null/undefined handling
4. **Type Safety**: Type mismatches, implicit conversions

DO NOT comment on security, performance, or style.
Be specific with file:line references.

Format your review as:
## Correctness Assessment
[Overall assessment]

## Issues Found
[Bulleted list with file:line references]

## Edge Cases to Consider
[Specific scenarios that might fail]
""",

    "security": """You are "The Security Analyst" reviewing code changes.
Your Role: Apply critical security thinking. Find vulnerabilities.

If developer intent is provided, use it to understand the purpose and constraints.

Focus ONLY on:
1. **Injection**: SQL, XSS, command injection, path traversal
2. **Authentication**: Auth bypass, session handling, token security
3. **Authorization**: Privilege escalation, IDOR, access control
4. **Data Exposure**: Secrets in code, PII leakage, logging sensitive data
5. **Dependencies**: Known vulnerabilities, insecure patterns

DO NOT comment on correctness, performance, or style.
Be specific with file:line references.

Format your review as:
## Security Assessment
[Overall security posture]

## Vulnerabilities Found
[Bulleted list with severity: CRITICAL/HIGH/MEDIUM/LOW]

## Security Recommendations
[Specific fixes]
""",

    "performance": """You are "The Performance Critic" reviewing code changes.
Your Role: Analyze performance implications. Find inefficiencies.

If developer intent is provided, use it to understand the purpose and constraints.

Focus ONLY on:
1. **Time Complexity**: O(n^2) loops, repeated iterations, inefficient algorithms
2. **Space Complexity**: Memory leaks, unnecessary allocations, large objects
3. **Database**: N+1 queries, missing indexes, unoptimized queries
4. **I/O**: Blocking operations, unnecessary network calls, file handling
5. **Caching**: Missing cache opportunities, cache invalidation issues

DO NOT comment on correctness, security, or style.
Be specific with file:line references.

Format your review as:
## Performance Assessment
[Overall performance impact]

## Performance Issues
[Bulleted list with impact: HIGH/MEDIUM/LOW]

## Optimization Suggestions
[Specific improvements]
"""
}

PLAN_PROMPTS = {
    "design": """You are "The Design Expert" reviewing an implementation plan.
Your Role: Evaluate architectural patterns and design decisions.

If developer intent is provided, use it to understand the purpose and constraints.

Focus ONLY on:
1. **Patterns**: Appropriate design patterns, anti-patterns to avoid
2. **Structure**: Separation of concerns, modularity, cohesion
3. **SOLID**: Single responsibility, open/closed, dependency inversion
4. **Maintainability**: Code organization, naming, documentation needs

DO NOT comment on security, scalability, or implementation details.

Format your review as:
## Design Assessment
[Overall design quality]

## Design Concerns
[Bulleted list]

## Recommended Patterns
[Specific suggestions]
""",

    "security": """You are "The Security Architect" reviewing an implementation plan.
Your Role: Analyze security architecture and threat model.

If developer intent is provided, use it to understand the purpose and constraints.

Focus ONLY on:
1. **Threat Model**: Attack surfaces, trust boundaries
2. **Authentication Design**: Auth flow, session management
3. **Authorization Design**: RBAC, permissions model
4. **Data Security**: Encryption at rest/transit, key management
5. **Compliance**: OWASP, industry standards

DO NOT comment on design patterns or scalability.

Format your review as:
## Security Architecture Assessment
[Overall security posture]

## Threat Model Concerns
[Attack vectors to consider]

## Security Requirements
[What must be implemented]
""",

    "scalability": """You are "The Scalability Analyst" reviewing an implementation plan.
Your Role: Evaluate scalability and long-term maintainability.

If developer intent is provided, use it to understand the purpose and constraints.

Focus ONLY on:
1. **Horizontal Scaling**: Can this scale out? Bottlenecks?
2. **Database Scaling**: Sharding, replication, query patterns
3. **Caching Strategy**: What to cache, invalidation approach
4. **Future Extensibility**: Will this support new features?
5. **Technical Debt**: What will be painful to change later?

DO NOT comment on security or design patterns.

Format your review as:
## Scalability Assessment
[Can this handle 10x? 100x?]

## Scaling Concerns
[Bottlenecks and limitations]

## Future Considerations
[What to plan for]
"""
}


def get_skill_dir():
    if os.name == 'nt':
        return Path(os.environ.get('USERPROFILE', '')) / '.claude' / 'skills' / 'h3'
    return Path.home() / '.claude' / 'skills' / 'h3'


def load_config():
    config_path = get_skill_dir() / 'config.json'
    default = {"reasoning": "high", "max_context": 200000}
    if config_path.exists():
        with open(config_path) as f:
            return {**default, **json.load(f)}
    return default


def load_dotenv():
    """Load environment variables from .env file in skill directory."""
    env_path = get_skill_dir() / '.env'
    if env_path.exists():
        with open(env_path) as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#') and '=' in line:
                    key, _, value = line.partition('=')
                    key = key.strip()
                    value = value.strip().strip('"').strip("'")
                    if key and value:
                        os.environ.setdefault(key, value)


def get_api_key():
    """Get OpenRouter API key from environment or .env file."""
    key = os.environ.get("OPENROUTER_API_KEY")
    if not key:
        # Try loading from .env file as fallback
        load_dotenv()
        key = os.environ.get("OPENROUTER_API_KEY")
    if not key:
        print("ERROR: OPENROUTER_API_KEY not found.")
        print(f"\nCreate a .env file at: {get_skill_dir() / '.env'}")
        print('With: OPENROUTER_API_KEY=your-key-here')
        sys.exit(1)
    return key


def build_user_message(context: dict, review_type: str) -> str:
    parts = []

    # Conversation context FIRST (most important for understanding intent)
    if context.get("conversation_context"):
        cc = context["conversation_context"]
        parts.append("## Developer Intent\n\n")
        if cc.get("original_request"):
            parts.append(f"**Original Request:** {cc['original_request']}\n\n")
        if cc.get("approach_notes"):
            parts.append(f"**Approach Notes:** {cc['approach_notes']}\n\n")
        if cc.get("relevant_exchanges"):
            parts.append("**Relevant Discussion:**\n")
            for msg in cc["relevant_exchanges"]:
                role = "User" if msg.get("role") == "user" else "Agent"
                content = msg.get("content", "")[:500]
                parts.append(f"- **{role}:** {content}\n")
            parts.append("\n")
        if cc.get("previous_review_findings"):
            parts.append(f"**Prior Review Notes:** {cc['previous_review_findings']}\n\n")
        parts.append("---\n\n")

    if review_type == "plan":
        parts.append("## Plan to Review\n")
        parts.append(context.get("plan_content", "No plan content"))
        parts.append("\n\n")

    if review_type == "pr" and context.get("pr_metadata"):
        pr = context["pr_metadata"]
        parts.append("## Pull Request Information\n")
        parts.append(f"**PR #{pr.get('number')}**: {pr.get('title')}\n")
        parts.append(f"**Author**: {pr.get('author')}\n")
        parts.append(f"**Branch**: {pr.get('head_branch')} -> {pr.get('base_branch')}\n")
        parts.append(f"**Changes**: +{pr.get('additions', 0)} / -{pr.get('deletions', 0)}\n\n")
        if pr.get("body"):
            parts.append("### PR Description\n")
            parts.append(pr["body"])
            parts.append("\n\n")

    if context.get("diff"):
        parts.append("## Code Changes (Diff)\n```diff\n")
        parts.append(context["diff"])
        parts.append("\n```\n\n")

    if context.get("file_contents"):
        parts.append("## Full File Contents\n")
        for path, content in context["file_contents"].items():
            parts.append(f"### {path}\n```\n{content}\n```\n\n")

    if context.get("documentation"):
        parts.append("## Relevant Documentation\n")
        for path, content in context["documentation"].items():
            parts.append(f"### {path}\n{content}\n\n")

    if context.get("test_files"):
        parts.append("## Related Test Files\n")
        for path, content in context["test_files"].items():
            parts.append(f"### {path}\n```\n{content}\n```\n\n")

    return "".join(parts)


def extract_content(result: dict) -> str:
    """
    Safely extract content from OpenRouter response.

    Args:
        result: Parsed JSON response from OpenRouter

    Returns:
        Content string, or error message if extraction fails
    """
    try:
        choices = result.get("choices", [])
        if not choices:
            return "ERROR: No choices in API response"
        message = choices[0].get("message", {})
        content = message.get("content", "")
        if not content:
            if result.get("error"):
                return f"ERROR: {result['error'].get('message', 'Unknown error')}"
            return "ERROR: Empty content in API response"
        return content
    except (IndexError, KeyError, TypeError) as e:
        return f"ERROR: Unexpected response format: {e}"


def call_reviewer(role: str, model: str, name: str, user_message: str,
                  review_type: str, api_key: str, reasoning: str,
                  search_engine: str = None) -> dict:
    prompts = CODE_PROMPTS if review_type in ["code", "pr"] else PLAN_PROMPTS
    system_prompt = prompts.get(role, prompts.get("correctness", ""))

    payload = {
        "model": model,
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_message}
        ]
    }

    # Add web search plugin (Pro feature)
    if search_engine:
        payload["plugins"] = [{"id": "web", "engine": search_engine}]

    if reasoning != "none":
        payload["provider"] = {"require_parameters": False}
        if "gpt" in model.lower() or "o1" in model.lower():
            payload["reasoning"] = {"effort": reasoning}

    headers = {
        "Authorization": f"Bearer {api_key}",
        "Content-Type": "application/json",
        "HTTP-Referer": "https://heavy3.ai/code-audit",
        "X-Title": "Heavy3 Code Audit"
    }

    start = time.time()

    def make_request():
        resp = requests.post(OPENROUTER_URL, headers=headers, json=payload, timeout=180)
        resp.raise_for_status()
        return resp

    try:
        resp = retry_with_backoff(make_request, role_name=name)
        result = resp.json()
        elapsed_ms = int((time.time() - start) * 1000)

        return {
            "role": role,
            "name": name,
            "model": model,
            "content": extract_content(result),
            "elapsed_ms": elapsed_ms,
            "tokens": {
                "input": result.get("usage", {}).get("prompt_tokens", 0),
                "output": result.get("usage", {}).get("completion_tokens", 0),
            }
        }
    except Exception as e:
        return {
            "role": role,
            "name": name,
            "model": model,
            "content": f"ERROR: {str(e)} (after {MAX_RETRIES} retries)",
            "elapsed_ms": int((time.time() - start) * 1000),
            "error": str(e)
        }


def get_council_config(config: dict, council_type: str) -> list:
    """
    Build council configuration from config.json or use defaults.

    Args:
        config: Loaded config dictionary
        council_type: "code" or "plan"

    Returns:
        List of council member configurations
    """
    # Get configured models (if any), merge with defaults
    council_models = config.get("council_models", {})
    models = {**DEFAULT_COUNCIL_MODELS, **council_models}

    # Check if web search is enabled
    enable_web_search = config.get("enable_web_search", False)

    if council_type == "code":
        return [
            {
                "role": "correctness",
                "model": models["correctness"] + (":online" if enable_web_search else ""),
                "name": "Correctness Expert",
                "search_engine": "native" if enable_web_search else None
            },
            {
                "role": "performance",
                "model": models["performance"] + (":online" if enable_web_search else ""),
                "name": "Performance Critic",
                "search_engine": "exa" if enable_web_search else None
            },
            {
                "role": "security",
                "model": models["security"] + (":online" if enable_web_search else ""),
                "name": "Security Analyst",
                "search_engine": "exa" if enable_web_search else None
            },
        ]
    else:  # plan
        return [
            {
                "role": "design",
                "model": models["correctness"] + (":online" if enable_web_search else ""),
                "name": "Design Expert",
                "search_engine": "native" if enable_web_search else None
            },
            {
                "role": "scalability",
                "model": models["performance"] + (":online" if enable_web_search else ""),
                "name": "Scalability Analyst",
                "search_engine": "exa" if enable_web_search else None
            },
            {
                "role": "security",
                "model": models["security"] + (":online" if enable_web_search else ""),
                "name": "Security Architect",
                "search_engine": "exa" if enable_web_search else None
            },
        ]


def run_council(context: dict, review_type: str) -> dict:
    config = load_config()
    api_key = get_api_key()
    reasoning = config.get("reasoning", "high")

    council_type = "plan" if review_type == "plan" else "code"
    council = get_council_config(config, council_type)

    user_message = build_user_message(context, review_type)

    # 200K context limit
    max_context = config.get("max_context", 200000)
    if len(user_message) > max_context:
        user_message = user_message[:max_context] + "\n\n[... truncated ...]"

    start = time.time()
    reviews = []

    # Show clear progress header
    print("\n" + "=" * 50, file=sys.stderr)
    print("Heavy3 Council (Sponsored by Heavy3.ai)", file=sys.stderr)
    print("=" * 50, file=sys.stderr)
    print(f"Starting parallel review with:", file=sys.stderr)
    for member in council:
        print(f"  • {member['name']} ({member['model'].split('/')[-1]})", file=sys.stderr)
    print("-" * 50, file=sys.stderr)

    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as executor:
        futures = {
            executor.submit(
                call_reviewer,
                member["role"],
                member["model"],
                member["name"],
                user_message,
                review_type,
                api_key,
                reasoning,
                member.get("search_engine")
            ): member
            for member in council
        }

        completed_count = 0
        for future in concurrent.futures.as_completed(futures):
            result = future.result()
            member = futures[future]
            elapsed_sec = result.get("elapsed_ms", 0) / 1000
            completed_count += 1
            if result.get("error"):
                print(f"  [{completed_count}/3] ✗ {member['name']} FAILED ({elapsed_sec:.1f}s)", file=sys.stderr)
            else:
                print(f"  [{completed_count}/3] ✓ {member['name']} completed ({elapsed_sec:.1f}s)", file=sys.stderr)
            reviews.append(result)

    total_sec = (time.time() - start)
    print("-" * 50, file=sys.stderr)
    print(f"Council complete in {total_sec:.1f}s", file=sys.stderr)
    print("=" * 50 + "\n", file=sys.stderr)

    role_order = {r["role"]: i for i, r in enumerate(council)}
    reviews.sort(key=lambda r: role_order.get(r["role"], 99))

    return {
        "reviews": reviews,
        "metadata": {
            "total_ms": int((time.time() - start) * 1000),
            "council_type": council_type,
        }
    }


def main():
    parser = argparse.ArgumentParser(description="Heavy3 Code Audit Council (Sponsored by Heavy3.ai)")
    parser.add_argument("--type", choices=["plan", "code", "pr"], required=True)
    parser.add_argument("--context-file", required=True)

    args = parser.parse_args()

    with open(args.context_file) as f:
        context = json.load(f)

    result = run_council(context, args.type)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
