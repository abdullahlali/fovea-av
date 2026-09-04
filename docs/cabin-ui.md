# Cabin UI — how the displays work together

Fovea models a **real car**: two screens, one brain.

```text
┌──────────────────────────────┐     WebSocket      ┌──────────────────────────────┐
│  DRIVER DISPLAY (Qt)         │  ───────────────►  │  PASSENGER DISPLAY (Svelte)  │
│  fovea_app                   │   scene JSON +     │  panel/ (browser)            │
│                              │   metrics + grok   │                              │
│  • Camera feed + boxes       │                    │  • Plain-language narration  │
│  • Prediction ghosts         │                    │  • Object summary + trust    │
│  • Scenario hazards (red)    │                    │  • Camera mode indicator     │
│  • Play/loop, scrubber       │                    │  • No duplicate video feed   │
└──────────────────────────────┘                    └──────────────────────────────┘
```

## Run together

```bash
# Terminal 1 — driver
./scripts/run_with_grok.sh assets/test/dashcam/street.mp4 --panel

# Terminal 2 — passenger
cd panel && npm run dev
```

Place Qt and browser side-by-side to simulate split cabin.

## Camera modes

| Mode | CLI | Default media |
|------|-----|---------------|
| Forward | `--camera forward` (default) | `assets/test/dashcam/street.mp4` |
| Reverse | `--camera reverse` | `assets/test/dashcam/reverse.mp4` |
| Surround | `--camera surround` | `assets/test/dashcam/surround.mp4` |

Driver selects camera at launch. Passenger panel reflects the active mode (read-only rail).

## Scenario sandbox

Red boxes (ID ≥ 9000) are injected hazards — see [assets/scenarios/README.md](../assets/scenarios/README.md).

## Passenger panel features

- WebSocket reconnect if driver restarts
- Snapshot on connect (late-joining panel gets last frame)
- Trust pulse: aggregate detection confidence
- Camera-aware briefing text
