# Dashcam sample clips

| File | Camera | Notes |
|------|--------|-------|
| `street.mp4` | Forward | Default forward clip |
| `highway.mp4` | Forward | Truck/highway clip for accuracy checks |
| `reverse.mp4` | Rear | Reverse / behind traffic |

```bash
# Refresh all clips (needs yt-dlp + network)
python3 scripts/download_sample_video.py --force --skip-image

# Run panel on highway truck footage
./build/app/fovea_app --forward assets/test/dashcam/highway.mp4 \
  --reverse assets/test/dashcam/reverse.mp4
# or:
./scripts/run_panel.sh
# then pass paths if your run script supports them — see PROJECT.md
```
