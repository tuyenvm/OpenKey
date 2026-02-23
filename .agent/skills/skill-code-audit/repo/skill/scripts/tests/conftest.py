"""
Shared pytest fixtures for Heavy3 Code Audit tests.
"""

import json
import os
import pytest
import tempfile
from pathlib import Path
from unittest.mock import patch, MagicMock

# Add parent directory to path for imports
import sys
sys.path.insert(0, str(Path(__file__).parent.parent))


# ============================================================================
# Environment Fixtures
# ============================================================================

@pytest.fixture
def mock_api_key():
    """Mock OPENROUTER_API_KEY environment variable."""
    with patch.dict(os.environ, {"OPENROUTER_API_KEY": "test-api-key-12345"}):
        yield "test-api-key-12345"


@pytest.fixture
def mock_no_api_key():
    """Mock missing OPENROUTER_API_KEY."""
    env = os.environ.copy()
    env.pop("OPENROUTER_API_KEY", None)
    with patch.dict(os.environ, env, clear=True):
        yield


@pytest.fixture
def temp_config(tmp_path):
    """Create a temporary config file."""
    config = {
        "model": "moonshotai/kimi-k2.5",
        "free_model": "nvidia/nemotron-3-nano-30b-a3b:free",
        "reasoning": "high",
        "docs_folder": "documents",
        "max_file_size": 50000,
        "max_context": 200000,
        "enable_web_search": True
    }
    config_path = tmp_path / "config.json"
    config_path.write_text(json.dumps(config, indent=2))
    env_path = tmp_path / ".env"
    env_path.write_text("OPENROUTER_API_KEY=test-key\n")
    return config_path, config


@pytest.fixture
def temp_pro_config(tmp_path):
    """Create a temporary config file with web search enabled (for council tests)."""
    config = {
        "model": "moonshotai/kimi-k2.5",
        "max_context": 200000,
        "enable_web_search": True
    }
    config_path = tmp_path / "config.json"
    config_path.write_text(json.dumps(config, indent=2))
    env_path = tmp_path / ".env"
    env_path.write_text("OPENROUTER_API_KEY=test-key\n")
    return config_path, config


# ============================================================================
# Context Fixtures
# ============================================================================

@pytest.fixture
def sample_code_context():
    """Sample context for code review."""
    return {
        "review_type": "code",
        "diff": """diff --git a/src/utils.py b/src/utils.py
index abc123..def456 100644
--- a/src/utils.py
+++ b/src/utils.py
@@ -10,6 +10,10 @@ def calculate_total(items):
     return sum(item.price for item in items)
+
+def validate_email(email):
+    # TODO: Add proper validation
+    return "@" in email
""",
        "changed_files": ["src/utils.py"],
        "file_contents": {
            "src/utils.py": """def calculate_total(items):
    return sum(item.price for item in items)

def validate_email(email):
    # TODO: Add proper validation
    return "@" in email
"""
        },
        "documentation": {
            "CLAUDE.md": "# Project Guidelines\nFollow PEP8."
        },
        "test_files": {}
    }


@pytest.fixture
def sample_code_context_with_conversation():
    """Sample context with conversation history."""
    return {
        "review_type": "code",
        "conversation_context": {
            "original_request": "Add email validation to the utils module",
            "approach_notes": "Using simple @ check for now, will improve later",
            "relevant_exchanges": [
                {"role": "user", "content": "Can you add basic email validation?"},
                {"role": "assistant", "content": "I'll add a simple validation function."}
            ],
            "previous_review_findings": None
        },
        "diff": "diff --git a/src/utils.py...",
        "changed_files": ["src/utils.py"],
        "file_contents": {"src/utils.py": "def validate_email(email): return '@' in email"},
        "documentation": {},
        "test_files": {}
    }


@pytest.fixture
def sample_plan_context():
    """Sample context for plan review."""
    return {
        "review_type": "plan",
        "plan_content": """# Implementation Plan: User Authentication

## Overview
Add JWT-based authentication to the API.

## Steps
1. Install jsonwebtoken package
2. Create auth middleware
3. Add login/register endpoints
4. Protect existing routes

## Files to Modify
- src/middleware/auth.js
- src/routes/api.js
- src/controllers/userController.js
""",
        "file_contents": {
            "src/routes/api.js": "const express = require('express');\nconst router = express.Router();"
        },
        "documentation": {}
    }


@pytest.fixture
def sample_pr_context():
    """Sample context for PR review."""
    return {
        "review_type": "pr",
        "pr_metadata": {
            "number": 42,
            "title": "Add email validation",
            "body": "This PR adds basic email validation to the utils module.",
            "author": "testuser",
            "base_branch": "main",
            "head_branch": "feature/email-validation",
            "additions": 15,
            "deletions": 2
        },
        "diff": "diff --git a/src/utils.py...",
        "changed_files": ["src/utils.py"],
        "file_contents": {"src/utils.py": "def validate_email(email): return '@' in email"},
        "documentation": {},
        "test_files": {}
    }


@pytest.fixture
def large_context():
    """Context that exceeds normal limits for truncation testing."""
    return {
        "review_type": "code",
        "diff": "x" * 50000,
        "changed_files": ["large_file.py"],
        "file_contents": {
            "large_file.py": "y" * 60000
        },
        "documentation": {},
        "test_files": {}
    }


# ============================================================================
# API Response Fixtures
# ============================================================================

@pytest.fixture
def openrouter_success_response():
    """Standard successful OpenRouter response."""
    return {
        "id": "gen-12345",
        "object": "chat.completion",
        "created": 1234567890,
        "model": "moonshotai/kimi-k2.5",
        "choices": [{
            "index": 0,
            "message": {
                "role": "assistant",
                "content": """## Summary
Good code with minor issues.

## Issues Found
- Line 15: Missing input validation

## Suggestions
- Add type hints

## Approved
Yes, with minor changes"""
            },
            "finish_reason": "stop"
        }],
        "usage": {
            "prompt_tokens": 1500,
            "completion_tokens": 250,
            "total_tokens": 1750
        }
    }


@pytest.fixture
def openrouter_streaming_chunks():
    """SSE streaming response chunks."""
    return [
        b'data: {"id":"gen-1","choices":[{"delta":{"role":"assistant"}}]}',
        b'data: {"id":"gen-1","choices":[{"delta":{"content":"## Summary"}}]}',
        b'data: {"id":"gen-1","choices":[{"delta":{"content":"\\nGood code."}}]}',
        b'data: {"id":"gen-1","choices":[{"delta":{"content":"\\n\\n## Issues"}}]}',
        b'data: {"id":"gen-1","choices":[{"delta":{"content":"\\nNone found."}}]}',
        b'data: [DONE]',
    ]


@pytest.fixture
def openrouter_malformed_chunks():
    """SSE streaming with malformed JSON."""
    return [
        b'data: {"id":"gen-1","choices":[{"delta":{"content":"Start"}}]}',
        b'data: {malformed json here',
        b'data: {"id":"gen-1","choices":[{"delta":{"content":" End"}}]}',
        b'data: [DONE]',
    ]


@pytest.fixture
def openrouter_error_response():
    """OpenRouter error response."""
    return {
        "error": {
            "message": "Model not available",
            "type": "invalid_request_error",
            "code": "model_not_available"
        }
    }


@pytest.fixture
def openrouter_rate_limit_response():
    """OpenRouter rate limit (429) response."""
    return {
        "error": {
            "message": "Rate limit exceeded. Please retry after 60 seconds.",
            "type": "rate_limit_error",
            "code": "rate_limit_exceeded"
        }
    }


# ============================================================================
# Council Fixtures
# ============================================================================

@pytest.fixture
def council_all_success():
    """All three council models return successfully."""
    return [
        {
            "role": "correctness",
            "name": "Correctness Expert",
            "model": "openai/gpt-5.2:online",
            "content": "## Correctness Assessment\nNo bugs found.\n\n## Issues Found\nNone.\n\n## Edge Cases to Consider\n- Empty input handling",
            "elapsed_ms": 2500,
            "tokens": {"input": 1500, "output": 200}
        },
        {
            "role": "security",
            "name": "Security Analyst",
            "model": "x-ai/grok-4:online",
            "content": "## Security Assessment\nNo vulnerabilities.\n\n## Vulnerabilities Found\nNone.\n\n## Security Recommendations\n- Add input sanitization",
            "elapsed_ms": 3000,
            "tokens": {"input": 1500, "output": 220}
        },
        {
            "role": "performance",
            "name": "Performance Critic",
            "model": "google/gemini-3-pro-preview:online",
            "content": "## Performance Assessment\nEfficient code.\n\n## Performance Issues\nNone.\n\n## Optimization Suggestions\n- Consider caching",
            "elapsed_ms": 2800,
            "tokens": {"input": 1500, "output": 180}
        }
    ]


@pytest.fixture
def council_one_failure():
    """Council with one model failing."""
    return [
        {
            "role": "correctness",
            "name": "Correctness Expert",
            "model": "openai/gpt-5.2:online",
            "content": "## Correctness Assessment\nNo bugs found.",
            "elapsed_ms": 2500,
            "tokens": {"input": 1500, "output": 200}
        },
        {
            "role": "security",
            "name": "Security Analyst",
            "model": "x-ai/grok-4:online",
            "content": "ERROR: Request timed out (after 3 retries)",
            "elapsed_ms": 180000,
            "error": "Request timed out"
        },
        {
            "role": "performance",
            "name": "Performance Critic",
            "model": "google/gemini-3-pro-preview:online",
            "content": "## Performance Assessment\nEfficient code.",
            "elapsed_ms": 2800,
            "tokens": {"input": 1500, "output": 180}
        }
    ]


# ============================================================================
# Temp File Fixtures
# ============================================================================

@pytest.fixture
def temp_context_file(tmp_path, sample_code_context):
    """Create a temporary context JSON file."""
    context_path = tmp_path / "context.json"
    context_path.write_text(json.dumps(sample_code_context, indent=2))
    return context_path


@pytest.fixture
def mock_skill_dir(tmp_path, monkeypatch):
    """Mock the skill directory for config loading."""
    skill_dir = tmp_path / ".claude" / "skills" / "h3"
    skill_dir.mkdir(parents=True)

    # Create default config
    config = {
        "model": "moonshotai/kimi-k2.5",
        "reasoning": "high",
        "max_context": 200000,
        "enable_web_search": True
    }
    (skill_dir / "config.json").write_text(json.dumps(config))

    # Create .env with API key
    (skill_dir / ".env").write_text("OPENROUTER_API_KEY=test-key\n")

    # Mock get_skill_dir to return our temp path
    def mock_get_skill_dir():
        return skill_dir

    return skill_dir, mock_get_skill_dir
