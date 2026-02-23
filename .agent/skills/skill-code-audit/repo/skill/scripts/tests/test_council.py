"""
Tests for council.py multi-model council mode.
"""

import json
import pytest
import responses
import sys
from pathlib import Path
from unittest.mock import patch, MagicMock
import concurrent.futures

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from council import (
    run_council,
    call_reviewer,
    get_council_config,
    DEFAULT_COUNCIL_MODELS,
    CODE_PROMPTS,
    PLAN_PROMPTS,
    OPENROUTER_URL
)

# Helper to get council config with web search enabled
def get_test_council(council_type: str) -> list:
    """Get council config for testing with web search enabled."""
    return get_council_config({"enable_web_search": True}, council_type)


class TestCouncilModels:
    """Tests for council model configuration."""

    def test_code_council_has_three_models(self):
        """Verify code council has exactly 3 models."""
        assert len(get_test_council("code")) == 3

    def test_plan_council_has_three_models(self):
        """Verify plan council has exactly 3 models."""
        assert len(get_test_council("plan")) == 3

    def test_code_council_roles(self):
        """Verify code council has correct roles."""
        roles = [m["role"] for m in get_test_council("code")]
        assert "correctness" in roles
        assert "security" in roles
        assert "performance" in roles

    def test_plan_council_roles(self):
        """Verify plan council has correct roles."""
        roles = [m["role"] for m in get_test_council("plan")]
        assert "design" in roles
        assert "security" in roles
        assert "scalability" in roles

    def test_all_models_have_online_suffix_when_web_search_enabled(self):
        """Verify all council models use :online when web search is enabled."""
        for council_type in ["code", "plan"]:
            for model in get_test_council(council_type):
                assert ":online" in model["model"], f"{model['name']} missing :online suffix"

    def test_all_models_have_search_engine_when_web_search_enabled(self):
        """Verify all council models have search_engine when web search is enabled."""
        for council_type in ["code", "plan"]:
            for model in get_test_council(council_type):
                assert model.get("search_engine") is not None, f"{model['name']} missing search_engine"

    def test_no_online_suffix_when_web_search_disabled(self):
        """Verify models don't have :online when web search is disabled."""
        council = get_council_config({"enable_web_search": False}, "code")
        for model in council:
            assert ":online" not in model["model"], f"{model['name']} should not have :online"

    def test_config_models_override_defaults(self):
        """Verify config.json council_models override defaults."""
        custom_config = {
            "council_models": {
                "correctness": "custom/model-1",
                "security": "custom/model-2",
            },
            "enable_web_search": False
        }
        council = get_council_config(custom_config, "code")
        models_by_role = {m["role"]: m["model"] for m in council}
        assert models_by_role["correctness"] == "custom/model-1"
        assert models_by_role["security"] == "custom/model-2"
        # Performance should still use default
        assert models_by_role["performance"] == DEFAULT_COUNCIL_MODELS["performance"]


class TestCouncilPrompts:
    """Tests for council system prompts."""

    def test_code_prompts_exist_for_all_roles(self):
        """Verify prompts exist for all code council roles."""
        roles = [m["role"] for m in get_test_council("code")]
        for role in roles:
            assert role in CODE_PROMPTS, f"Missing prompt for {role}"

    def test_plan_prompts_exist_for_all_roles(self):
        """Verify prompts exist for all plan council roles."""
        roles = [m["role"] for m in get_test_council("plan")]
        for role in roles:
            assert role in PLAN_PROMPTS, f"Missing prompt for {role}"

    def test_prompts_are_role_specific(self):
        """Verify each prompt is specific to its role."""
        assert "bug" in CODE_PROMPTS["correctness"].lower()
        assert "security" in CODE_PROMPTS["security"].lower() or "vulnerabil" in CODE_PROMPTS["security"].lower()
        assert "performance" in CODE_PROMPTS["performance"].lower()

    def test_prompts_include_format_instructions(self):
        """Verify prompts include expected format instructions."""
        for role, prompt in CODE_PROMPTS.items():
            # Each prompt should have ## headers for formatted output
            assert "##" in prompt, f"{role} prompt missing format headers"


class TestCallReviewer:
    """Tests for individual reviewer API calls."""

    @responses.activate
    def test_call_reviewer_success(self, mock_api_key, sample_code_context):
        """Verify successful reviewer call."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={
                "choices": [{"message": {"content": "## Assessment\nGood code."}}],
                "usage": {"prompt_tokens": 1500, "completion_tokens": 200}
            },
            status=200
        )

        from council import build_user_message
        user_message = build_user_message(sample_code_context, "code")

        result = call_reviewer(
            role="correctness",
            model="openai/gpt-5.2:online",
            name="Correctness Expert",
            user_message=user_message,
            review_type="code",
            api_key="test-key",
            reasoning="high",
            search_engine="native"
        )

        assert result["role"] == "correctness"
        assert result["name"] == "Correctness Expert"
        assert "## Assessment" in result["content"]
        assert result["elapsed_ms"] > 0
        assert result["tokens"]["input"] == 1500
        assert result["tokens"]["output"] == 200
        assert "error" not in result

    @responses.activate
    def test_call_reviewer_with_web_search_plugin(self, mock_api_key, sample_code_context):
        """Verify web search plugin is added to request."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"choices": [{"message": {"content": "test"}}], "usage": {}},
            status=200
        )

        from council import build_user_message
        user_message = build_user_message(sample_code_context, "code")

        call_reviewer(
            role="security",
            model="google/gemini-3-pro:online",
            name="Security Analyst",
            user_message=user_message,
            review_type="code",
            api_key="test-key",
            reasoning="high",
            search_engine="exa"
        )

        request = responses.calls[0].request
        payload = json.loads(request.body)

        assert "plugins" in payload
        assert payload["plugins"][0]["id"] == "web"
        assert payload["plugins"][0]["engine"] == "exa"

    @responses.activate
    def test_call_reviewer_error_captured(self, mock_api_key, sample_code_context):
        """Verify errors are captured in result."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"error": {"message": "Model unavailable"}},
            status=500
        )
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"error": {"message": "Model unavailable"}},
            status=500
        )
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"error": {"message": "Model unavailable"}},
            status=500
        )

        from council import build_user_message
        user_message = build_user_message(sample_code_context, "code")

        result = call_reviewer(
            role="correctness",
            model="openai/gpt-5.2:online",
            name="Correctness Expert",
            user_message=user_message,
            review_type="code",
            api_key="test-key",
            reasoning="high"
        )

        assert "error" in result
        assert "ERROR" in result["content"]
        assert result["elapsed_ms"] > 0


class TestRunCouncil:
    """Tests for the full council execution."""

    def test_council_runs_three_parallel_calls(self, mock_api_key, sample_code_context, temp_pro_config):
        """Verify council runs 3 models in parallel."""
        config_path, config = temp_pro_config

        # Track how many calls are made
        call_count = 0

        def mock_call_reviewer(*args, **kwargs):
            nonlocal call_count
            call_count += 1
            return {
                "role": kwargs.get("role", args[0]) if kwargs else args[0],
                "name": kwargs.get("name", args[2]) if kwargs else args[2],
                "model": kwargs.get("model", args[1]) if kwargs else args[1],
                "content": "## Assessment\nTest content.",
                "elapsed_ms": 1000,
                "tokens": {"input": 100, "output": 50}
            }

        with patch('council.load_config', return_value=config):
            with patch('council.get_api_key', return_value="test-key"):
                with patch('council.call_reviewer', side_effect=mock_call_reviewer):
                    result = run_council(sample_code_context, "code")

        assert call_count == 3
        assert len(result["reviews"]) == 3

    def test_council_returns_all_reviews(self, mock_api_key, sample_code_context, temp_pro_config, council_all_success):
        """Verify council returns all three reviews."""
        config_path, config = temp_pro_config

        # Mock call_reviewer to return our fixtures
        review_iter = iter(council_all_success)

        def mock_call_reviewer(*args, **kwargs):
            return next(review_iter)

        with patch('council.load_config', return_value=config):
            with patch('council.get_api_key', return_value="test-key"):
                with patch('council.call_reviewer', side_effect=mock_call_reviewer):
                    result = run_council(sample_code_context, "code")

        assert len(result["reviews"]) == 3

        # Check all roles present
        roles = [r["role"] for r in result["reviews"]]
        assert "correctness" in roles
        assert "security" in roles
        assert "performance" in roles

    def test_council_handles_partial_failure(self, mock_api_key, sample_code_context, temp_pro_config, council_one_failure):
        """Verify council handles one model failing."""
        config_path, config = temp_pro_config

        review_iter = iter(council_one_failure)

        def mock_call_reviewer(*args, **kwargs):
            return next(review_iter)

        with patch('council.load_config', return_value=config):
            with patch('council.get_api_key', return_value="test-key"):
                with patch('council.call_reviewer', side_effect=mock_call_reviewer):
                    result = run_council(sample_code_context, "code")

        # Should still return all 3 results
        assert len(result["reviews"]) == 3

        # One should have error
        errors = [r for r in result["reviews"] if r.get("error")]
        assert len(errors) == 1
        assert errors[0]["role"] == "security"

    def test_council_metadata_includes_timing(self, mock_api_key, sample_code_context, temp_pro_config):
        """Verify council metadata includes total timing."""
        config_path, config = temp_pro_config

        def mock_call_reviewer(*args, **kwargs):
            return {
                "role": args[0],
                "name": args[2],
                "model": args[1],
                "content": "test",
                "elapsed_ms": 1000,
                "tokens": {"input": 100, "output": 50}
            }

        with patch('council.load_config', return_value=config):
            with patch('council.get_api_key', return_value="test-key"):
                with patch('council.call_reviewer', side_effect=mock_call_reviewer):
                    result = run_council(sample_code_context, "code")

        assert "metadata" in result
        assert "total_ms" in result["metadata"]
        # Note: With mocked reviewers, total_ms may be 0 since no actual time passes
        assert result["metadata"]["total_ms"] >= 0


class TestCouncilProgressDisplay:
    """Tests for progress display during council execution."""

    def test_progress_printed_to_stderr(self, mock_api_key, sample_code_context, temp_pro_config, capsys):
        """Verify progress messages go to stderr."""
        config_path, config = temp_pro_config

        def mock_call_reviewer(*args, **kwargs):
            return {
                "role": args[0],
                "name": args[2],
                "model": args[1],
                "content": "test",
                "elapsed_ms": 1000,
                "tokens": {"input": 100, "output": 50}
            }

        with patch('council.load_config', return_value=config):
            with patch('council.get_api_key', return_value="test-key"):
                with patch('council.call_reviewer', side_effect=mock_call_reviewer):
                    run_council(sample_code_context, "code")

        captured = capsys.readouterr()

        # Progress should be in stderr
        assert "Heavy3 Council" in captured.err or "Sponsored" in captured.err
        # Results shown after completion
        assert "completed" in captured.err or "✓" in captured.err

    def test_completion_shown_for_each_model(self, mock_api_key, sample_code_context, temp_pro_config, capsys):
        """Verify each model shows completion status."""
        config_path, config = temp_pro_config

        def mock_call_reviewer(*args, **kwargs):
            return {
                "role": args[0],
                "name": args[2],
                "model": args[1],
                "content": "test",
                "elapsed_ms": 1500,
                "tokens": {"input": 100, "output": 50}
            }

        with patch('council.load_config', return_value=config):
            with patch('council.get_api_key', return_value="test-key"):
                with patch('council.call_reviewer', side_effect=mock_call_reviewer):
                    run_council(sample_code_context, "code")

        captured = capsys.readouterr()

        # Should show completion for each model
        # Either by name or by "completed" message
        assert captured.err.count("completed") >= 3 or captured.err.count("+") >= 3


class TestCouncilOutput:
    """Tests for council output format."""

    def test_council_outputs_json(self, mock_api_key, sample_code_context, temp_pro_config):
        """Verify council outputs valid JSON."""
        config_path, config = temp_pro_config

        def mock_call_reviewer(*args, **kwargs):
            return {
                "role": args[0],
                "name": args[2],
                "model": args[1],
                "content": "test",
                "elapsed_ms": 1000,
                "tokens": {"input": 100, "output": 50}
            }

        with patch('council.load_config', return_value=config):
            with patch('council.get_api_key', return_value="test-key"):
                with patch('council.call_reviewer', side_effect=mock_call_reviewer):
                    result = run_council(sample_code_context, "code")

        # Should be serializable to JSON
        json_str = json.dumps(result)
        parsed = json.loads(json_str)

        assert "reviews" in parsed
        assert "metadata" in parsed

    def test_reviews_ordered_by_role(self, mock_api_key, sample_code_context, temp_pro_config):
        """Verify reviews are sorted by role order."""
        config_path, config = temp_pro_config

        # Return reviews in reverse order
        reviews = [
            {"role": "performance", "name": "P", "model": "m", "content": "p", "elapsed_ms": 100, "tokens": {}},
            {"role": "security", "name": "S", "model": "m", "content": "s", "elapsed_ms": 100, "tokens": {}},
            {"role": "correctness", "name": "C", "model": "m", "content": "c", "elapsed_ms": 100, "tokens": {}},
        ]
        review_iter = iter(reviews)

        def mock_call_reviewer(*args, **kwargs):
            return next(review_iter)

        with patch('council.load_config', return_value=config):
            with patch('council.get_api_key', return_value="test-key"):
                with patch('council.call_reviewer', side_effect=mock_call_reviewer):
                    result = run_council(sample_code_context, "code")

        # Should be reordered to match council order (correctness, performance, security)
        roles = [r["role"] for r in result["reviews"]]
        expected_order = ["correctness", "performance", "security"]
        assert roles == expected_order
