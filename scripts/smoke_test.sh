#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if [[ ! -x build/app/fovea_app ]]; then
  echo "Build fovea first:"
  echo "  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFOVEA_ENABLE_ONNX=ON -DFOVEA_ENABLE_QT=ON -DFOVEA_ENABLE_VIDEO=ON"
  echo "  cmake --build build"
  exit 1
fi

if [[ ! -f assets/test/dashcam/street.mp4 ]]; then
  python3 scripts/download_sample_video.py --skip-image || true
fi

echo "==> CLI still image"
./build/core/fovea_cli assets/test/bdd/street.jpg

echo "==> CLI scenario inject"
./build/core/fovea_cli assets/test/bdd/street.jpg --scenario assets/scenarios/cyclist_ahead.json

echo "==> Camera profiles"
./build/app/fovea_app --help

echo ""
echo "Ready. Run the full demo:"
echo "  ./scripts/run_with_grok.sh assets/test/dashcam/street.mp4 --panel"
echo "  cd panel && npm run dev"
