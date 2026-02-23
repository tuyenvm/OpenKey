#!/usr/bin/env python3
"""
Heavy3 Code Audit - Open Source
Single model review via OpenRouter (default: Kimi K2.5)
Sponsored by Heavy3.ai
"""

import argparse
import json
import logging
import os
import sys
import time
from pathlib import Path

# Configure logging (debug output to stderr, doesn't interfere with streaming)
logging.basicConfig(
    level=logging.WARNING,
    format='[%(levelname)s] %(message)s',
    stream=sys.stderr
)
logger = logging.getLogger(__name__)

try:
    import requests
except ImportError:
    print("ERROR: 'requests' module not found. Install with: pip install requests")
    sys.exit(1)


# ============================================================================
# RETRY CONFIGURATION
# ============================================================================

MAX_RETRIES = 3
INITIAL_BACKOFF_SECONDS = 2  # 2s, 4s, 8s


def retry_with_backoff(func, max_retries=MAX_RETRIES):
    """Execute function with exponential backoff on transient failures."""
    last_error = None
    for attempt in range(max_retries):
        try:
            return func()
        except requests.exceptions.Timeout as e:
            last_error = e
            if attempt < max_retries - 1:
                wait = INITIAL_BACKOFF_SECONDS * (2 ** attempt)
                print(f"Request timed out. Retrying in {wait}s... (attempt {attempt + 2}/{max_retries})")
                time.sleep(wait)
        except requests.exceptions.HTTPError as e:
            # Retry on 5xx errors and 429 (rate limit)
            if hasattr(e, 'response') and e.response is not None:
                status = e.response.status_code
                if status >= 500 or status == 429:
                    last_error = e
                    if attempt < max_retries - 1:
                        wait = INITIAL_BACKOFF_SECONDS * (2 ** attempt)
                        print(f"Server error ({status}). Retrying in {wait}s... (attempt {attempt + 2}/{max_retries})")
                        time.sleep(wait)
                        continue
            # Non-retryable HTTP error
            raise
        except requests.exceptions.ConnectionError as e:
            last_error = e
            if attempt < max_retries - 1:
                wait = INITIAL_BACKOFF_SECONDS * (2 ** attempt)
                print(f"Connection error. Retrying in {wait}s... (attempt {attempt + 2}/{max_retries})")
                time.sleep(wait)
    # All retries exhausted
    raise last_error


# ============================================================================
# CONFIGURATION
# ============================================================================

DEFAULT_CONFIG = {
    "model": "moonshotai/kimi-k2.5",         # Default: Kimi K2.5 (best price/performance)
    "free_model": "nvidia/nemotron-3-nano-30b-a3b:free",  # Free tier (update as models rotate)
    "reasoning": "high",                     # Always high for thorough review
    "docs_folder": "documents",
    "max_file_size": 50000,
    "max_context": 200000,                   # 200K context limit
    "enable_web_search": True                # Web search enabled by default
}

OPENROUTER_URL = "https://openrouter.ai/api/v1/chat/completions"

# ============================================================================
# COST ESTIMATION
# ============================================================================

# Pricing per 1M tokens (approximate, update as OpenRouter prices change)
MODEL_PRICING = {
    "moonshotai/kimi-k2.5": {"input": 0.50, "output": 2.80},
    "moonshotai/kimi-k2.5:online": {"input": 0.50, "output": 2.80},
    "deepseek/deepseek-v3.2": {"input": 0.27, "output": 0.40},
    "openai/gpt-5.2": {"input": 1.75, "output": 14.00},
    "openai/gpt-5.2:online": {"input": 1.75, "output": 14.00},
    "google/gemini-3-pro-preview": {"input": 2.00, "output": 12.00},
    "google/gemini-3-pro-preview:online": {"input": 2.00, "output": 12.00},
    "x-ai/grok-4": {"input": 3.00, "output": 15.00},
    "x-ai/grok-4:online": {"input": 3.00, "output": 15.00},
    "nvidia/nemotron-3-nano-30b-a3b:free": {"input": 0.0, "output": 0.0},
}

# Default pricing for unknown models
DEFAULT_PRICING = {"input": 0.50, "output": 1.00}


def estimate_cost(model: str, input_chars: int, est_output_tokens: int = 2500) -> dict:
    """
    Estimate cost for a review.

    Args:
        model: OpenRouter model ID
        input_chars: Total characters in context
        est_output_tokens: Estimated output tokens (default 2500 for typical review)

    Returns:
        dict with input_tokens, output_tokens, input_cost, output_cost, total_cost
    """
    # Rule of thumb: 1 token ≈ 4 characters
    input_tokens = input_chars // 4

    # Get pricing for model (strip :online suffix for lookup)
    base_model = model.replace(":online", "")
    prices = MODEL_PRICING.get(model) or MODEL_PRICING.get(base_model) or DEFAULT_PRICING

    input_cost = (input_tokens * prices["input"]) / 1_000_000
    output_cost = (est_output_tokens * prices["output"]) / 1_000_000

    return {
        "input_tokens": input_tokens,
        "output_tokens": est_output_tokens,
        "input_cost": input_cost,
        "output_cost": output_cost,
        "total_cost": input_cost + output_cost,
        "model": model,
    }


def format_cost_estimate(estimate: dict) -> str:
    """Format cost estimate for display."""
    return f"""## Cost Estimate

| Metric | Value |
|--------|-------|
| Context size | ~{estimate['input_tokens'] * 4 // 1000}K chars |
| Est. input tokens | ~{estimate['input_tokens'] // 1000}K |
| Model | {estimate['model']} |
| **Est. cost** | **~${estimate['total_cost']:.4f}** |

Proceeding with review...
"""

# ============================================================================
# SYSTEM PROMPTS
# ============================================================================

PLAN_REVIEW_PROMPT = """You are a senior software architect performing a thorough review of an implementation plan.

If developer intent/conversation context is provided, use it to understand:
- The original problem being solved
- Constraints or requirements the developer mentioned
- Why certain approaches were chosen
This context helps you provide more relevant, targeted feedback.

Your task is to provide a second opinion on the proposed plan, identifying:
1. **Potential Issues**: Problems, edge cases, or risks not addressed
2. **Architecture Concerns**: Design decisions that may cause problems
3. **Missing Considerations**: Important aspects not covered
4. **Suggestions**: Improvements or alternative approaches

Be constructive but thorough. Point out real issues, not nitpicks.

Format your review as:

## Summary
[1-2 sentence overall assessment]

## Concerns
[Bulleted list of issues, ordered by severity]

## Suggestions
[Specific, actionable recommendations]

## Questions to Consider
[Things the implementer should think about]
"""

CODE_REVIEW_PROMPT = """You are a senior software engineer performing a thorough code review.

You are reviewing code changes (diff) along with full file context and relevant documentation.

If developer intent/conversation context is provided, use it to understand:
- The purpose behind these changes
- Constraints or requirements the developer mentioned
- Why certain approaches were chosen
This context helps you provide more relevant, targeted feedback.

Your task is to identify:
1. **Bugs**: Actual bugs or likely runtime errors
2. **Logic Issues**: Incorrect logic, edge cases, race conditions
3. **Security**: Vulnerabilities (XSS, injection, auth bypass, etc.)
4. **Performance**: Inefficiencies, N+1 queries, memory leaks
5. **Maintainability**: Code that will be hard to maintain
6. **Best Practices**: Deviations from project conventions

DO NOT comment on:
- Formatting/style (unless egregious)
- Minor naming preferences
- Things that work correctly

Format your review as:

## Summary
[1-2 sentence overall assessment - is this good to merge?]

## Issues Found
[Bulleted list with file:line references where applicable]

## Suggestions
[Specific improvements, with code examples if helpful]

## Approved
[Yes/No/With Changes - your recommendation]
"""

PR_REVIEW_PROMPT = """You are a senior software engineer performing a thorough pull request review.

You are reviewing a GitHub PR with:
- PR metadata (title, description, author, branch info)
- The full diff of changes
- Full content of changed files for context
- Relevant project documentation

If developer intent/conversation context is provided, use it to understand:
- The purpose behind these changes
- Constraints or requirements the developer mentioned
- Why certain approaches were chosen
This context helps you provide more relevant, targeted feedback.

Your task is to:
1. **Verify PR purpose**: Does the code actually accomplish what the PR description says?
2. **Check for bugs**: Actual bugs or likely runtime errors
3. **Review logic**: Incorrect logic, edge cases, race conditions
4. **Security audit**: Vulnerabilities (XSS, injection, auth bypass, etc.)
5. **Performance check**: Inefficiencies, N+1 queries, memory leaks
6. **Maintainability**: Code that will be hard to maintain
7. **Test coverage**: Are there tests? Are edge cases covered?
8. **Documentation**: Does the PR need docs updates?

DO NOT comment on:
- Formatting/style (unless egregious)
- Minor naming preferences
- Things that work correctly

Format your review as:

## Summary
[1-2 sentence overall assessment]

## PR Description Check
[Does the code match what the PR claims to do? Any discrepancies?]

## Issues Found
[Bulleted list with file:line references where applicable, ordered by severity]

## Suggestions
[Specific improvements, with code examples if helpful]

## Questions for Author
[Clarifications needed before approval]

## Verdict
[APPROVE / REQUEST CHANGES / COMMENT - your recommendation with brief reasoning]
"""


# ============================================================================
# MAIN LOGIC
# ============================================================================

def get_skill_dir():
    """Get the skill directory path (cross-platform)."""
    if os.name == 'nt':  # Windows
        return Path(os.environ.get('USERPROFILE', '')) / '.claude' / 'skills' / 'h3'
    else:  # macOS/Linux
        return Path.home() / '.claude' / 'skills' / 'h3'


def load_config():
    """Load config from skill directory."""
    config_path = get_skill_dir() / 'config.json'
    if config_path.exists():
        with open(config_path) as f:
            user_config = json.load(f)
            return {**DEFAULT_CONFIG, **user_config}
    return DEFAULT_CONFIG


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
        print("\nTo fix this, either:")
        print("\n1. Create a .env file in the skill directory:")
        print(f"   {get_skill_dir() / '.env'}")
        print('   OPENROUTER_API_KEY=your-key-here')
        print("\nOR")
        if os.name == 'nt':  # Windows
            print("\n2. Set it as an environment variable (PowerShell):")
            print('   [System.Environment]::SetEnvironmentVariable("OPENROUTER_API_KEY", "your-key", "User")')
            print("   Then restart your terminal")
        else:  # macOS/Linux
            print("\n2. Add to your shell config (~/.zshrc or ~/.bashrc):")
            print('   export OPENROUTER_API_KEY="your-key-here"')
            print("   Then restart your terminal or run: source ~/.zshrc")
        print("\nGet an API key from https://openrouter.ai/keys")
        sys.exit(1)
    return key


def build_user_message(context: dict, review_type: str) -> str:
    """Build the user message with all context."""
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
        parts.append(context.get("plan_content", "No plan content provided"))
        parts.append("\n\n")

    if review_type == "pr" and context.get("pr_metadata"):
        pr = context["pr_metadata"]
        parts.append("## Pull Request Information\n")
        parts.append(f"**PR #{pr.get('number', 'N/A')}**: {pr.get('title', 'No title')}\n")
        parts.append(f"**Author**: {pr.get('author', 'Unknown')}\n")
        parts.append(f"**Branch**: {pr.get('head_branch', '?')} → {pr.get('base_branch', '?')}\n")
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


def get_system_prompt(review_type: str) -> str:
    """Get the appropriate system prompt for the review type."""
    if review_type == "plan":
        return PLAN_REVIEW_PROMPT
    elif review_type == "pr":
        return PR_REVIEW_PROMPT
    else:
        return CODE_REVIEW_PROMPT


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
            logger.warning("No choices in OpenRouter response")
            return "ERROR: No choices in API response. The model may have failed to generate a response."

        message = choices[0].get("message", {})
        content = message.get("content", "")

        if not content:
            # Check for error in response
            if result.get("error"):
                error_msg = result["error"].get("message", "Unknown error")
                return f"ERROR: API returned error: {error_msg}"
            logger.warning("Empty content in OpenRouter response")
            return "ERROR: Empty content in API response. The model returned no text."

        return content

    except (IndexError, KeyError, TypeError) as e:
        logger.error(f"Failed to extract content from response: {e}")
        return f"ERROR: Unexpected response format from API: {e}"


def call_openrouter(config: dict, review_type: str, context: dict, stream: bool = True) -> str:
    """Call OpenRouter API with the review request.

    Args:
        config: Configuration dict with model, tier, etc.
        review_type: One of 'plan', 'code', 'pr'
        context: Context dict with diff, files, etc.
        stream: If True, stream response and print as it arrives (default: True)

    Returns:
        The complete review text
    """
    api_key = get_api_key()

    system_prompt = get_system_prompt(review_type)
    user_message = build_user_message(context, review_type)

    # Truncate if too long (200K limit)
    max_context = config.get("max_context", 200000)
    if len(user_message) > max_context:
        user_message = user_message[:max_context] + "\n\n[... truncated due to length ...]"

    # Determine model to use (with optional :online suffix for web search)
    model = config["model"]
    enable_web_search = config.get("enable_web_search", True)

    if enable_web_search:
        # Add :online suffix if not already present
        if not model.endswith(":online") and ":free" not in model:
            model = f"{model}:online"

    payload = {
        "model": model,
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_message}
        ],
        "stream": stream
    }

    # Add web search plugin if enabled
    if enable_web_search and ":online" in model:
        # Use Exa for semantic search (works with most models)
        payload["plugins"] = [{"id": "web", "engine": "exa"}]

    # Add reasoning/thinking parameter if supported
    reasoning = config.get("reasoning", "high")
    if reasoning and reasoning != "none":
        # OpenRouter passes provider-specific params
        payload["provider"] = {
            "require_parameters": False
        }
        # For OpenAI models with reasoning
        if "gpt" in config["model"].lower() or "o1" in config["model"].lower():
            payload["reasoning"] = {"effort": reasoning}

    headers = {
        "Authorization": f"Bearer {api_key}",
        "Content-Type": "application/json",
        "HTTP-Referer": "https://heavy3.ai/code-audit",
        "X-Title": "Heavy3 Code Audit"
    }

    def make_request():
        """Inner function for retry logic."""
        response = requests.post(
            OPENROUTER_URL,
            headers=headers,
            json=payload,
            timeout=180,  # 3 minute timeout for reasoning models
            stream=stream
        )
        response.raise_for_status()
        return response

    try:
        response = retry_with_backoff(make_request)

        if stream:
            # Handle streaming response (SSE format)
            full_content = []
            malformed_count = 0
            for line in response.iter_lines():
                if line:
                    line_str = line.decode('utf-8')
                    if line_str.startswith('data: '):
                        data_str = line_str[6:]  # Remove 'data: ' prefix
                        if data_str.strip() == '[DONE]':
                            break
                        try:
                            data = json.loads(data_str)
                            delta = data.get('choices', [{}])[0].get('delta', {})
                            content = delta.get('content', '')
                            if content:
                                print(content, end='', flush=True)
                                full_content.append(content)
                        except json.JSONDecodeError as e:
                            malformed_count += 1
                            # Log malformed chunks for debugging (to stderr)
                            logger.debug(f"Skipping malformed SSE chunk: {data_str[:100]}... ({e})")
            print()  # Final newline
            if malformed_count > 0:
                logger.warning(f"Skipped {malformed_count} malformed SSE chunk(s) during streaming")
            return ''.join(full_content)
        else:
            # Non-streaming response - use safe extraction
            result = response.json()
            return extract_content(result)

    except requests.exceptions.Timeout:
        return "ERROR: Request timed out after retries. The model may be overloaded. Try again later."
    except requests.exceptions.HTTPError as e:
        error_detail = ""
        try:
            if hasattr(e, 'response') and e.response is not None:
                error_detail = e.response.json().get("error", {}).get("message", "")
        except (json.JSONDecodeError, KeyError, TypeError):
            pass
        return f"ERROR: API request failed: {e}\n{error_detail}"
    except Exception as e:
        return f"ERROR: Unexpected error: {e}"


MODEL_SHORTCUTS = {
    "gpt": "openai/gpt-5.2",
    "premium": "openai/gpt-5.2",
    "kimi": "moonshotai/kimi-k2.5",
    "standard": "moonshotai/kimi-k2.5",
    "std": "moonshotai/kimi-k2.5",
    "deepseek": "deepseek/deepseek-v3.2",
    # "free" is handled specially in resolve_model() to read from config
}


def resolve_model(model_arg: str, config: dict) -> str:
    """Resolve model shortcut to full OpenRouter model ID."""
    if not model_arg:
        return None
    lower = model_arg.lower()

    # Handle 'free' specially - use free_model from config
    if lower == "free":
        return config.get("free_model", "nvidia/nemotron-3-nano-30b-a3b:free")

    return MODEL_SHORTCUTS.get(lower, model_arg)


def main():
    parser = argparse.ArgumentParser(description="Heavy3 Code Audit - Lite Mode")
    parser.add_argument("--type", choices=["plan", "code", "pr"], required=True,
                        help="Type of review: plan, code, or pr")
    parser.add_argument("--context-file", required=True,
                        help="Path to JSON file with review context")
    parser.add_argument("--model", "-m", default=None,
                        help="Model to use: 'gpt'/'premium', 'deepseek'/'std', 'free', or full OpenRouter model ID")
    parser.add_argument("--no-stream", action="store_true",
                        help="Disable streaming (wait for full response)")

    args = parser.parse_args()

    # Load config
    config = load_config()

    # Override model if specified
    if args.model:
        config["model"] = resolve_model(args.model, config)
        print(f"Using model: {config['model']}")

    # Load context
    try:
        with open(args.context_file) as f:
            context = json.load(f)
    except FileNotFoundError:
        print(f"ERROR: Context file not found: {args.context_file}")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"ERROR: Invalid JSON in context file: {e}")
        sys.exit(1)

    # Call API with streaming (default) or blocking
    stream = not args.no_stream
    review = call_openrouter(config, args.type, context, stream=stream)

    # Output review (only if not streaming, since streaming already prints)
    if not stream:
        print(review)

    # Sponsored footer
    print("\n" + "-" * 60)
    print("Powered by Heavy3.ai - Open Source Code Audit")
    print("Use /h3 --council for 3-model consensus review")
    print("-" * 60)


if __name__ == "__main__":
    main()
