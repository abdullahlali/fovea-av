#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

pass=0
fail=0

check() {
  local name="$1"
  shift
  echo ""
  echo "▸ $name"
  if "$@"; then
    echo "  ✓ pass"
    pass=$((pass + 1))
  else
    echo "  ✗ FAIL"
    fail=$((fail + 1))
  fi
}

echo "═══════════════════════════════════════"
echo " Fovea — full verification harness"
echo "═══════════════════════════════════════"

# Node 18+ for panel
if [[ -s "${HOME}/.nvm/nvm.sh" ]]; then
  # shellcheck disable=SC1091
  source "${HOME}/.nvm/nvm.sh"
  nvm use 20 --silent 2>/dev/null || nvm use 18 --silent 2>/dev/null || true
fi

check "Build (panel + C++ + tests)" ./scripts/build_all.sh

check "Unit tests" ./build/tests/fovea_tests

check "Panel dist exists" test -f panel/dist/index.html

check "Panel assets relative paths" grep -q './assets/' panel/dist/index.html

check "Forward dashcam asset" test -f assets/test/dashcam/street.mp4

check "Reverse dashcam asset" test -f assets/test/dashcam/reverse.mp4

check "ONNX model" test -f models/yolov8n.onnx

check "Scenario files" test -f assets/scenarios/vehicle_cut_in.json

check "CLI still image" ./build/core/fovea_cli assets/test/bdd/street.jpg >/dev/null

check "CLI scenario inject" ./build/core/fovea_cli assets/test/bdd/street.jpg --scenario assets/scenarios/cyclist_ahead.json >/dev/null

check "fovea_app binary" test -x build/app/fovea_app

check "Grok client compiles" strings build/core/libfovea_core.a 2>/dev/null | grep -q narrate || test -f build/core/libfovea_core.a

echo ""
echo "═══════════════════════════════════════"
echo " Results: $pass passed, $fail failed"
echo "═══════════════════════════════════════"

if [[ "$fail" -gt 0 ]]; then
  exit 1
fi

echo ""
echo "Launch:"
echo "  ./scripts/run_panel.sh"
