#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODELS_DIR="${ROOT_DIR}/models"

mkdir -p "${MODELS_DIR}"

if [[ -f "${MODELS_DIR}/yolov8n.onnx" ]]; then
  echo "Model already present: ${MODELS_DIR}/yolov8n.onnx"
  exit 0
fi

if ! python3 -c "import ultralytics" >/dev/null 2>&1; then
  echo "Installing ultralytics..."
  pip3 install ultralytics
fi

(
  cd "${MODELS_DIR}"
  python3 - <<'PY'
from ultralytics import YOLO

model = YOLO("yolov8n.pt")
model.export(format="onnx", imgsz=640)
print("Exported yolov8n.onnx")
PY
)

echo "Model ready at ${MODELS_DIR}/yolov8n.onnx"
