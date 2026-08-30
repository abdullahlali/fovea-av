# Fovea

**Autonomy, in focus.**

Fovea is an interactive autonomy visualization platform. It converts camera input into a structured driving scene with predictive trajectories, confidence-aware perception data, and Grok-powered passenger-facing explanations.

> *Fovea* (Latin: *fovea centralis*) is the region of the retina with the sharpest vision. Fovea renders machine perception with the same principle: **focus clarity where it matters**, fade uncertainty where it does not.

This project is an independent portfolio build inspired by in-vehicle autonomy trust UX. It is not affiliated with Tesla or xAI.

## Status

| Milestone | Status |
|-----------|--------|
| C++ perception pipeline | ✅ |
| YOLOv8 ONNX inference | ✅ |
| Qt visualization shell | ✅ |
| Video replay + IoU tracking | ✅ |
| Grok narration integration | ✅ |
| Svelte passenger panel | ✅ |
| Multi-camera (reverse, 360°) | 🔜 v0.7+ |

## Run (visual app)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DFOVEA_ENABLE_ONNX=ON -DFOVEA_ENABLE_QT=ON -DFOVEA_ENABLE_VIDEO=ON
cmake --build build

python3 scripts/download_sample_video.py
./build/app/fovea_app assets/test/dashcam/street.mp4 --panel
```

Passenger panel (separate terminal):

```bash
cd panel && nvm use && npm install && npm run dev
```

## Run (terminal debug)

```bash
./build/core/fovea_cli assets/test/bdd/street.jpg --grok
```

See [docs/how-it-works.md](docs/how-it-works.md) for the pipeline walkthrough.

**Two displays, one pipeline:** [docs/cabin-ui.md](docs/cabin-ui.md) explains how the Qt driver window and Svelte passenger panel work together.

## Architecture

```
capture → infer → tracker → scene_graph → predictor → (grok) → render
   │        │        │           │             │         │
   C++      C++      C++         C++           C++      xAI API
```

```text
fovea-av/
├── core/           # C++ perception pipeline + scene graph
├── app/            # Qt driver shell (scene + video windows)
├── panel/          # Svelte passenger UI (WebSocket)
├── models/         # ONNX model artifacts (not committed)
├── assets/         # Test images and dashcam clips
└── docs/           # Architecture + technical brief
```

## Roadmap

- **v0.5** — Video replay, IoU tracking, OpenCV capture
- **v0.6** — Svelte passenger panel + WebSocket bridge
- **v0.7** — Reverse camera + scenario sandbox
- **v0.8** — 360° surround view (FoveaLens / Qt Quick 3D)

## Tech Stack

| Layer | Technology |
|-------|------------|
| Core | C++17, CMake |
| Perception | ONNX Runtime, OpenCV |
| UI Shell | Qt 6 |
| Passenger Panel | Svelte 5, Vite |
| AI Narration | Grok API |

## License

MIT — see [LICENSE](LICENSE).

## Author

Abdullah Liaqat Ali — [GitHub](https://github.com/abdullahlali) · [LinkedIn](https://linkedin.com/in/abdullahlali/)
