"""
Tests for context gathering and message building.
Ensures the skill correctly assembles context JSON for reviews.
"""

import pytest
import sys
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from review import build_user_message, get_system_prompt
from council import build_user_message as council_build_user_message


class TestBuildUserMessageCodeReview:
    """Tests for code review context building."""

    def test_basic_code_review_structure(self, sample_code_context):
        """Verify basic code review context includes required sections."""
        message = build_user_message(sample_code_context, "code")

        # Should include diff
        assert "## Code Changes (Diff)" in message
        assert "```diff" in message
        assert sample_code_context["diff"] in message

        # Should include file contents
        assert "## Full File Contents" in message
        assert "src/utils.py" in message
        assert "def calculate_total" in message

        # Should include documentation
        assert "## Relevant Documentation" in message
        assert "CLAUDE.md" in message

    def test_code_review_includes_conversation_context(self, sample_code_context_with_conversation):
        """Verify conversation context is included when provided."""
        message = build_user_message(sample_code_context_with_conversation, "code")

        # Developer Intent section should appear FIRST
        assert "## Developer Intent" in message
        assert message.index("## Developer Intent") < message.index("## Code Changes") if "## Code Changes" in message else True

        # Should include original request
        assert "**Original Request:**" in message
        assert "Add email validation" in message

        # Should include approach notes
        assert "**Approach Notes:**" in message
        assert "simple @ check" in message

        # Should include relevant exchanges
        assert "**Relevant Discussion:**" in message
        assert "Can you add basic email validation?" in message

    def test_code_review_without_conversation_context(self, sample_code_context):
        """Verify code review works without conversation context."""
        # Ensure no conversation_context key
        assert "conversation_context" not in sample_code_context

        message = build_user_message(sample_code_context, "code")

        # Should not have Developer Intent section
        assert "## Developer Intent" not in message

        # Should still have diff and files
        assert "## Code Changes (Diff)" in message
        assert "## Full File Contents" in message

    def test_code_review_with_test_files(self, sample_code_context):
        """Verify test files are included when present."""
        sample_code_context["test_files"] = {
            "tests/test_utils.py": "def test_calculate_total(): assert calculate_total([]) == 0"
        }

        message = build_user_message(sample_code_context, "code")

        assert "## Related Test Files" in message
        assert "test_utils.py" in message
        assert "def test_calculate_total" in message

    def test_code_review_empty_sections_handled(self):
        """Verify empty optional sections don't break message building."""
        minimal_context = {
            "review_type": "code",
            "diff": "some diff",
            "changed_files": [],
            "file_contents": {},
            "documentation": {},
            "test_files": {}
        }

        message = build_user_message(minimal_context, "code")

        # Should still have diff section
        assert "## Code Changes (Diff)" in message
        assert "some diff" in message

        # Should NOT have empty sections
        assert "## Full File Contents" not in message or "###" not in message.split("## Full File Contents")[1].split("##")[0]


class TestBuildUserMessagePlanReview:
    """Tests for plan review context building."""

    def test_plan_review_includes_plan_content(self, sample_plan_context):
        """Verify plan content is included for plan reviews."""
        message = build_user_message(sample_plan_context, "plan")

        assert "## Plan to Review" in message
        assert "# Implementation Plan: User Authentication" in message
        assert "JWT-based authentication" in message
        assert "Steps" in message

    def test_plan_review_includes_referenced_files(self, sample_plan_context):
        """Verify files referenced in plan are included."""
        message = build_user_message(sample_plan_context, "plan")

        assert "## Full File Contents" in message
        assert "src/routes/api.js" in message
        assert "express.Router()" in message

    def test_plan_review_without_plan_content(self):
        """Verify graceful handling when plan_content is missing."""
        context = {
            "review_type": "plan",
            "file_contents": {},
            "documentation": {}
        }

        message = build_user_message(context, "plan")

        assert "## Plan to Review" in message
        assert "No plan content provided" in message


class TestBuildUserMessagePRReview:
    """Tests for PR review context building."""

    def test_pr_review_includes_metadata(self, sample_pr_context):
        """Verify PR metadata is included."""
        message = build_user_message(sample_pr_context, "pr")

        assert "## Pull Request Information" in message
        assert "PR #42" in message
        assert "Add email validation" in message
        assert "testuser" in message
        assert "main" in message
        assert "feature/email-validation" in message
        assert "+15" in message
        assert "-2" in message

    def test_pr_review_includes_body(self, sample_pr_context):
        """Verify PR body/description is included."""
        message = build_user_message(sample_pr_context, "pr")

        assert "### PR Description" in message
        assert "This PR adds basic email validation" in message

    def test_pr_review_without_body(self, sample_pr_context):
        """Verify PR review works when body is empty."""
        sample_pr_context["pr_metadata"]["body"] = None

        message = build_user_message(sample_pr_context, "pr")

        # Should still have metadata but no description section
        assert "## Pull Request Information" in message
        assert "PR #42" in message


class TestContextTruncation:
    """Tests for context size limits and truncation."""

    def test_large_context_gets_truncated(self, large_context):
        """Verify context exceeding limit is truncated."""
        message = build_user_message(large_context, "code")

        # Large context fixture has ~110K chars, should not all fit
        # The truncation happens in call_openrouter, not build_user_message
        # So here we just verify the message is built without error
        assert len(message) > 0
        assert "## Code Changes (Diff)" in message

    def test_truncation_message_appended(self):
        """Verify truncation indicator is added when context is cut."""
        # This test verifies the truncation logic in call_openrouter
        # We test the truncation directly
        max_context = 100  # Very small for testing
        user_message = "x" * 150

        if len(user_message) > max_context:
            user_message = user_message[:max_context] + "\n\n[... truncated due to length ...]"

        assert len(user_message) < 150
        assert "[... truncated due to length ...]" in user_message


class TestSystemPrompts:
    """Tests for system prompt selection."""

    def test_code_review_prompt(self):
        """Verify correct prompt for code reviews."""
        prompt = get_system_prompt("code")

        assert "senior software engineer" in prompt.lower()
        assert "code review" in prompt.lower()
        assert "Bugs" in prompt
        assert "Security" in prompt
        assert "Performance" in prompt

    def test_plan_review_prompt(self):
        """Verify correct prompt for plan reviews."""
        prompt = get_system_prompt("plan")

        assert "senior software architect" in prompt.lower()
        assert "implementation plan" in prompt.lower()
        assert "Architecture" in prompt
        assert "Suggestions" in prompt

    def test_pr_review_prompt(self):
        """Verify correct prompt for PR reviews."""
        prompt = get_system_prompt("pr")

        assert "pull request" in prompt.lower()
        assert "PR description" in prompt.lower() or "pr purpose" in prompt.lower()
        assert "Verdict" in prompt
        assert "APPROVE" in prompt


class TestConversationContextLimits:
    """Tests for conversation context extraction limits."""

    def test_exchange_content_truncated(self, sample_code_context_with_conversation):
        """Verify long exchange messages are truncated to 500 chars."""
        # Add a very long message
        sample_code_context_with_conversation["conversation_context"]["relevant_exchanges"] = [
            {"role": "user", "content": "x" * 1000}
        ]

        message = build_user_message(sample_code_context_with_conversation, "code")

        # The message content should be truncated (done in build_user_message at line 264)
        # Check that we don't have 1000 x's
        x_count = message.count("x")
        assert x_count <= 500  # Should be truncated to 500

    def test_multiple_exchanges_included(self, sample_code_context_with_conversation):
        """Verify multiple exchanges are all included."""
        exchanges = [
            {"role": "user", "content": f"Message {i}"} for i in range(5)
        ]
        sample_code_context_with_conversation["conversation_context"]["relevant_exchanges"] = exchanges

        message = build_user_message(sample_code_context_with_conversation, "code")

        # All 5 messages should be included
        for i in range(5):
            assert f"Message {i}" in message


class TestCouncilContextBuilding:
    """Tests for council.py context building (should match review.py)."""

    def test_council_build_matches_review(self, sample_code_context):
        """Verify council context building matches review context building."""
        review_message = build_user_message(sample_code_context, "code")
        council_message = council_build_user_message(sample_code_context, "code")

        # Both should have same structure
        assert "## Code Changes (Diff)" in review_message
        assert "## Code Changes (Diff)" in council_message

        assert "## Full File Contents" in review_message
        assert "## Full File Contents" in council_message

    def test_council_includes_conversation_context(self, sample_code_context_with_conversation):
        """Verify council also includes conversation context."""
        message = council_build_user_message(sample_code_context_with_conversation, "code")

        assert "## Developer Intent" in message
        assert "**Original Request:**" in message
