"""
Tests for review.py OpenRouter API communication.
"""

import json
import pytest
import responses
import sys
from pathlib import Path
from unittest.mock import patch, MagicMock

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from review import (
    call_openrouter,
    get_api_key,
    load_config,
    resolve_model,
    retry_with_backoff,
    estimate_cost,
    format_cost_estimate,
    OPENROUTER_URL,
    DEFAULT_CONFIG,
    MODEL_SHORTCUTS,
    MODEL_PRICING
)


class TestAPIKeyHandling:
    """Tests for API key management."""

    def test_get_api_key_success(self, mock_api_key):
        """Verify API key is retrieved from environment."""
        key = get_api_key()
        assert key == "test-api-key-12345"

    def test_get_api_key_missing_exits(self, mock_no_api_key, capsys, tmp_path):
        """Verify missing API key causes exit with helpful message."""
        # Mock skill dir to point to empty directory (no .env file)
        empty_dir = tmp_path / "empty_skill"
        empty_dir.mkdir()

        def mock_get_skill_dir():
            return empty_dir

        with patch("review.get_skill_dir", mock_get_skill_dir):
            with pytest.raises(SystemExit) as exc_info:
                get_api_key()

        assert exc_info.value.code == 1
        captured = capsys.readouterr()
        assert "OPENROUTER_API_KEY" in captured.out
        assert "openrouter.ai/keys" in captured.out


class TestConfigLoading:
    """Tests for configuration loading."""

    def test_load_config_with_file(self, mock_skill_dir):
        """Verify config loads from file when present."""
        skill_dir, mock_get = mock_skill_dir

        with patch("review.get_skill_dir", mock_get):
            config = load_config()

        # Note: tier is now in .env, not config.json
        assert config["model"] == "moonshotai/kimi-k2.5"
        assert config["reasoning"] == "high"

    def test_load_config_missing_file_uses_defaults(self, tmp_path):
        """Verify defaults used when config file missing."""
        empty_dir = tmp_path / "empty"
        empty_dir.mkdir()

        def mock_get():
            return empty_dir

        with patch("review.get_skill_dir", mock_get):
            config = load_config()

        assert config == DEFAULT_CONFIG

    def test_load_config_merges_with_defaults(self, mock_skill_dir):
        """Verify user config merges with defaults."""
        skill_dir, mock_get = mock_skill_dir

        # Add partial user config (missing some keys)
        # Note: tier is now stored in .env, not config.json
        partial_config = {"custom_key": "custom_value", "enable_web_search": True}
        (skill_dir / "config.json").write_text(json.dumps(partial_config))

        with patch("review.get_skill_dir", mock_get):
            config = load_config()

        # User value should override
        assert config["custom_key"] == "custom_value"
        assert config["enable_web_search"] is True
        # Default values should still be present
        assert config["model"] == DEFAULT_CONFIG["model"]
        assert config["reasoning"] == DEFAULT_CONFIG["reasoning"]


class TestModelResolution:
    """Tests for model shortcut resolution."""

    def test_resolve_gpt_shortcut(self, temp_config):
        """Verify 'gpt' resolves to GPT 5.2."""
        _, config = temp_config
        model = resolve_model("gpt", config)
        assert model == "openai/gpt-5.2"

    def test_resolve_deepseek_shortcut(self, temp_config):
        """Verify 'deepseek' resolves to DeepSeek V3.2."""
        _, config = temp_config
        model = resolve_model("deepseek", config)
        assert model == "deepseek/deepseek-v3.2"

    def test_resolve_free_uses_config(self, temp_config):
        """Verify 'free' uses free_model from config."""
        _, config = temp_config
        model = resolve_model("free", config)
        assert model == config["free_model"]

    def test_resolve_full_model_id_passthrough(self, temp_config):
        """Verify full model IDs pass through unchanged."""
        _, config = temp_config
        model = resolve_model("anthropic/claude-3-opus", config)
        assert model == "anthropic/claude-3-opus"

    def test_resolve_none_returns_none(self, temp_config):
        """Verify None input returns None."""
        _, config = temp_config
        model = resolve_model(None, config)
        assert model is None

    def test_resolve_case_insensitive(self, temp_config):
        """Verify shortcuts are case-insensitive."""
        _, config = temp_config
        assert resolve_model("GPT", config) == "openai/gpt-5.2"
        assert resolve_model("Gpt", config) == "openai/gpt-5.2"
        assert resolve_model("DEEPSEEK", config) == "deepseek/deepseek-v3.2"


class TestOpenRouterAPICall:
    """Tests for OpenRouter API calls."""

    @responses.activate
    def test_call_openrouter_success_non_streaming(
        self, mock_api_key, sample_code_context, openrouter_success_response
    ):
        """Verify successful non-streaming API call."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json=openrouter_success_response,
            status=200
        )

        config = {**DEFAULT_CONFIG, "model": "deepseek/deepseek-v3.2"}
        result = call_openrouter(config, "code", sample_code_context, stream=False)

        assert "## Summary" in result
        assert "Good code" in result
        assert "ERROR" not in result

    @responses.activate
    def test_call_openrouter_sends_correct_headers(self, mock_api_key, sample_code_context):
        """Verify correct headers are sent."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"choices": [{"message": {"content": "test"}}]},
            status=200
        )

        config = {**DEFAULT_CONFIG}
        call_openrouter(config, "code", sample_code_context, stream=False)

        assert len(responses.calls) == 1
        request = responses.calls[0].request

        assert "Bearer test-api-key-12345" in request.headers["Authorization"]
        assert request.headers["Content-Type"] == "application/json"
        assert "heavy3.ai" in request.headers["HTTP-Referer"]
        assert "Heavy3" in request.headers["X-Title"]

    @responses.activate
    def test_call_openrouter_sends_correct_payload(self, mock_api_key, sample_code_context):
        """Verify correct payload structure."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"choices": [{"message": {"content": "test"}}]},
            status=200
        )

        config = {**DEFAULT_CONFIG, "model": "deepseek/deepseek-v3.2", "reasoning": "high", "enable_web_search": False}
        call_openrouter(config, "code", sample_code_context, stream=False)

        request = responses.calls[0].request
        payload = json.loads(request.body)

        assert payload["model"] == "deepseek/deepseek-v3.2"
        assert len(payload["messages"]) == 2
        assert payload["messages"][0]["role"] == "system"
        assert payload["messages"][1]["role"] == "user"
        assert payload["stream"] == False

    @responses.activate
    def test_call_openrouter_with_web_search(self, mock_api_key, sample_code_context):
        """Verify web search enabled adds :online suffix."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"choices": [{"message": {"content": "test"}}]},
            status=200
        )

        config = {**DEFAULT_CONFIG, "enable_web_search": True}
        call_openrouter(config, "code", sample_code_context, stream=False)

        request = responses.calls[0].request
        payload = json.loads(request.body)

        # Model should have :online suffix
        assert ":online" in payload["model"]
        # Should have plugins
        assert "plugins" in payload
        assert payload["plugins"][0]["id"] == "web"

    @responses.activate
    def test_call_openrouter_gpt_reasoning(self, mock_api_key, sample_code_context):
        """Verify reasoning parameter added for GPT models."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"choices": [{"message": {"content": "test"}}]},
            status=200
        )

        config = {**DEFAULT_CONFIG, "model": "openai/gpt-5.2", "reasoning": "high"}
        call_openrouter(config, "code", sample_code_context, stream=False)

        request = responses.calls[0].request
        payload = json.loads(request.body)

        assert "reasoning" in payload
        assert payload["reasoning"]["effort"] == "high"

    @responses.activate
    def test_call_openrouter_non_gpt_no_reasoning(self, mock_api_key, sample_code_context):
        """Verify reasoning parameter NOT added for non-GPT models."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"choices": [{"message": {"content": "test"}}]},
            status=200
        )

        config = {**DEFAULT_CONFIG, "model": "deepseek/deepseek-v3.2", "reasoning": "high"}
        call_openrouter(config, "code", sample_code_context, stream=False)

        request = responses.calls[0].request
        payload = json.loads(request.body)

        # DeepSeek shouldn't have reasoning param
        assert "reasoning" not in payload

    @responses.activate
    def test_call_openrouter_truncates_large_context(self, mock_api_key, large_context):
        """Verify large context is truncated."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"choices": [{"message": {"content": "test"}}]},
            status=200
        )

        config = {**DEFAULT_CONFIG, "max_context": 1000}  # Very small for test
        call_openrouter(config, "code", large_context, stream=False)

        request = responses.calls[0].request
        payload = json.loads(request.body)
        user_message = payload["messages"][1]["content"]

        # Should be truncated
        assert len(user_message) <= 1100  # 1000 + truncation message
        assert "[... truncated" in user_message


class TestRetryLogic:
    """Tests for retry with exponential backoff."""

    @responses.activate
    def test_retry_on_500_error(self, mock_api_key, sample_code_context):
        """Verify retry on 500 server error."""
        # First call fails with 500, second succeeds
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"error": {"message": "Server error"}},
            status=500
        )
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"choices": [{"message": {"content": "success"}}]},
            status=200
        )

        config = {**DEFAULT_CONFIG}
        result = call_openrouter(config, "code", sample_code_context, stream=False)

        # Should have retried and succeeded
        assert len(responses.calls) == 2
        assert "success" in result

    @responses.activate
    def test_retry_on_429_rate_limit(self, mock_api_key, sample_code_context):
        """Verify retry on 429 rate limit."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"error": {"message": "Rate limited"}},
            status=429
        )
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"choices": [{"message": {"content": "success"}}]},
            status=200
        )

        config = {**DEFAULT_CONFIG}
        result = call_openrouter(config, "code", sample_code_context, stream=False)

        assert len(responses.calls) == 2
        assert "success" in result

    @responses.activate
    def test_no_retry_on_400_client_error(self, mock_api_key, sample_code_context):
        """Verify no retry on 400 client error."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"error": {"message": "Bad request"}},
            status=400
        )

        config = {**DEFAULT_CONFIG}
        result = call_openrouter(config, "code", sample_code_context, stream=False)

        # Should NOT retry on 400
        assert len(responses.calls) == 1
        assert "ERROR" in result

    @responses.activate
    def test_max_retries_exhausted(self, mock_api_key, sample_code_context):
        """Verify error after max retries exhausted."""
        # All calls fail
        for _ in range(3):
            responses.add(
                responses.POST,
                OPENROUTER_URL,
                json={"error": {"message": "Server error"}},
                status=500
            )

        config = {**DEFAULT_CONFIG}
        result = call_openrouter(config, "code", sample_code_context, stream=False)

        assert len(responses.calls) == 3  # MAX_RETRIES
        assert "ERROR" in result


class TestResponseFormats:
    """Tests for different response format handling."""

    @responses.activate
    def test_handles_empty_choices(self, mock_api_key, sample_code_context):
        """Verify handling of response with empty choices returns error string."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"choices": []},
            status=200
        )

        config = {**DEFAULT_CONFIG}
        # Now returns error string instead of raising exception
        result = call_openrouter(config, "code", sample_code_context, stream=False)
        assert "ERROR" in result
        assert "choices" in result.lower() or "response" in result.lower()

    @responses.activate
    def test_handles_missing_content(self, mock_api_key, sample_code_context):
        """Verify handling of response with missing content returns error string."""
        responses.add(
            responses.POST,
            OPENROUTER_URL,
            json={"choices": [{"message": {}}]},
            status=200
        )

        config = {**DEFAULT_CONFIG}
        # Now returns error string instead of raising exception
        result = call_openrouter(config, "code", sample_code_context, stream=False)
        assert "ERROR" in result
        assert "empty" in result.lower() or "content" in result.lower()


class TestCostEstimation:
    """Tests for cost estimation functions."""

    def test_estimate_cost_deepseek(self):
        """Estimate cost for DeepSeek model."""
        result = estimate_cost("deepseek/deepseek-v3.2", 40000, 2500)

        assert result["input_tokens"] == 10000  # 40000 chars / 4
        assert result["output_tokens"] == 2500
        assert result["model"] == "deepseek/deepseek-v3.2"
        # DeepSeek: $0.27/M input, $0.40/M output
        # 10000 * 0.27 / 1M = 0.0027, 2500 * 0.40 / 1M = 0.001
        assert abs(result["input_cost"] - 0.0027) < 0.0001
        assert abs(result["output_cost"] - 0.001) < 0.0001
        assert abs(result["total_cost"] - 0.0037) < 0.0001

    def test_estimate_cost_gpt(self):
        """Estimate cost for GPT model."""
        result = estimate_cost("openai/gpt-5.2", 40000, 2500)

        # GPT: $1.75/M input, $14.00/M output
        assert abs(result["input_cost"] - 0.0175) < 0.0001
        assert abs(result["output_cost"] - 0.035) < 0.0001

    def test_estimate_cost_online_suffix_stripped(self):
        """Online suffix stripped for pricing lookup."""
        result = estimate_cost("openai/gpt-5.2:online", 40000, 2500)

        # Should use same pricing as without :online
        assert result["model"] == "openai/gpt-5.2:online"
        assert abs(result["input_cost"] - 0.0175) < 0.0001

    def test_estimate_cost_free_model(self):
        """Free model has zero cost."""
        result = estimate_cost("nvidia/nemotron-3-nano-30b-a3b:free", 40000, 2500)

        assert result["input_cost"] == 0.0
        assert result["output_cost"] == 0.0
        assert result["total_cost"] == 0.0

    def test_estimate_cost_unknown_model_uses_default(self):
        """Unknown model uses default pricing."""
        result = estimate_cost("unknown/model", 40000, 2500)

        # Default: $0.50/M input, $1.00/M output
        assert abs(result["input_cost"] - 0.005) < 0.0001
        assert abs(result["output_cost"] - 0.0025) < 0.0001

    def test_estimate_cost_default_output_tokens(self):
        """Default output tokens is 2500."""
        result = estimate_cost("deepseek/deepseek-v3.2", 40000)
        assert result["output_tokens"] == 2500


class TestFormatCostEstimate:
    """Tests for cost estimate formatting."""

    def test_format_cost_estimate_basic(self):
        """Format cost estimate for display."""
        estimate = {
            "input_tokens": 10000,
            "output_tokens": 2500,
            "input_cost": 0.0027,
            "output_cost": 0.001,
            "total_cost": 0.0037,
            "model": "deepseek/deepseek-v3.2"
        }
        result = format_cost_estimate(estimate)

        assert "40K chars" in result  # 10000 * 4 = 40000
        assert "10K" in result  # input tokens
        assert "deepseek/deepseek-v3.2" in result
        assert "$0.0037" in result

    def test_format_cost_estimate_includes_model(self):
        """Format includes model name."""
        estimate = {
            "input_tokens": 5000,
            "output_tokens": 2500,
            "input_cost": 0.01,
            "output_cost": 0.035,
            "total_cost": 0.045,
            "model": "openai/gpt-5.2"
        }
        result = format_cost_estimate(estimate)

        assert "openai/gpt-5.2" in result


