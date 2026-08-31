#!/usr/bin/env bash
# Run Fovea with Grok + passenger panel. Loads .env from project root if present.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if [[ -f .env ]]; then
  set -a
  # shellcheck disable=SC1091
  source .env
  set +a
fi

MEDIA="${1:-assets/test/dashcam/street.mp4}"
BUILD="${ROOT}/build/app/fovea_app"

if [[ ! -x "$BUILD" ]]; then
  echo "Build fovea_app first: cmake --build build --target fovea_app"
  exit 1
fi

if [[ -z "${XAI_API_KEY:-}" ]]; then
  echo "Note: XAI_API_KEY not set — Grok runs in offline fallback mode."
  echo "Add your key: echo 'XAI_API_KEY=xai-...' > .env"
  echo ""
fi

exec "$BUILD" "$MEDIA" --grok --panel
