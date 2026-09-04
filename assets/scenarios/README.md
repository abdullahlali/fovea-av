# Scenarios

Inject synthetic hazards into the live pipeline for demos and hazard-hunt walkthroughs.

| File | Use with | Trigger |
|------|----------|---------|
| `pedestrian_behind.json` | Forward or reverse video | Frame 60 |
| `vehicle_cut_in.json` | Forward dashcam | Frame 90 |
| `cyclist_ahead.json` | Still image or any video | Always |
| `rear_cross_traffic.json` | `--camera reverse` | Frame 45 |

Scenario detections render in **red** on the driver display and use IDs ≥ 9000.

## Run

```bash
# Still image + persistent cyclist
./build/app/fovea_app assets/test/bdd/street.jpg \
  --scenario assets/scenarios/cyclist_ahead.json --panel --grok

# Forward dashcam — cut-in at frame 90
./build/app/fovea_app assets/test/dashcam/street.mp4 \
  --scenario assets/scenarios/vehicle_cut_in.json --panel

# Reverse camera — cross traffic at frame 45
./build/app/fovea_app --camera reverse \
  --scenario assets/scenarios/rear_cross_traffic.json --panel
```

## Format

```json
{
  "name": "example",
  "description": "Optional note for docs",
  "hazards": [
    {
      "label": "person",
      "confidence": 0.91,
      "trigger_frame": 60,
      "bbox": { "x": 540, "y": 420, "width": 72, "height": 150 }
    }
  ]
}
```

Omit `trigger_frame` (or set `-1`) to inject on every frame.
