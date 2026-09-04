# How Fovea Fits Together

## The big picture

Fovea is a **C++ perception pipeline** with two frontends:

- `fovea_cli` — debug output (JSON + timings)
- `fovea_app` — Qt window (boxes + Grok text)

Both call the same `Pipeline::process_image()` in `core/`.

---

## Step-by-step (one image)

### 1. Capture (`capture.cpp`)

**What:** Load image from disk into memory.

**How:** Uses `stb_image` (single-header library) to decode JPEG/PNG into raw RGB bytes.

**Output:** `SceneFrame.image` with `width`, `height`, `pixels[]`.

```cpp
SceneFrame frame = capture_.load_frame({.image_path = "street.jpg"});
```

---

### 2. Infer (`infer.cpp` + `infer_onnx.cpp`)

**What:** Find objects in the image (people, cars, bus, etc.).

**How:**
1. Resize image to 640×640 with letterboxing (keeps aspect ratio)
2. Run **YOLOv8n** through **ONNX Runtime** (neural network)
3. Parse 8400 candidate boxes, apply NMS (remove duplicates)
4. Filter to driving-relevant classes only
5. Reject boxes covering >55% of image (sanity check)

**Output:** `frame.detections[]` — each has `label`, `confidence`, `bbox`.

**Pattern:** PIMPL — `InferEngine::Impl` holds the ONNX session so headers stay clean.

---

### 3. Tracker (`tracker.cpp`) — video

**What:** Maintain stable object IDs across frames and estimate velocity.

**How:** IoU matching between consecutive detections (same label, IoU ≥ 0.3).  
Velocity = pixel displacement / Δt from bbox centers.

**Output:** `detection.id` persists frame-to-frame; `detection.velocity` feeds the predictor.

---

### 4. Scene Graph (`scene_graph.cpp`)

**What:** Normalize detections into a stable structure.

**How:** Assigns IDs (1, 2, 3…), exports JSON for Grok and debugging.

**Output:** Same `frame.detections` with IDs + `to_json()` string.

This is the **contract** between perception and UI/AI layers.

---

### 5. Predictor (`predictor.cpp`)

**What:** Show where objects will be in ~2 seconds.

**How:** `future_position = current_position + velocity × 2s`  
On still images velocity is 0 (ghosts overlap boxes). On video, tracker supplies real velocity.

**Output:** `frame.predictions[]` — future bounding boxes.

---

### 6. Grok (`grok_client.cpp`)

**What:** Plain-language explanation for passengers.

**How:**
1. Serialize scene graph to JSON
2. POST to `https://api.x.ai/v1/chat/completions` via **libcurl**
3. Parse response, extract `message.content`

**Requires:** `export XAI_API_KEY=your_key`

**Offline:** Returns fallback text if key is missing.

---

### 7. Render

| App | File | What it does |
|-----|------|--------------|
| CLI | `main.cpp` | Prints JSON + metrics |
| Qt (image) | `scene_window.cpp` | Single-frame view |
| Qt (video) | `video_window.cpp` | Play/pause, scrubber, live tracking |
| Svelte panel | `panel/src/App.svelte` | Passenger UI via WebSocket (`--panel`) |

---

## Key data structures (`types.hpp`)

```cpp
Detection   → one object (label, confidence, bbox, velocity)
SceneFrame  → one timestep (image + detections + predictions)
PipelineResult → frame + metrics + grok response
```

---

## Folder map

```text
core/
  include/fovea/   ← headers (public API)
  src/
    capture.cpp    ← image loading
    infer*.cpp     ← YOLO inference
    scene_graph.cpp← IDs + JSON
    predictor.cpp  ← future ghosts
    grok_client.cpp← xAI API
    tracker.cpp    ← IoU tracking + velocity
    video_capture.cpp ← OpenCV frame reader (optional)
    pipeline.cpp   ← orchestrates everything
app/               ← Qt UI (scene + video windows)
models/            ← yolov8n.onnx (not in git)
assets/test/       ← sample images
```

---

## Build flags

| Flag | Default | Effect |
|------|---------|--------|
| `FOVEA_ENABLE_ONNX` | ON | Real YOLO vs stub detections |
| `FOVEA_ENABLE_QT` | ON | Build `fovea_app` |
| `FOVEA_ENABLE_VIDEO` | ON | OpenCV video replay (requires OpenCV) |

---

## Run commands

```bash
# Terminal debug
./build/core/fovea_cli assets/test/bdd/street.jpg --grok

# Visual app (image)
./build/app/fovea_app assets/test/bdd/street.jpg --grok

# Visual app (video replay — requires OpenCV)
python3 scripts/download_sample_video.py
./build/app/fovea_app assets/test/dashcam/street.mp4

# Passenger panel (v0.6)
./build/app/fovea_app assets/test/dashcam/street.mp4 --panel
cd panel && nvm use && npm install && npm run dev
```

---

## What's next

- [x] Video frame loop + real velocity tracking
- [x] Svelte passenger panel (WebSocket from C++)
- [x] Reverse camera (`--camera reverse`)
- [x] Scenario sandbox (`--scenario` JSON hazards)
- [x] 360° surround view (v0.8)
