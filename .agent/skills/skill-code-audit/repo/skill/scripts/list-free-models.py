#!/usr/bin/env python3
"""
List Free Models Utility
Fetches and displays free models from OpenRouter, sorted by release date.
"""

import sys
from datetime import datetime

try:
    import requests
except ImportError:
    print("ERROR: 'requests' module not found. Install with: pip install requests")
    sys.exit(1)


OPENROUTER_MODELS_URL = "https://openrouter.ai/api/v1/models"

# Keywords that suggest a model supports extended thinking/reasoning
THINKING_KEYWORDS = [
    "thinking", "r1", "o1", "o3", "reasoner", "reason",
    "deepthink", "think", "cot"  # chain of thought
]

# Keywords that suggest NOT a thinking model
NON_THINKING_KEYWORDS = [
    "instruct", "chat", "fast", "turbo", "mini", "lite", "small"
]


def is_thinking_model(model_id: str, model_name: str) -> str:
    """
    Determine if model is likely a thinking/reasoning model.
    Returns: "THINKING", "STANDARD", or "UNKNOWN"
    """
    combined = f"{model_id} {model_name}".lower()

    # Check for thinking indicators
    for keyword in THINKING_KEYWORDS:
        if keyword in combined:
            return "THINKING"

    # Check for non-thinking indicators
    for keyword in NON_THINKING_KEYWORDS:
        if keyword in combined:
            return "STANDARD"

    return "UNKNOWN"


def fetch_models():
    """Fetch all models from OpenRouter API."""
    try:
        response = requests.get(OPENROUTER_MODELS_URL, timeout=30)
        response.raise_for_status()
        return response.json().get("data", [])
    except requests.exceptions.RequestException as e:
        print(f"ERROR: Failed to fetch models: {e}")
        print("\nTry browsing manually:")
        print("  https://openrouter.ai/models?fmt=table&order=newest")
        sys.exit(1)


def parse_date(date_str: str) -> datetime:
    """Parse date string to datetime object. Returns datetime.min if invalid."""
    if not date_str:
        return datetime.min
    try:
        # Try ISO format first
        return datetime.fromisoformat(date_str.replace("Z", "+00:00"))
    except:
        pass
    try:
        # Try Unix timestamp (some APIs return this)
        return datetime.fromtimestamp(int(date_str))
    except:
        pass
    return datetime.min


def format_date(date_str: str) -> str:
    """Format date for display."""
    dt = parse_date(date_str)
    if dt == datetime.min:
        return "-"
    return dt.strftime("%Y-%m-%d")


def is_free_model(model: dict) -> bool:
    """Check if model is free (both input and output pricing are 0)."""
    pricing = model.get("pricing", {})

    # Handle different pricing formats
    prompt_price = pricing.get("prompt", "0")
    completion_price = pricing.get("completion", "0")

    # Convert to float and check if both are 0
    try:
        prompt = float(prompt_price) if prompt_price else 0
        completion = float(completion_price) if completion_price else 0
        return prompt == 0 and completion == 0
    except (ValueError, TypeError):
        return False


def main():
    print("Fetching models from OpenRouter...")
    print()

    models = fetch_models()

    # Filter to free models only
    free_models = [m for m in models if is_free_model(m)]

    if not free_models:
        print("No free models found at the moment.")
        print("\nCheck manually: https://openrouter.ai/models?fmt=table&order=newest")
        return

    # Sort by date (newest first), models without dates go last
    def sort_key(m):
        dt = parse_date(m.get("created"))
        # Models with dates sort by date desc, models without sort to end
        has_date = dt != datetime.min
        return (not has_date, -dt.timestamp() if has_date else 0)

    free_models.sort(key=sort_key)

    print("FREE MODELS ON OPENROUTER (newest first)")
    print("-" * 78)
    print()
    print(f" {'#':<3} {'Model ID':<43} {'Type':<10} {'Context':<10} {'Released':<10}")
    print(f" {'-'*2:<3} {'-'*41:<43} {'-'*8:<10} {'-'*8:<10} {'-'*10:<10}")

    for i, model in enumerate(free_models, 1):
        model_id = model.get("id", "unknown")
        model_name = model.get("name", model_id)
        context = model.get("context_length", 0) or 0
        context_str = f"{context:,}" if context else "?"
        released = format_date(model.get("created"))

        # Determine thinking status
        thinking_status = is_thinking_model(model_id, model_name)

        # Format thinking tag
        if thinking_status == "THINKING":
            tag = "THINKING"
        elif thinking_status == "STANDARD":
            tag = "standard"
        else:
            tag = "-"

        # Truncate model_id for display
        display_id = model_id if len(model_id) <= 41 else model_id[:38] + "..."

        print(f" {i:<3} {display_id:<43} {tag:<10} {context_str:<10} {released:<10}")

    print()
    print("-" * 78)
    print()
    print("LEGEND:")
    print("   THINKING  = Extended reasoning (recommended for code reviews)")
    print("   standard  = Regular model (faster)")
    print("   -         = Unknown type / no date available")
    print()
    print("TO USE A FREE MODEL:")
    print()
    print("   1. Edit: ~/.claude/skills/h3/config.json")
    print('   2. Set:  "free_model": "<model-id-from-above>"')
    print("   3. Run:  /h3 --free")
    print()
    print("Tip: Newer models (at top) are more likely to still be free!")
    print()
    print("More info: https://openrouter.ai/models")


if __name__ == "__main__":
    main()
