# How Fovea Fits Together

## Data flow (one image)

```text
image file
   │
   ▼
Capture::load_frame()          # stb_image → RGB pixels in SceneFrame
   │
   ▼
InferEngine::run()             # ONNX YOLOv8 → bounding boxes + labels
   │
   ▼
SceneGraph::update()           # assign stable IDs to detections
   │
   ▼
Predictor::apply()             # velocity → 2s prediction ghosts
   │
   ▼
SceneWindow / fovea_cli        # render or print JSON
```

## Key structs

- `ImageBuffer` — raw pixels from disk
- `Detection` — one object: label, confidence, bbox, velocity
- `SceneFrame` — everything for one timestep
- `PipelineResult` — frame + timing metrics (+ Grok later)

## Two apps, one core

| Binary | Purpose |
|--------|---------|
| `fovea_cli` | Debug in terminal (JSON + metrics) |
| `fovea_app` | Visual Qt window (boxes + ghosts) |

Both call the same `fovea::Pipeline` in `core/`.

## Confidence styling (Qt)

Higher confidence → brighter gold box alpha.  
Prediction ghosts → dashed white rectangles.
