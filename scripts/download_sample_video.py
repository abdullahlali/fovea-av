#!/usr/bin/env python3
"""Prepare Fovea demo assets: BDD street image + forward/reverse dashcam clips."""

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
HIGHWAY_VIDEO_OUT = ROOT / "assets/test/dashcam/highway.mp4"
REVERSE_VIDEO_OUT = ROOT / "assets/test/dashcam/reverse.mp4"
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


def download_yt_search_clip(
    dst: Path,
    *,
    queries: list[str],
    output_stem: str,
    start: float,
    duration: float,
) -> None:
    if shutil.which("yt-dlp") is None:
        raise SystemExit("yt-dlp is required for sample video downloads.")

    dst.parent.mkdir(parents=True, exist_ok=True)
    pattern = dst.parent / f"{output_stem}.%(id)s.%(ext)s"

    last_error = ""
    for query in queries:
        print(f"Searching: {query}")
        for old in dst.parent.glob(f"{output_stem}.*"):
            if old.suffix in {".mp4", ".mkv", ".webm"}:
                old.unlink(missing_ok=True)

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
                str(pattern),
                f"ytsearch1:{query}",
            ],
            check=False,
        )
        if result.returncode != 0:
            last_error = result.stderr.strip() or "yt-dlp search failed"
            continue

        candidates = sorted(dst.parent.glob(f"{output_stem}.*"))
        candidates = [p for p in candidates if p.suffix in {".mp4", ".mkv", ".webm"}]
        if not candidates:
            last_error = "download finished but no video file was found"
            continue

        trim_video(candidates[0], dst, start=start, duration=duration)
        for candidate in candidates:
            candidate.unlink(missing_ok=True)
        return

    raise SystemExit(
        "Could not download a sample clip from YouTube.\n"
        f"Last error: {last_error}\n"
        "Install/update yt-dlp or provide a local file via FOVEA_BDD_VIDEO / "
        "FOVEA_REVERSE_VIDEO / FOVEA_SURROUND_VIDEO."
    )


def download_forward_dashcam_clip(dst: Path, *, start: float, duration: float) -> None:
    download_yt_search_clip(
        dst,
        queries=[
            "dashcam highway truck ahead driving pov 1080p",
            "forward dashcam freeway cars trucks traffic",
            "dashcam highway driving forward view",
            "forward dashcam city driving pov",
        ],
        output_stem="street.source",
        start=start,
        duration=duration,
    )


def download_highway_truck_clip(dst: Path, *, start: float, duration: float) -> None:
    download_yt_search_clip(
        dst,
        queries=[
            "dashcam following semi truck highway front view",
            "dashcam large truck ahead freeway driving",
            "car dashcam highway truck lane change",
        ],
        output_stem="highway.source",
        start=start,
        duration=duration,
    )


def download_reverse_camera_clip(dst: Path, *, start: float, duration: float) -> None:
    download_yt_search_clip(
        dst,
        queries=[
            "rear facing dashcam highway driving behind cars",
            "rear dashcam road traffic behind car",
            "backup camera dashcam highway",
        ],
        output_stem="reverse.source",
        start=start,
        duration=duration,
    )


def build_clip(
    dst: Path,
    *,
    label: str,
    force: bool,
    start: float,
    duration: float,
    user_video: str | None,
    downloader,
) -> None:
    if dst.exists() and not force:
        print(f"{label} already exists: {dst}")
        return

    if user_video:
        trim_video(Path(user_video), dst, start=start, duration=duration)
        print(f"Trimmed user {label} -> {dst}")
        return

    downloader(dst, start=start, duration=duration)
    print(f"Saved {label} -> {dst}")


def build_sample_video(
    dataset_root: Path,
    *,
    force: bool,
    start: float,
    duration: float,
    user_video: str | None,
) -> None:
    build_clip(
        VIDEO_OUT,
        label="forward dashcam",
        force=force,
        start=start,
        duration=duration,
        user_video=user_video,
        downloader=download_forward_dashcam_clip,
    )

    labels_path = dataset_root / "bdd100k_labels_release/bdd100k/labels/bdd100k_labels_images_val.json"
    if labels_path.exists() and VIDEO_OUT.exists():
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
    parser.add_argument("--skip-video", action="store_true", help="Skip forward dashcam sample")
    parser.add_argument("--skip-reverse", action="store_true", help="Skip reverse camera sample")
    parser.add_argument("--skip-highway", action="store_true", help="Skip highway truck sample")
    parser.add_argument(
        "--video",
        default=os.environ.get("FOVEA_BDD_VIDEO"),
        help="Optional local forward dashcam MP4 to trim instead of downloading",
    )
    parser.add_argument(
        "--highway-video",
        default=os.environ.get("FOVEA_HIGHWAY_VIDEO"),
        help="Optional local highway dashcam MP4 to trim instead of downloading",
    )
    parser.add_argument(
        "--reverse-video",
        default=os.environ.get("FOVEA_REVERSE_VIDEO"),
        help="Optional local rear-camera MP4 to trim instead of downloading",
    )
    parser.add_argument("--start", type=float, default=30.0, help="Trim start offset (seconds)")
    parser.add_argument("--duration", type=float, default=20.0, help="Trim duration (seconds)")
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

    if not args.skip_highway:
        build_clip(
            HIGHWAY_VIDEO_OUT,
            label="highway truck dashcam",
            force=args.force,
            start=args.start,
            duration=args.duration,
            user_video=args.highway_video,
            downloader=download_highway_truck_clip,
        )

    if not args.skip_reverse:
        build_clip(
            REVERSE_VIDEO_OUT,
            label="reverse camera",
            force=args.force,
            start=args.start,
            duration=args.duration,
            user_video=args.reverse_video,
            downloader=download_reverse_camera_clip,
        )

    print("\nRun:")
    print("  ./scripts/run_panel.sh")
    print("  ./build/app/fovea_app --forward assets/test/dashcam/highway.mp4 --grok")


if __name__ == "__main__":
    main()
