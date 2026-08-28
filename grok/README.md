# Grok Integration

Fovea sends structured scene graph JSON to the xAI API for passenger-facing narration.

## Environment

```bash
export XAI_API_KEY=your_key_here
```

## API

- Base URL: `https://api.x.ai/v1`
- Model: `grok-4.6`

## Request Shape

```json
{
  "model": "grok-4.6",
  "input": [
    {
      "type": "text",
      "text": "You are an in-vehicle autonomy copilot. Given this scene graph, explain what the vehicle should do in 2-3 sentences.\n\n<scene_json>"
    }
  ]
}
```

## Implementation Status

- `core/src/grok_client.cpp` — offline-safe stub with `XAI_API_KEY` detection
- Next: HTTP client via libcurl or cpp-httplib

## Planned Function Calls

- `get_detection(id)`
- `get_prediction(id)`
- `list_high_threat_objects()`

This enables Grok Q&A against live C++ scene state.
