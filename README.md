# Fovea

**Autonomy, in focus.**

Fovea is an interactive autonomy visualization platform. It converts camera input into a structured driving scene with predictive trajectories, confidence-aware perception data, and Grok-powered passenger-facing explanations.

> *Fovea* (Latin: *fovea centralis*) is the region of the retina with the sharpest vision. Fovea renders machine perception with the same principle: **focus clarity where it matters**, fade uncertainty where it does not.

This project is an independent portfolio build inspired by in-vehicle autonomy trust UX. It is not affiliated with Tesla or xAI.

# Fovea

**Autonomy, in focus.**

Fovea is an interactive autonomy visualization platform. It converts camera input into a structured driving scene with predictive trajectories, confidence-aware perception data, and Grok-powered passenger-facing explanations.

## Status

| Milestone | Status |
|-----------|--------|
| C++ perception pipeline | ✅ |
| YOLOv8 ONNX inference | ✅ |
| Qt visualization shell | ✅ |
| Grok narration integration | 🚧 stub |
| Svelte passenger panel | 🚧 planned |

## Run (visual app)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFOVEA_ENABLE_ONNX=ON -DFOVEA_ENABLE_QT=ON
cmake --build build
./build/app/fovea_app assets/test/bdd/street.jpg
```

## Run (terminal debug)

```bash
./build/core/fovea_cli assets/test/bdd/street.jpg --grok
```

See [docs/how-it-works.md](docs/how-it-works.md) for the pipeline walkthrough.

## Architecture

```
capture → infer → scene_graph → predictor → (grok) → render
   │        │          │             │         │
   C++      C++        C++           C++      xAI API
```

```text
fovea-av/
├── core/           # C++ perception pipeline + scene graph
├── app/            # Qt/QML in-vehicle shell (upcoming)
├── panel/          # Svelte passenger UI (upcoming)
├── grok/           # Grok integration notes + examples
├── models/         # ONNX model artifacts (not committed)
├── assets/         # Test images and dashcam clips
└── docs/           # Architecture + technical brief
```

## Quick Start

### Prerequisites

- C++17 compiler (Clang 14+ or GCC 11+)
- CMake 3.20+

Optional (upcoming milestones):

- ONNX Runtime
- Qt 6
- Node.js 20+ (Svelte panel)

### Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Run

```bash
./build/core/fovea_cli assets/test/street.jpg
./build/core/fovea_cli assets/test/street.jpg --grok
```

With Grok narration:

```bash
export XAI_API_KEY=your_key_here
./build/core/fovea_cli assets/test/street.jpg --grok
```

## Roadmap

- **v0.1** — C++ pipeline, scene graph JSON, prediction ghosts, metrics
- **v0.2** — ONNX YOLO inference on real images
- **v0.3** — Qt visualization shell with confidence-weighted overlays
- **v0.4** — Grok HTTP client + passenger narration panel
- **v0.5** — Svelte split-screen cabin UI + WebSocket bridge
- **v0.6** — Scenario sandbox + Hazard Hunt mini-game

## Tech Stack

| Layer | Technology |
|-------|------------|
| Core | C++17, CMake |
| Perception | ONNX Runtime, OpenCV (planned) |
| UI Shell | Qt 6 / QML, Qt Quick 3D (planned) |
| Passenger Panel | Svelte 5 (planned) |
| AI Narration | Grok API (`grok-4.6`) |

## License

MIT — see [LICENSE](LICENSE).

## Author

Abdullah Liaqat Ali — [GitHub](https://github.com/abdullahlali) · [LinkedIn](https://linkedin.com/in/abdullahlali/)
