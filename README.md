# Fovea

**Autonomy, in focus.**

Tesla-style driver display: dual dashcam perception → stable tracking → 3D bird’s-eye mental model + Grok copilot.

Full write-up + resume bullets: **[PROJECT.md](PROJECT.md)**

## Run

```bash
./scripts/build_all.sh
./scripts/run_panel.sh

# Highway truck accuracy check (after download)
./scripts/run_panel.sh --forward assets/test/dashcam/highway.mp4 --reverse assets/test/dashcam/reverse.mp4
```

Refresh sample videos:

```bash
python3 scripts/download_sample_video.py --force --skip-image
```

Legacy dual-camera cabin window:

```bash
./build/app/fovea_app --cabin --grok
```


## Cabin layout

```text
┌─────────────────────────────┬──────────────────────┐
│  Forward camera + boxes     │  BIG road warnings   │
│  Reverse camera + boxes     │  Gear P/R/N/D          │
│  Play / scrubber            │  Grok narration        │
│  Prediction ghosts          │  Object lists          │
└─────────────────────────────┴──────────────────────┘
```

- **Drive (D):** forward + rear cameras active, panel shows both
- **Reverse (R):** panel focuses rear warnings
- **Gear + camera focus** selectable on passenger panel (bidirectional WebSocket)

## Options

```bash
./build/app/fovea_app --grok --gear D
./build/app/fovea_app --forward assets/test/dashcam/street.mp4 --reverse assets/test/dashcam/reverse.mp4
./build/app/fovea_app --driver-only --camera forward   # legacy single-camera window
```

## Docker

```bash
docker compose build
# macOS/Linux with display forwarding:
docker compose run --rm fovea
```

See [PROJECT.md](PROJECT.md), [docs/cabin-ui.md](docs/cabin-ui.md) and [assets/scenarios/README.md](assets/scenarios/README.md).
