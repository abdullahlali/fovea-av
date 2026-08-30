#!/usr/bin/env python3
"""Prepare Fovea demo assets: BDD street image + forward dashcam sample clip."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BDD_IMAGE_OUT = ROOT / "assets/test/bdd/street.jpg"
VIDEO_OUT = ROOT / "assets/test/dashcam/street.mp4"
KAGGLE_DATASET = "solesensei/solesensei_bdd100k"
BDD_STREET_FRAME = "b1c66a42-6f7d68ca.jpg"  # city street, bus + pedestrians


def run(cmd: list[str], *, check: bool = True) -> subprocess.CompletedProcess[str]:
    print("$", " ".join(cmd))
    return subprocess.run(cmd, check=check, text=True, capture_output=True)


def ensure_kaggle_dataset() -> Path:
    try:
        import kagglehub
    except ImportError as exc:
        raise SystemExit(
            "kagglehub is required. Install with: pip install kagglehub"
        ) from exc

    path = Path(kagglehub.dataset_download(KAGGLE_DATASET))
    print(f"BDD100K dataset: {path}")
    return path


def find_bdd_image(dataset_root: Path, filename: str) -> Path:
    matches = list(dataset_root.rglob(filename))
    if not matches:
        raise FileNotFoundError(f"Could not find {filename} under {dataset_root}")
    return matches[0]


def sync_street_image(dataset_root: Path, force: bool) -> None:
    src = find_bdd_image(dataset_root, BDD_STREET_FRAME)
    BDD_IMAGE_OUT.parent.mkdir(parents=True, exist_ok=True)

    if BDD_IMAGE_OUT.exists() and not force:
        print(f"Street image already exists: {BDD_IMAGE_OUT}")
        return

    shutil.copy2(src, BDD_IMAGE_OUT)
    print(f"Synced BDD street image -> {BDD_IMAGE_OUT}")


def trim_video(src: Path, dst: Path, *, start: float, duration: float) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    run(
        [
            "ffmpeg",
            "-y",
            "-ss",
            str(start),
            "-i",
            str(src),
            "-t",
            str(duration),
            "-c:v",
            "libx264",
            "-preset",
            "fast",
            "-crf",
            "23",
            "-an",
            str(dst),
        ]
    )


def download_forward_dashcam_clip(dst: Path, *, start: float, duration: float) -> None:
    if shutil.which("yt-dlp") is None:
        raise SystemExit("yt-dlp is required for the default sample video download.")

    tmp = dst.with_suffix(".source.mp4")
    query = "forward dashcam city driving pov"
    result = run(
        [
            "yt-dlp",
            "--extractor-args",
            "youtube:player-client=android",
            "-f",
            "best[height<=720][ext=mp4]/best[height<=720]",
            "--max-downloads",
            "1",
            "-o",
            str(tmp),
            f"ytsearch1:{query}",
        ]
    )
    if result.returncode != 0:
        print(result.stderr, file=sys.stderr)
        raise SystemExit("yt-dlp failed to download a dashcam sample.")

  # yt-dlp may append an id before the extension.
    candidates = sorted(dst.parent.glob("street.source*.mp4"))
    if not candidates:
        candidates = [tmp] if tmp.exists() else []
    if not candidates:
        raise SystemExit("Downloaded dashcam source not found.")

    trim_video(candidates[0], dst, start=start, duration=duration)
    for candidate in candidates:
        candidate.unlink(missing_ok=True)


def build_sample_video(
    dataset_root: Path,
    *,
    force: bool,
    start: float,
    duration: float,
    user_video: str | None,
) -> None:
    if VIDEO_OUT.exists() and not force:
        print(f"Sample video already exists: {VIDEO_OUT}")
        return

    if user_video:
        trim_video(Path(user_video), VIDEO_OUT, start=start, duration=duration)
        print(f"Trimmed user video -> {VIDEO_OUT}")
        return

    # BDD Kaggle release is image-only (10th-second keyframes). Use a forward dashcam clip
    # for replay/tracking demos unless the user supplies an official BDD video path.
    download_forward_dashcam_clip(VIDEO_OUT, start=start, duration=duration)
    print(f"Saved sample dashcam clip -> {VIDEO_OUT}")

    labels_path = dataset_root / "bdd100k_labels_release/bdd100k/labels/bdd100k_labels_images_val.json"
    if labels_path.exists():
        labels = json.loads(labels_path.read_text())
        frame = next(item for item in labels if item.get("name") == BDD_STREET_FRAME)
        print(
            "BDD reference frame:",
            BDD_STREET_FRAME,
            frame.get("attributes", {}),
        )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true", help="Overwrite existing assets")
    parser.add_argument("--skip-image", action="store_true", help="Skip BDD street.jpg sync")
    parser.add_argument("--skip-video", action="store_true", help="Skip sample video build")
    parser.add_argument(
        "--video",
        default=os.environ.get("FOVEA_BDD_VIDEO"),
        help="Optional local BDD/dashcam MP4 to trim instead of downloading",
    )
    parser.add_argument("--start", type=float, default=30.0, help="Trim start offset (seconds)")
    parser.add_argument("--duration", type=float, default=15.0, help="Trim duration (seconds)")
    args = parser.parse_args()

    dataset_root = ensure_kaggle_dataset()

    if not args.skip_image:
        sync_street_image(dataset_root, args.force)

    if not args.skip_video:
        build_sample_video(
            dataset_root,
            force=args.force,
            start=args.start,
            duration=args.duration,
            user_video=args.video,
        )

    print("\nRun:")
    print(f"  ./build/app/fovea_app {VIDEO_OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
