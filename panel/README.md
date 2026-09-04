# Passenger panel

Svelte 5 UI for the rear/passenger display. Connects to `fovea_app --panel` over WebSocket.

## Run

```bash
# Terminal 1
./build/app/fovea_app assets/test/dashcam/street.mp4 --panel --grok

# Terminal 2
cd panel && npm install && npm run dev
```

Open http://localhost:5173

## Features

| Feature | Status |
|---------|--------|
| Live scene JSON from driver app | ✅ |
| Grok narration | ✅ |
| Object list with hold/smoothing | ✅ |
| Camera mode rail (forward / reverse / surround) | ✅ |
| Trust pulse (confidence meter) | ✅ |
| WebSocket auto-reconnect | ✅ |

The panel does **not** replay video — that stays on the Qt driver display.

## Env

```bash
VITE_FOVEA_WS_URL=ws://127.0.0.1:8765 npm run dev
```
