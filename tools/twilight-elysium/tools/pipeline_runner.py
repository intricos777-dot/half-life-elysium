#!/usr/bin/env python3
"""
Twilight Elysium Pipeline Runner
==================================
Orchestrates: scraper -> renderer -> video pipeline for multiple MP3s.
"""

import sys
import glob
from pathlib import Path
from tools.renderer import run_pipeline, Scene, Vec3

REPO_ROOT = Path.home() / "Projects" / "twilight-elysium"


def create_music_scene() -> Scene:
    """Create a scene optimized for music visualization."""
    scene = Scene(
        background_color=(5, 5, 15)
    )

    # Add wireframe objects from assets if available
    from renderer import load_models_from_assets
    objects = load_models_from_assets()
    scene.objects.extend(objects)

    return scene


def batch_render(mp3_dir: Path):
    """Render all MP3s in a directory."""
    mp3s = list(mp3_dir.glob("*.mp3"))
    if not mp3s:
        print(f"No MP3s found in {mp3_dir}")
        return

    print(f"[Batch] Found {len(mp3s)} MP3 files")

    scene = create_music_scene()

    for i, mp3 in enumerate(mp3s, 1):
        print(f"\n[Batch] Processing {i}/{len(mp3s)}: {mp3.name}")
        output_name = mp3.stem.replace(" ", "_")
        try:
            run_pipeline(mp3, output_name, scene)
        except Exception as e:
            print(f"[Batch] Error processing {mp3.name}: {e}")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        print("\nUsage:")
        print("  pipeline_runner.py <mp3_file> [output_name]")
        print("  pipeline_runner.py --batch <mp3_directory>")
        sys.exit(1)

    if sys.argv[1] == "--batch":
        if len(sys.argv) < 3:
            print("Usage: pipeline_runner.py --batch <mp3_directory>")
            sys.exit(1)
        batch_render(Path(sys.argv[2]))
    else:
        mp3_path = Path(sys.argv[1])
        output_name = sys.argv[2] if len(sys.argv) > 2 else mp3_path.stem
        scene = create_music_scene()
        run_pipeline(mp3_path, output_name, scene)


if __name__ == "__main__":
    main()
