"""
Tests for list-free-models.py - Free model discovery utility.
Covers free model detection, thinking model classification, and date parsing.
"""

import pytest
import sys
import importlib
from datetime import datetime
from pathlib import Path
from unittest.mock import patch, MagicMock

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

# Import the module with hyphen in name
list_free_models = importlib.import_module("list-free-models")


# ============================================================================
# Test: is_free_model
# ============================================================================

class TestIsFreeModel:
    """Tests for free model detection."""

    def test_free_model_string_zeros(self):
        """Model with string '0' pricing is free."""
        model = {"pricing": {"prompt": "0", "completion": "0"}}
        assert list_free_models.is_free_model(model) is True

    def test_free_model_numeric_zeros(self):
        """Model with numeric 0 pricing is free."""
        model = {"pricing": {"prompt": 0, "completion": 0}}
        assert list_free_models.is_free_model(model) is True

    def test_free_model_float_zeros(self):
        """Model with float 0.0 pricing is free."""
        model = {"pricing": {"prompt": 0.0, "completion": 0.0}}
        assert list_free_models.is_free_model(model) is True

    def test_paid_model_both_nonzero(self):
        """Model with non-zero pricing is not free."""
        model = {"pricing": {"prompt": "0.001", "completion": "0.002"}}
        assert list_free_models.is_free_model(model) is False

    def test_paid_model_prompt_only(self):
        """Model with only prompt cost is not free."""
        model = {"pricing": {"prompt": "0.01", "completion": "0"}}
        assert list_free_models.is_free_model(model) is False

    def test_paid_model_completion_only(self):
        """Model with only completion cost is not free."""
        model = {"pricing": {"prompt": "0", "completion": "0.01"}}
        assert list_free_models.is_free_model(model) is False

    def test_missing_pricing_key(self):
        """Model without pricing key defaults to free (0 cost)."""
        model = {}
        # Implementation defaults missing pricing to "0" (free)
        assert list_free_models.is_free_model(model) is True

    def test_empty_pricing_dict(self):
        """Model with empty pricing dict defaults to free (0 cost)."""
        model = {"pricing": {}}
        # Implementation defaults missing prompt/completion to "0" (free)
        assert list_free_models.is_free_model(model) is True

    def test_none_pricing_values(self):
        """Model with None pricing values."""
        model = {"pricing": {"prompt": None, "completion": None}}
        # None should be treated as 0 based on the implementation
        assert list_free_models.is_free_model(model) is True

    def test_invalid_pricing_string(self):
        """Model with invalid pricing string should return False."""
        model = {"pricing": {"prompt": "invalid", "completion": "0"}}
        assert list_free_models.is_free_model(model) is False


# ============================================================================
# Test: is_thinking_model
# ============================================================================

class TestIsThinkingModel:
    """Tests for thinking/reasoning model classification."""

    # THINKING models
    def test_r1_in_id(self):
        """Models with 'r1' in ID are thinking models."""
        result = list_free_models.is_thinking_model("deepseek/deepseek-r1", "DeepSeek R1")
        assert result == "THINKING"

    def test_o1_in_id(self):
        """Models with 'o1' in ID are thinking models."""
        result = list_free_models.is_thinking_model("openai/o1-preview", "O1 Preview")
        assert result == "THINKING"

    def test_o3_in_id(self):
        """Models with 'o3' in ID are thinking models."""
        result = list_free_models.is_thinking_model("openai/o3-mini", "O3 Mini")
        assert result == "THINKING"

    def test_thinking_in_name(self):
        """Models with 'thinking' in name are thinking models."""
        result = list_free_models.is_thinking_model("some/model", "Deep Thinking Model")
        assert result == "THINKING"

    def test_reasoner_in_name(self):
        """Models with 'reasoner' in name are thinking models."""
        result = list_free_models.is_thinking_model("some/model", "Advanced Reasoner")
        assert result == "THINKING"

    def test_cot_in_id(self):
        """Models with 'cot' (chain of thought) are thinking models."""
        result = list_free_models.is_thinking_model("some/cot-model", "Chain of Thought")
        assert result == "THINKING"

    # STANDARD models
    def test_instruct_model(self):
        """Models with 'instruct' are standard."""
        result = list_free_models.is_thinking_model("mistral/mistral-instruct", "Mistral Instruct")
        assert result == "STANDARD"

    def test_chat_model(self):
        """Models with 'chat' are standard."""
        result = list_free_models.is_thinking_model("openai/gpt-4-chat", "GPT-4 Chat")
        assert result == "STANDARD"

    def test_fast_model(self):
        """Models with 'fast' are standard."""
        result = list_free_models.is_thinking_model("some/fast-model", "Fast Model")
        assert result == "STANDARD"

    def test_turbo_model(self):
        """Models with 'turbo' are standard."""
        result = list_free_models.is_thinking_model("openai/gpt-4-turbo", "GPT-4 Turbo")
        assert result == "STANDARD"

    def test_mini_model(self):
        """Models with 'mini' are standard."""
        result = list_free_models.is_thinking_model("some/model-mini", "Model Mini")
        assert result == "STANDARD"

    def test_lite_model(self):
        """Models with 'lite' are standard."""
        result = list_free_models.is_thinking_model("some/model-lite", "Model Lite")
        assert result == "STANDARD"

    # UNKNOWN models
    def test_unknown_model(self):
        """Models without clear indicators are unknown."""
        result = list_free_models.is_thinking_model("random/model-v2", "Random Model V2")
        assert result == "UNKNOWN"

    def test_empty_strings(self):
        """Empty model ID and name should return unknown."""
        result = list_free_models.is_thinking_model("", "")
        assert result == "UNKNOWN"


# ============================================================================
# Test: parse_date
# ============================================================================

class TestParseDate:
    """Tests for date parsing."""

    def test_iso_format_with_z(self):
        """Parse ISO 8601 date with Z suffix."""
        result = list_free_models.parse_date("2025-01-15T12:00:00Z")
        assert result.year == 2025
        assert result.month == 1
        assert result.day == 15

    def test_iso_format_with_offset(self):
        """Parse ISO 8601 date with timezone offset."""
        result = list_free_models.parse_date("2025-06-20T08:30:00+00:00")
        assert result.year == 2025
        assert result.month == 6
        assert result.day == 20

    def test_empty_string(self):
        """Empty string returns datetime.min."""
        result = list_free_models.parse_date("")
        assert result == datetime.min

    def test_none_value(self):
        """None returns datetime.min."""
        result = list_free_models.parse_date(None)
        assert result == datetime.min

    def test_invalid_format(self):
        """Invalid format returns datetime.min."""
        result = list_free_models.parse_date("not-a-date")
        assert result == datetime.min

    def test_partial_date(self):
        """Partial date format returns datetime.min."""
        result = list_free_models.parse_date("2025-01")
        assert result == datetime.min


# ============================================================================
# Test: format_date
# ============================================================================

class TestFormatDate:
    """Tests for date formatting."""

    def test_format_valid_date(self):
        """Format valid ISO date."""
        result = list_free_models.format_date("2025-01-15T12:00:00Z")
        assert result == "2025-01-15"

    def test_format_invalid_date(self):
        """Format invalid date returns dash."""
        result = list_free_models.format_date("invalid")
        assert result == "-"

    def test_format_empty_string(self):
        """Format empty string returns dash."""
        result = list_free_models.format_date("")
        assert result == "-"

    def test_format_none(self):
        """Format None returns dash."""
        result = list_free_models.format_date(None)
        assert result == "-"


# ============================================================================
# Test: fetch_models (mocked)
# ============================================================================

class TestFetchModels:
    """Tests for API fetching (mocked)."""

    @patch.object(list_free_models, 'requests')
    def test_fetch_models_success(self, mock_requests):
        """Successful API response."""
        mock_response = MagicMock()
        mock_response.json.return_value = {"data": [{"id": "test/model"}]}
        mock_response.raise_for_status = MagicMock()
        mock_requests.get.return_value = mock_response

        result = list_free_models.fetch_models()
        assert len(result) == 1
        assert result[0]["id"] == "test/model"

    @patch.object(list_free_models, 'requests')
    def test_fetch_models_empty_data(self, mock_requests):
        """API response with empty data array."""
        mock_response = MagicMock()
        mock_response.json.return_value = {"data": []}
        mock_response.raise_for_status = MagicMock()
        mock_requests.get.return_value = mock_response

        result = list_free_models.fetch_models()
        assert result == []

    @patch.object(list_free_models, 'requests')
    def test_fetch_models_multiple(self, mock_requests):
        """API response with multiple models."""
        mock_response = MagicMock()
        mock_response.json.return_value = {
            "data": [
                {"id": "model/a"},
                {"id": "model/b"},
                {"id": "model/c"}
            ]
        }
        mock_response.raise_for_status = MagicMock()
        mock_requests.get.return_value = mock_response

        result = list_free_models.fetch_models()
        assert len(result) == 3


# ============================================================================
# Test: Constants
# ============================================================================

class TestConstants:
    """Tests for module constants."""

    def test_openrouter_url(self):
        """Verify OpenRouter API URL is correct."""
        assert "openrouter.ai" in list_free_models.OPENROUTER_MODELS_URL

    def test_thinking_keywords_exist(self):
        """Verify thinking keywords list exists and has entries."""
        assert len(list_free_models.THINKING_KEYWORDS) > 0
        assert "thinking" in list_free_models.THINKING_KEYWORDS
        assert "r1" in list_free_models.THINKING_KEYWORDS

    def test_non_thinking_keywords_exist(self):
        """Verify non-thinking keywords list exists and has entries."""
        assert len(list_free_models.NON_THINKING_KEYWORDS) > 0
        assert "instruct" in list_free_models.NON_THINKING_KEYWORDS
        assert "fast" in list_free_models.NON_THINKING_KEYWORDS
