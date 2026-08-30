# Cabin UI — how the displays work together

Fovea models a **real car**: two screens, one brain.

```text
┌──────────────────────────────┐     WebSocket      ┌──────────────────────────────┐
│  DRIVER DISPLAY (Qt)         │  ───────────────►  │  PASSENGER DISPLAY (Svelte)  │
│  fovea_app                   │   scene JSON +     │  panel/ (browser)            │
│                              │   metrics + grok   │                              │
│  • Camera feed + boxes       │                    │  • Plain-language narration  │
│  • Prediction ghosts         │                    │  • “What the car sees”       │
│  • Play/pause, scrubber      │                    │  • Object summary            │
│  • Engineer metrics (ms)     │                    │  • No duplicate video feed   │
└──────────────────────────────┘                    └──────────────────────────────┘
              │                                                    ▲
              └──────────── C++ Pipeline (single source) ──────────┘
                    capture → infer → track → predict → grok
```

## Why two apps?

| Piece | Role | Why separate |
|-------|------|--------------|
| **Qt `fovea_app`** | Driver / center stack | Native video, low-latency overlays, C++ render loop |
| **Svelte `panel/`** | Passenger / rear screen | Web tech for rich text UI; updates over WebSocket without blocking video |

In a Tesla, the driver sees the road visualization; the passenger often gets context (media, trip info, explanations). Fovea does the same for **autonomy trust** — the driver keeps eyes on boxes and ghosts; the passenger gets narration and a readable object list.

**They are not two unrelated dashboards.** The passenger panel only works when `fovea_app --panel` is running. It mirrors live pipeline output; it does not run its own perception.

## How to run them together

```bash
# Terminal 1 — driver display (starts WebSocket on :8765)
./build/app/fovea_app assets/test/dashcam/street.mp4 --panel

# Terminal 2 — passenger display
cd panel && nvm use && npm run dev
# Open http://localhost:5173
```

Place the Qt window and browser side-by-side to simulate a split cabin.

## What each screen shows

### Driver (Qt) — always the source of truth

- Forward camera (video or image) with detection boxes and 2s prediction ghosts
- Playback controls for video
- Latency metrics (capture / infer / total)
- Optional Grok text inline when `--grok` is set

### Passenger (Svelte) — context, not a clone

- Connection status to the driver app
- Grok narration (large, readable)
- Compact list of tracked objects
- Camera mode strip (Forward today; Reverse / 360° on roadmap)

The passenger panel **intentionally does not** replay the video feed — that stays on the driver display to avoid sync issues and duplicate GPU work.

## Roadmap: unified experience

| Version | Goal |
|---------|------|
| **v0.6** (now) | Two windows, WebSocket bridge — proves split-screen UX |
| **v0.7** | Reverse camera input + scenario sandbox |
| **v0.8** | Optional **embedded** passenger view inside Qt (`QWebEngineView`) for a single-window demo |
| **v0.9+** | 360° FoveaLens bird’s-eye in Qt; passenger panel shows spatial summary |

For portfolio demos, two windows is correct (matches real hardware). For convenience, we can later embed the Svelte build inside Qt so one binary opens both panes.

## Camera views (multi-feed)

Today only **forward dashcam** is wired. The UI already reserves:

- **Reverse** — backup camera when shifting to R (v0.7)
- **360°** — surround stitch or bird’s-eye (v0.8, FoveaLens)

All feeds will still flow through the same pipeline and scene graph; only the capture source changes.
