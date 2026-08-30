# Svelte passenger panel — v0.6

Professional split-screen cabin UI fed by the C++ WebSocket bridge. Design follows automotive HMI principles: high contrast, glance-safe typography, and Tesla v12–inspired layout (status bar, camera strip, latency readouts).

## Requirements

Node.js **18+** (see `.nvmrc`).

```bash
nvm use   # from panel/
```

## Dev

```bash
# Terminal 1 — Qt app with panel bridge
./build/app/fovea_app assets/test/dashcam/street.mp4 --panel

# Terminal 2 — passenger UI
cd panel
nvm use
npm install
npm run dev
```

Open http://localhost:5173 — connects to `ws://127.0.0.1:8765`.

## Camera views (roadmap)

| View | Status |
|------|--------|
| Forward dashcam | ✅ v0.6 (current default) |
| Reverse camera | 🔜 v0.7+ |
| 360° surround | 🔜 v0.8+ (FoveaLens) |

The panel shows the camera strip now; only forward is wired to the pipeline today.
