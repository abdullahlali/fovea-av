#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODELS_DIR="${ROOT_DIR}/models"

mkdir -p "${MODELS_DIR}"

echo "Model download script is scaffolded."
echo "Next step: add YOLOv8 ONNX download URL and checksum verification."
echo "Target directory: ${MODELS_DIR}"
