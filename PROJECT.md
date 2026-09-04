# Fovea — Project Overview

**Autonomy, in focus.** A resume-ready driver-assist MVP that turns dual dashcam video into a Tesla-style cabin mental model: real-time perception, stable object tracking, a 3D bird’s-eye scene, and LLM copilot briefings.

---

## What it is

Fovea is an end-to-end **driver display** system:

1. **Perceive** — YOLOv8 (ONNX Runtime) detects vehicles, trucks, pedestrians, and bikes in forward + rear dashcam streams  
2. **Track** — Multi-object tracker coasts through missed frames and locks labels (no truck↔bus flicker)  
3. **Project** — 2D boxes → ego-centric 2-lane world coordinates (meters)  
4. **Render** — Three.js 3D bird’s-eye “mind of car” view with YOU fixed in your lane  
5. **Advise** — Action banner + Grok (xAI) copilot briefing, detection-aware (never says “clear” when a truck is ahead)  
6. **Serve** — Headless C++ engine + Qt WebEngine panel over WebSocket, or legacy dual-camera cabin window  

Default product mode is **panel-only** (cameras run headless; driver sees the mental model, not raw video).

---

## Architecture

```text
┌──────────────────┐     WS cabin JSON      ┌────────────────────────────┐
│  PanelEngine     │ ─────────────────────► │  Svelte Driver Panel       │
│  (C++ / Qt)      │ ◄───────────────────── │  Three.js 3D scene         │
│                  │     control (gear)     │  Copilot / warnings / gear │
│  Forward YOLO    │                        └────────────────────────────┘
│  Reverse YOLO    │
│  Tracker + coast │
│  Grok client     │
│  Static HTTP     │ ──► http://127.0.0.1:8766 (panel dist)
└──────────────────┘
         │
         ▼
   yolov8n.onnx (ONNX Runtime)
```

| Layer | Tech |
|-------|------|
| Perception | C++17, YOLOv8n ONNX, OpenCV / custom capture, letterbox + NMS |
| Tracking | IoU + velocity coasting, cross-class vehicle matching, label hysteresis |
| App shell | Qt6 (Widgets + WebEngine), WebSocket bridge (`ws://127.0.0.1:8765`) |
| Panel UI | Svelte 5, Vite 6, Three.js WebGL BEV |
| Copilot | xAI Grok API + local detection-aware fallback |
| Build / QA | CMake, scripts (`build_all`, `run_panel`, `verify_all`), Docker Compose |
| Data | BDD100K stills, dashcam MP4s (street / highway / reverse) |

---

## Implemented features

### Perception & tracking
- Dual-camera pipeline (forward + reverse), alternating inference for latency
- Driving-relevant COCO classes only (person, bike, car, motorcycle, bus, truck, lights, stop)
- Lowered confidence floor + **cross-class NMS** so truck/bus/car on the same object don’t fight
- **Coasting tracker**: keeps IDs alive across ~12 missed frames with predicted boxes
- Strong **label lock** for large vehicles (truck/bus)

### Driver mental model (3D)
- Tesla-style elevated camera behind ego
- **2-lane road**: YOU centered in your lane; one adjacent lane to the left; divider never through ego
- Meter-space projection (`worldX` / `worldZ`) from bbox bottom + apparent height
- Soft lane snap, exponential pose smoothing, long fade-out so objects don’t pop

### Cabin UX
- Landscape driver display (action banner, Copilot, 4-slot detected grid, P/R/N/D)
- Gear-aware warnings (e.g. reverse → behind focus)
- Copilot ignores contradictory “road is clear” narration when detections exist
- Live / offline / pipeline latency in the header

### Product engineering
- Panel-only default + `--cabin` legacy dual feed
- Local static server (fixes `file://` asset loading in WebEngine)
- `.env` loader that overrides stale shell `XAI_API_KEY`
- Scenario JSON hazard injection for demos
- Full verify harness (11 automated checks)

---

## How to run

```bash
./scripts/build_all.sh
./scripts/run_panel.sh

# Highway truck clip (if downloaded)
./build/app/fovea_app --forward assets/test/dashcam/highway.mp4 \
  --reverse assets/test/dashcam/reverse.mp4 --grok

# Refresh sample videos
python3 scripts/download_sample_video.py --force --skip-image
```

Requires: Node 18+, CMake, Qt6 WebEngine, ONNX Runtime, `models/yolov8n.onnx`, optional `XAI_API_KEY` in `.env`.

---

## Resume bullet points

Copy/paste and tweak to your voice:

- Built **Fovea**, an end-to-end driver-assist MVP that converts dual dashcam video into a Tesla-style 3D bird’s-eye cabin display with real-time object detection, tracking, and LLM copilot briefings.  
- Implemented a **C++17 perception pipeline** with YOLOv8 (ONNX Runtime), letterbox preprocessing, cross-class NMS, and dual forward/rear camera inference for low-latency cabin updates.  
- Designed a **multi-object tracker with motion coasting and label hysteresis**, eliminating truck↔bus flicker and preventing objects from disappearing on intermittent detector misses.  
- Engineered an **ego-centric 2-lane world model** that projects 2D bounding boxes into meter-space BEV coordinates with soft lane assignment and temporal pose smoothing.  
- Built a **Three.js WebGL mental-model renderer** (Svelte 5 panel) with elevated camera, shaded vehicle meshes, and stable ego-lane geometry inspired by Tesla Autopilot visualization.  
- Delivered a **Qt6 WebEngine + WebSocket cabin product**: headless perception engine, local static asset server, gear-aware warnings, and bidirectional P/R/N/D controls.  
- Integrated **xAI Grok** for passenger narration with detection-aware fallbacks so the copilot never claims a clear road when hazards are present.  
- Shipped **build/verify automation** (CMake, Vite, Docker, 11-check harness) and scenario injection for repeatable demos on BDD100K and highway dashcam footage.

### Shorter 3-bullet version

- Built a full-stack AV cabin MVP (C++ YOLO + Qt + Svelte/Three.js) that renders a live Tesla-style bird’s-eye driver display from dual dashcams.  
- Hardened perception with coasting multi-object tracking, cross-class NMS, and label locking so large vehicles stay stable frame-to-frame.  
- Productized the stack with WebSocket panel sync, Grok copilot briefings, gear-aware alerts, and an automated verify harness.

---

## Key files

| Path | Role |
|------|------|
| `core/src/infer_onnx.cpp` | YOLOv8 ONNX inference + NMS |
| `core/src/tracker.cpp` | Coasting MOT + label lock |
| `app/panel_engine.cpp` | Headless dual-camera engine |
| `app/panel_bridge.cpp` | WebSocket cabin protocol |
| `panel/src/components/SceneView.svelte` | Three.js 3D scene |
| `panel/src/lib/sceneModel.js` | BEV projection (2-lane meters) |
| `panel/src/lib/passenger.js` | Panel track fusion + briefing |
| `scripts/verify_all.sh` | Full QA harness |
| `scripts/download_sample_video.py` | Dashcam / highway asset fetch |

---

## Design principles

1. **Ego-centric truth** — YOU never sits on a lane line; the world moves around the vehicle.  
2. **Stability over raw YOLO** — Prefer a slightly stale correct truck over a flickering bus.  
3. **Detection-aware language** — Copilot and banners agree with what the scene shows.  
4. **Panel is the product** — Cameras are sensors; the mental model is the UI.
