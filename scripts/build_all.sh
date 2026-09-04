#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

ensure_node_18() {
  local need_major=18
  local current_major
  current_major="$(node -p "process.versions.node.split('.')[0]" 2>/dev/null || echo 0)"

  if [[ "${current_major}" -ge "${need_major}" ]]; then
    return 0
  fi

  if [[ -s "${HOME}/.nvm/nvm.sh" ]]; then
    # shellcheck disable=SC1091
    source "${HOME}/.nvm/nvm.sh"
    if [[ -f panel/.nvmrc ]]; then
      nvm use --silent >/dev/null 2>&1 || nvm install >/dev/null
    else
      nvm use 20 --silent >/dev/null 2>&1 || nvm install 20 >/dev/null
    fi
    current_major="$(node -p "process.versions.node.split('.')[0]")"
  fi

  if [[ "${current_major}" -lt "${need_major}" ]]; then
    echo "error: Node ${need_major}+ required for panel build (current: $(node -v 2>/dev/null || echo unknown))" >&2
    echo "hint: nvm use 20   (or install Node 18+)" >&2
    exit 1
  fi
}

echo "==> Build passenger panel"
ensure_node_18
(
  cd panel
  npm install
  npm run build
)

echo "==> Configure + build fovea"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DFOVEA_ENABLE_ONNX=ON -DFOVEA_ENABLE_QT=ON -DFOVEA_ENABLE_VIDEO=ON
cmake --build build

echo "==> Smoke tests"
./scripts/smoke_test.sh

echo ""
echo "Launch cabin display:"
echo "  ./build/app/fovea_app --grok"
