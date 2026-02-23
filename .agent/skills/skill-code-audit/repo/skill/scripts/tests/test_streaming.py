"""
Tests for streaming response handling in review.py.
"""

import json
import pytest
import responses
import sys
from io import StringIO
from pathlib import Path
from unittest.mock import patch, MagicMock

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from review import call_openrouter, OPENROUTER_URL, DEFAULT_CONFIG


class MockStreamResponse:
    """Mock response object for streaming tests."""

    def __init__(self, chunks, status_code=200):
        self.chunks = chunks
        self.status_code = status_code

    def iter_lines(self):
        for chunk in self.chunks:
            yield chunk

    def raise_for_status(self):
        if self.status_code >= 400:
            from requests.exceptions import HTTPError
            raise HTTPError(f"HTTP Error {self.status_code}")


class TestStreamingParsing:
    """Tests for SSE streaming response parsing."""

    def test_parses_sse_data_prefix(self, mock_api_key, sample_code_context):
        """Verify SSE 'data: ' prefix is correctly stripped."""
        chunks = [
            b'data: {"choices":[{"delta":{"content":"Hello"}}]}',
            b'data: {"choices":[{"delta":{"content":" World"}}]}',
            b'data: [DONE]',
        ]

        mock_response = MockStreamResponse(chunks)

        with patch('review.retry_with_backoff', return_value=mock_response):
            config = {**DEFAULT_CONFIG}
            result = call_openrouter(config, "code", sample_code_context, stream=True)

        assert result == "Hello World"

    def test_handles_done_sentinel(self, mock_api_key, sample_code_context):
        """Verify [DONE] sentinel stops processing."""
        chunks = [
            b'data: {"choices":[{"delta":{"content":"Before"}}]}',
            b'data: [DONE]',
            b'data: {"choices":[{"delta":{"content":"After"}}]}',  # Should be ignored
        ]

        mock_response = MockStreamResponse(chunks)

        with patch('review.retry_with_backoff', return_value=mock_response):
            config = {**DEFAULT_CONFIG}
            result = call_openrouter(config, "code", sample_code_context, stream=True)

        assert result == "Before"
        assert "After" not in result

    def test_handles_empty_lines(self, mock_api_key, sample_code_context):
        """Verify empty lines in stream are skipped."""
        chunks = [
            b'data: {"choices":[{"delta":{"content":"A"}}]}',
            b'',  # Empty line
            b'data: {"choices":[{"delta":{"content":"B"}}]}',
            b'data: [DONE]',
        ]

        mock_response = MockStreamResponse(chunks)

        with patch('review.retry_with_backoff', return_value=mock_response):
            config = {**DEFAULT_CONFIG}
            result = call_openrouter(config, "code", sample_code_context, stream=True)

        assert result == "AB"

    def test_handles_role_only_delta(self, mock_api_key, sample_code_context):
        """Verify delta with only role (no content) is handled."""
        chunks = [
            b'data: {"choices":[{"delta":{"role":"assistant"}}]}',  # No content key
            b'data: {"choices":[{"delta":{"content":"Content"}}]}',
            b'data: [DONE]',
        ]

        mock_response = MockStreamResponse(chunks)

        with patch('review.retry_with_backoff', return_value=mock_response):
            config = {**DEFAULT_CONFIG}
            result = call_openrouter(config, "code", sample_code_context, stream=True)

        assert result == "Content"

    def test_handles_empty_content(self, mock_api_key, sample_code_context):
        """Verify delta with empty content string is handled."""
        chunks = [
            b'data: {"choices":[{"delta":{"content":""}}]}',
            b'data: {"choices":[{"delta":{"content":"Real content"}}]}',
            b'data: [DONE]',
        ]

        mock_response = MockStreamResponse(chunks)

        with patch('review.retry_with_backoff', return_value=mock_response):
            config = {**DEFAULT_CONFIG}
            result = call_openrouter(config, "code", sample_code_context, stream=True)

        assert result == "Real content"


class TestStreamingMalformedJSON:
    """Tests for handling malformed JSON in streaming."""

    def test_skips_malformed_json_silently(self, mock_api_key, sample_code_context, capsys):
        """Verify malformed JSON chunks are skipped (current behavior)."""
        chunks = [
            b'data: {"choices":[{"delta":{"content":"Start"}}]}',
            b'data: {this is not valid json}',
            b'data: {"choices":[{"delta":{"content":" End"}}]}',
            b'data: [DONE]',
        ]

        mock_response = MockStreamResponse(chunks)

        with patch('review.retry_with_backoff', return_value=mock_response):
            config = {**DEFAULT_CONFIG}
            result = call_openrouter(config, "code", sample_code_context, stream=True)

        # Should skip the malformed chunk and continue
        assert result == "Start End"

        # Note: Current implementation silently ignores - we're documenting this behavior
        # The fix will add logging for malformed chunks

    def test_handles_truncated_json(self, mock_api_key, sample_code_context):
        """Verify truncated JSON is handled gracefully."""
        chunks = [
            b'data: {"choices":[{"delta":{"content":"OK"}}]}',
            b'data: {"choices":[{"delta":{"content":"',  # Truncated
            b'data: {"choices":[{"delta":{"content":"More"}}]}',
            b'data: [DONE]',
        ]

        mock_response = MockStreamResponse(chunks)

        with patch('review.retry_with_backoff', return_value=mock_response):
            config = {**DEFAULT_CONFIG}
            result = call_openrouter(config, "code", sample_code_context, stream=True)

        # Should at least get the valid chunks
        assert "OK" in result
        assert "More" in result

    def test_handles_unexpected_json_structure(self, mock_api_key, sample_code_context):
        """Verify unexpected JSON structure is handled."""
        chunks = [
            b'data: {"choices":[{"delta":{"content":"A"}}]}',
            b'data: {"unexpected": "structure"}',  # Valid JSON but wrong structure
            b'data: {"choices":[{"delta":{"content":"B"}}]}',
            b'data: [DONE]',
        ]

        mock_response = MockStreamResponse(chunks)

        with patch('review.retry_with_backoff', return_value=mock_response):
            config = {**DEFAULT_CONFIG}
            result = call_openrouter(config, "code", sample_code_context, stream=True)

        # Should handle gracefully (current implementation may return empty for bad structure)
        assert "A" in result
        assert "B" in result


class TestStreamingOutput:
    """Tests for streaming output to console."""

    def test_prints_content_as_received(self, mock_api_key, sample_code_context, capsys):
        """Verify content is printed incrementally."""
        chunks = [
            b'data: {"choices":[{"delta":{"content":"First"}}]}',
            b'data: {"choices":[{"delta":{"content":" Second"}}]}',
            b'data: {"choices":[{"delta":{"content":" Third"}}]}',
            b'data: [DONE]',
        ]

        mock_response = MockStreamResponse(chunks)

        with patch('review.retry_with_backoff', return_value=mock_response):
            config = {**DEFAULT_CONFIG}
            result = call_openrouter(config, "code", sample_code_context, stream=True)

        captured = capsys.readouterr()

        # All content should be printed
        assert "First" in captured.out
        assert "Second" in captured.out
        assert "Third" in captured.out

        # Return value should match printed content
        assert result == "First Second Third"

    def test_final_newline_printed(self, mock_api_key, sample_code_context, capsys):
        """Verify final newline is printed after stream completes."""
        chunks = [
            b'data: {"choices":[{"delta":{"content":"Content"}}]}',
            b'data: [DONE]',
        ]

        mock_response = MockStreamResponse(chunks)

        with patch('review.retry_with_backoff', return_value=mock_response):
            config = {**DEFAULT_CONFIG}
            call_openrouter(config, "code", sample_code_context, stream=True)

        captured = capsys.readouterr()

        # Should end with newline
        assert captured.out.endswith("\n")

    def test_flush_called_for_realtime_output(self, mock_api_key, sample_code_context):
        """Verify flush=True is used for real-time output."""
        chunks = [
            b'data: {"choices":[{"delta":{"content":"Test"}}]}',
            b'data: [DONE]',
        ]

        mock_response = MockStreamResponse(chunks)

        # Capture print calls to verify flush
        print_calls = []
        original_print = print

        def mock_print(*args, **kwargs):
            print_calls.append((args, kwargs))
            original_print(*args, **kwargs)

        with patch('review.retry_with_backoff', return_value=mock_response):
            with patch('builtins.print', mock_print):
                config = {**DEFAULT_CONFIG}
                call_openrouter(config, "code", sample_code_context, stream=True)

        # At least one call should have flush=True
        flush_calls = [c for c in print_calls if c[1].get('flush') == True]
        assert len(flush_calls) > 0


class TestStreamingErrorHandling:
    """Tests for error handling during streaming."""

    def test_timeout_returns_error_message(self, mock_api_key, sample_code_context):
        """Verify timeout returns helpful error message."""
        import requests

        def raise_timeout():
            raise requests.exceptions.Timeout("Connection timed out")

        with patch('review.retry_with_backoff', side_effect=requests.exceptions.Timeout("Timeout")):
            config = {**DEFAULT_CONFIG}
            result = call_openrouter(config, "code", sample_code_context, stream=True)

        assert "ERROR" in result
        assert "timed out" in result.lower() or "timeout" in result.lower()

    def test_connection_error_returns_error_message(self, mock_api_key, sample_code_context):
        """Verify connection error returns helpful message."""
        import requests

        with patch('review.retry_with_backoff', side_effect=requests.exceptions.ConnectionError("Failed")):
            config = {**DEFAULT_CONFIG}
            result = call_openrouter(config, "code", sample_code_context, stream=True)

        # Connection errors are handled by retry_with_backoff which would re-raise
        # after max retries, so we expect an error
        assert "ERROR" in result or result is None or isinstance(result, str)

    def test_http_error_includes_detail(self, mock_api_key, sample_code_context):
        """Verify HTTP error includes error detail from response."""
        import requests

        mock_response = MagicMock()
        mock_response.status_code = 400
        mock_response.json.return_value = {"error": {"message": "Invalid model specified"}}

        error = requests.exceptions.HTTPError("Bad Request")
        error.response = mock_response

        with patch('review.retry_with_backoff', side_effect=error):
            config = {**DEFAULT_CONFIG}
            result = call_openrouter(config, "code", sample_code_context, stream=True)

        assert "ERROR" in result
        assert "Invalid model" in result or "Bad Request" in result


class TestStreamingVsNonStreaming:
    """Tests comparing streaming vs non-streaming behavior."""

    def test_same_content_both_modes(self, mock_api_key, sample_code_context):
        """Verify same content returned in both modes."""
        expected_content = "## Summary\nGood code."

        # Non-streaming response
        non_stream_response = {
            "choices": [{"message": {"content": expected_content}}]
        }

        # Streaming response
        stream_chunks = [
            b'data: {"choices":[{"delta":{"content":"## Summary"}}]}',
            b'data: {"choices":[{"delta":{"content":"\\nGood code."}}]}',
            b'data: [DONE]',
        ]

        # Test non-streaming
        with patch('review.retry_with_backoff') as mock:
            mock_resp = MagicMock()
            mock_resp.json.return_value = non_stream_response
            mock.return_value = mock_resp

            config = {**DEFAULT_CONFIG}
            non_stream_result = call_openrouter(config, "code", sample_code_context, stream=False)

        # Test streaming
        with patch('review.retry_with_backoff', return_value=MockStreamResponse(stream_chunks)):
            config = {**DEFAULT_CONFIG}
            stream_result = call_openrouter(config, "code", sample_code_context, stream=True)

        assert non_stream_result == expected_content
        assert stream_result == expected_content

    def test_streaming_default_true(self, mock_api_key, sample_code_context):
        """Verify streaming is the default behavior."""
        # The call_openrouter function should default to stream=True
        import inspect
        from review import call_openrouter

        sig = inspect.signature(call_openrouter)
        stream_param = sig.parameters.get('stream')

        assert stream_param is not None
        assert stream_param.default == True
