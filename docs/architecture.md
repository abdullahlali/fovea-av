# Architecture

## Pipeline

Fovea processes each frame through a modular C++ pipeline:

1. **Capture** — ingest image/video frame metadata
2. **Infer** — object detection (ONNX stub in v0.1)
3. **Scene Graph** — normalize detections + assign stable IDs
4. **Predictor** — project 2-second trajectory ghosts from velocity
5. **Grok** — generate passenger-facing narration from scene JSON
6. **Render** — Qt/Svelte visualization layers (upcoming)

## Scene Graph Schema

Each frame exports JSON with:

- `detections[]` — label, confidence, bbox, velocity
- `predictions[]` — future bbox per detection at `horizon_seconds`

This schema is the contract between C++ core, Qt renderer, Svelte panel, and Grok.

## Threading Plan (v0.2+)

```text
[Capture Thread] → [Frame Queue] → [Inference Workers] → [Scene Graph]
                                                          ↓
                                              [Render Thread / Qt]
                                                          ↓
                                              [WebSocket → Svelte]
```

Target: render loop never blocks on inference.

## Standout Features

- **Prediction ghosts** — forward bbox projection
- **Confidence-aware overlays** — high confidence = sharp, low = faded (Qt)
- **Grok narration** — plain-language autonomy explanation
- **Live metrics** — capture/infer/predict/total latency
- **Scenario sandbox** — inject hazards and re-run pipeline (planned)
