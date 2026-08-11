#!/usr/bin/env python3
"""
Content Dream Engine
=====================
Scrape, generate, and compose 3D scenes from:
- Web assets (shaders, models, textures)
- Audio analysis (MP3s, songs)
- Game scripts (Dark Souls JSON data)
- Narrative directives and character events

Usage:
    dream.py scrape shaders
    dream.py scrape models
    dream.py dream <song.mp3> [--storyline dark_souls_hero_journey]
    dream.py generate character "knight" --weapon longsword
    dream.py compose <song.mp3> --output video.mp4
"""

from __future__ import annotations
import argparse
import json
import math
import os
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Optional

# Add tools to path
sys.path.insert(0, str(Path(__file__).parent))

from dream_engine import DreamGenerator, create_dream_scene
from renderer import SoftwareRenderer, WIDTH, HEIGHT, FPS, Vec3
from asset_scraper import scrape_shadertoy, scrape_gltf_models, scrape_poly_haven

REPO_ROOT = Path(__file__).resolve().parents[3]
CONTENT_DIR = REPO_ROOT / "Content"
ASSETS_DIR = REPO_ROOT / "assets"
OUTPUT_DIR = REPO_ROOT / "output"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


class ContentDreamEngine:
    """
    Unified engine that combines scraping, generation, and composition.
    """
    
    def __init__(self):
        self.dream_generator = DreamGenerator()
        self.renderer = SoftwareRenderer()
        self.last_scene = None
        self.audio_file = None
        
    def scrape(self, asset_type: str, dry_run: bool = False):
        """Scrape assets from the web."""
        print(f"[DreamEngine] Scraping {asset_type}...")
        
        if asset_type in ["shaders", "all"]:
            print("[DreamEngine] Fetching shaders from Shadertoy...")
            shaders = scrape_shadertoy(dry_run=dry_run)
            print(f"[DreamEngine] Downloaded {len(shaders)} shaders")
        
        if asset_type in ["models", "all"]:
            print("[DreamEngine] Fetching models from Khronos glTF samples...")
            models = scrape_gltf_models(dry_run=dry_run)
            print(f"[DreamEngine] Downloaded {len(models)} models")
        
        if asset_type in ["textures", "all"]:
            print("[DreamEngine] Fetching textures from Poly Haven...")
            textures = scrape_poly_haven(dry_run=dry_run)
            print(f"[DreamEngine] Downloaded {len(textures)} textures")
    
    def load_scripts(self):
        """Load game scripts and narrative data."""
        self.dream_generator.load_scripts()
        print("[DreamEngine] Loaded game scripts and narrative data")
    
    def set_audio(self, mp3_path: str):
        """Set the audio file for dreaming."""
        from tools.renderer import AudioAnalyzer
        self.audio_file = Path(mp3_path)
        if not self.audio_file.exists():
            print(f"[DreamEngine] ERROR: Audio file not found: {mp3_path}")
            return False
        
        print(f"[DreamEngine] Loading audio: {self.audio_file.name}")
        self.audio_analyzer = AudioAnalyzer(self.audio_file)
        if not self.audio_analyzer.extract():
            print("[DreamEngine] ERROR: Failed to extract audio data")
            return False
        print("[DreamEngine] Audio analysis complete")
        return True
    
    def dream(self, storyline: Optional[str] = None) -> Scene:
        """
        Generate a dream scene from current state.
        If storyline is specified, override the default generation.
        """
        if storyline:
            self._apply_storyline(storyline)
        
        # Generate scene from current audio position
        if hasattr(self, 'audio_analyzer') and self.audio_analyzer:
            # Get current time from audio
            current_time = 0.0
            scene = create_dream_scene(self.audio_analyzer, current_time)
        else:
            # Generate without audio
            scene = self.dream_generator.dream(time.time())
        
        self.last_scene = scene
        return scene
    
    def _apply_storyline(self, storyline: str):
        """
        Apply a narrative storyline to the dream generator.
        Overrides spawn patterns and themes.
        """
        storyline = storyline.lower().replace(" ", "_")
        
        if "hero_journey" in storyline or "monomyth" in storyline:
            # Hero's journey: more weapons, fewer enemies initially
            self.dream_generator.script_state.weapons = self.dream_generator.script_state.weapons[:3]
            self.dream_generator.script_state.enemies = self.dream_generator.script_state.enemies[:2]
        
        elif "tragedy" in storyline:
            # Tragedy: more enemies, darker palette
            self.dream_generator.script_state.enemies = self.dream_generator.script_state.enemies * 2
            self.dream_generator.script_state.weapons = []
        
        elif "boss_fight" in storyline:
            # Boss fight: single powerful enemy, arena
            if self.dream_generator.script_state.enemies:
                self.dream_generator.script_state.current_enemy = self.dream_generator.script_state.enemies[0]
        
        elif "exploration" in storyline or "peaceful" in storyline:
            # Exploration: locations only, no enemies
            self.dream_generator.script_state.enemies = []
            self.dream_generator.script_state.weapons = []
    
    def generate_character(self, character_type: str, weapon: Optional[str] = None) -> SceneObject:
        """
        Generate a procedural character based on type and equipment.
        """
        print(f"[DreamEngine] Generating character: {character_type}")
        if weapon:
            print(f"[DreamEngine] Equipped: {weapon}")
        
        # Find weapon data if specified
        weapon_data = None
        if weapon:
            for w in self.dream_generator.script_state.weapons:
                if weapon.lower() in w.get("name", "").lower():
                    weapon_data = w
                    break
        
        # Generate character mesh
        t = time.time()
        triangles = []
        
        if character_type.lower() in ["knight", "warrior"]:
            # Heavy armor humanoid
            size = 1.2
            triangles.extend([
                # Torso (armored)
                Triangle(Vec3(-0.6, 0, 0), Vec3(0.6, 0, 0), Vec3(0, 1.3, 0)),
                Triangle(Vec3(-0.6, 0, 0), Vec3(0.6, 0, 0), Vec3(0, 0, 0)),
                Triangle(Vec3(0, 1.3, 0), Vec3(0.6, 0, 0), Vec3(0, 0, 0)),
                # Head (helmet)
                Triangle(Vec3(-0.35, 1.3, 0), Vec3(0.35, 1.3, 0), Vec3(0, 1.9, 0)),
                # Legs
                Triangle(Vec3(-0.25, 0, 0), Vec3(-0.25, -1.3, 0), Vec3(0, 0, 0)),
                Triangle(Vec3(0.25, 0, 0), Vec3(0.25, -1.3, 0), Vec3(0, 0, 0)),
            ])
        elif character_type.lower() in ["mage", "sorcerer"]:
            # Robed figure
            triangles.extend([
                Triangle(Vec3(-0.4, 0, 0), Vec3(0.4, 0, 0), Vec3(0, 1.4, 0)),
                Triangle(Vec3(-0.4, 0, 0), Vec3(0.4, 0, 0), Vec3(0, 0, 0)),
                Triangle(Vec3(0, 1.4, 0), Vec3(0.4, 0, 0), Vec3(0, 0, 0)),
                # Hood
                Triangle(Vec3(-0.3, 1.4, 0), Vec3(0.3, 1.4, 0), Vec3(0, 1.8, 0.3)),
            ])
        else:
            # Default humanoid
            triangles.extend([
                Triangle(Vec3(-0.5, 0, 0), Vec3(0.5, 0, 0), Vec3(0, 1.2, 0)),
                Triangle(Vec3(-0.5, 0, 0), Vec3(0.5, 0, 0), Vec3(0, 0, 0)),
                Triangle(Vec3(0, 1.2, 0), Vec3(0.5, 0, 0), Vec3(0, 0, 0)),
                Triangle(Vec3(-0.3, 1.2, 0), Vec3(0.3, 1.2, 0), Vec3(0, 1.7, 0)),
            ])
        
        # Add weapon if specified
        if weapon_data:
            weapon_type = weapon_data.get("type", "sword")
            if weapon_type in ["sword", "greatsword"]:
                triangles.extend([
                    Triangle(Vec3(0, 1.0, 0), Vec3(0, 2.5, 0), Vec3(0.15, 1.0, 0)),
                    Triangle(Vec3(0, 1.0, 0), Vec3(0, 2.5, 0), Vec3(-0.15, 1.0, 0)),
                ])
        
        return SceneObject(
            triangles=triangles,
            shader_id="wireframe",
            position=Vec3(0, 0, 0),
            rotation=Vec3(0, 0, 0),
            scale=1.0
        )
    
    def compose(self, output_name: str, duration: Optional[float] = None):
        """
        Compose a full video from audio + dream scene.
        Renders frames and saves as MP4.
        """
        if not self.audio_file:
            print("[DreamEngine] ERROR: No audio file set. Use set_audio() first.")
            return None
        
        if not self.last_scene:
            self.dream()
        
        print(f"[DreamEngine] Composing video: {output_name}")
        print(f"[DreamEngine] Audio: {self.audio_file.name}")
        print(f"[DreamEngine] Scene objects: {len(self.last_scene.objects)}")
        
        # Use renderer to create video
        output_path = OUTPUT_DIR / f"{output_name}.mp4"
        
        # Check if we have ffmpeg
        try:
            import subprocess
            subprocess.run(["ffmpeg", "-version"], capture_output=True, check=True)
            has_ffmpeg = True
        except (FileNotFoundError, subprocess.CalledProcessError):
            has_ffmpeg = False
            print("[DreamEngine] WARNING: ffmpeg not found, saving frames only")
        
        # Render frames
        frames_dir = OUTPUT_DIR / f"{output_name}_frames"
        frames_dir.mkdir(exist_ok=True)
        
        frame_count = 0
        audio_duration = self.audio_analyzer.duration if hasattr(self, 'audio_analyzer') and self.audio_analyzer else 10.0
        
        if duration:
            audio_duration = min(duration, audio_duration)
        
        total_frames = int(audio_duration * FPS)
        
        print(f"[DreamEngine] Rendering {total_frames} frames...")
        
        for i in range(total_frames):
            t = i / FPS
            scene = self.dream(t)
            frame = self.renderer.render_frame(scene, t)
            
            frame_path = frames_dir / f"frame_{i:06d}.png"
            from PIL import Image
            img = Image.fromarray(frame)
            img.save(str(frame_path))
            frame_count += 1
            
            if i % (FPS * 2) == 0:
                print(f"[DreamEngine] Progress: {i}/{total_frames} frames ({i//FPS}s/{int(audio_duration)}s)")
        
        print(f"[DreamEngine] Rendered {frame_count} frames to {frames_dir}")
        
        # Compose with ffmpeg if available
        if has_ffmpeg:
            print("[DreamEngine] Encoding video with ffmpeg...")
            try:
                import subprocess
                cmd = [
                    "ffmpeg", "-y",
                    "-framerate", str(FPS),
                    "-i", str(frames_dir / "frame_%06d.png"),
                    "-i", str(self.audio_file),
                    "-c:v", "libx264",
                    "-preset", "fast",
                    "-crf", "23",
                    "-c:a", "aac",
                    "-b:a", "192k",
                    "-shortest",
                    "-pix_fmt", "yuv420p",
                    str(output_path)
                ]
                result = subprocess.run(cmd, capture_output=True, text=True)
                if result.returncode == 0:
                    print(f"[DreamEngine] Video saved: {output_path}")
                    # Cleanup frames
                    import shutil
                    shutil.rmtree(frames_dir)
                    return str(output_path)
                else:
                    print(f"[DreamEngine] ffmpeg error: {result.stderr}")
            except Exception as e:
                print(f"[DreamEngine] Video composition failed: {e}")
        
        return str(frames_dir)


def main():
    parser = argparse.ArgumentParser(
        description="Content Dream Engine - scrape, generate, and compose 3D dream scenes",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  dream.py scrape shaders                    # Download shaders from Shadertoy
  dream.py scrape models                     # Download models from Khronos
  dream.py load-scripts                      # Load Dark Souls game data
  dream.py dream song.mp3                    # Generate dream from audio
  dream.py dream song.mp3 --storyline boss   # Generate with boss fight theme
  dream.py generate knight --weapon longsword # Create a knight character
  dream.py compose song.mp3 -o output.mp4    # Full video composition
        """
    )
    
    subparsers = parser.add_subparsers(dest="command", help="Command to run")
    
    # Scrape command
    scrape_parser = subparsers.add_parser("scrape", help="Scrape assets from the web")
    scrape_parser.add_argument("type", choices=["shaders", "models", "textures", "all"],
                              help="Asset type to scrape")
    scrape_parser.add_argument("--dry-run", action="store_true",
                              help="Show what would be downloaded without downloading")
    
    # Load scripts command
    load_parser = subparsers.add_parser("load-scripts", help="Load game scripts and data")
    
    # Dream command
    dream_parser = subparsers.add_parser("dream", help="Generate dream scene from audio")
    dream_parser.add_argument("audio", help="Path to MP3 audio file")
    dream_parser.add_argument("--storyline", help="Narrative storyline to apply")
    dream_parser.add_argument("--output", help="Output directory for frames")
    
    # Generate command
    gen_parser = subparsers.add_parser("generate", help="Generate procedural content")
    gen_parser.add_argument("type", choices=["character", "weapon", "location", "enemy"],
                           help="Type of content to generate")
    gen_parser.add_argument("name", help="Name or type of content")
    gen_parser.add_argument("--weapon", help="Weapon to equip (for characters)")
    gen_parser.add_argument("--location", help="Location template to use")
    
    # Compose command
    compose_parser = subparsers.add_parser("compose", help="Compose full video")
    compose_parser.add_argument("audio", help="Path to MP3 audio file")
    compose_parser.add_argument("-o", "--output", required=True, help="Output filename (without extension)")
    compose_parser.add_argument("--duration", type=float, help="Duration in seconds (default: full audio)")
    compose_parser.add_argument("--storyline", help="Narrative storyline to apply")
    
    # Status command
    subparsers.add_parser("status", help="Show engine status and available content")
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        sys.exit(1)
    
    engine = ContentDreamEngine()
    
    if args.command == "scrape":
        engine.scrape(args.type, dry_run=args.dry_run)
    
    elif args.command == "load-scripts":
        engine.load_scripts()
    
    elif args.command == "dream":
        if not engine.set_audio(args.audio):
            sys.exit(1)
        engine.load_scripts()
        scene = engine.dream(storyline=args.storyline)
        print(f"[DreamEngine] Generated scene with {len(scene.objects)} objects")
        if args.output:
            # Save frame
            frame = engine.renderer.render_frame(scene, 0.0)
            from PIL import Image
            img = Image.fromarray(frame)
            output_path = Path(args.output)
            output_path.parent.mkdir(parents=True, exist_ok=True)
            img.save(str(output_path))
            print(f"[DreamEngine] Frame saved: {output_path}")
    
    elif args.command == "generate":
        engine.load_scripts()
        if args.type == "character":
            obj = engine.generate_character(args.name, weapon=args.weapon)
            print(f"[DreamEngine] Generated character '{args.name}' with {len(obj.triangles)} triangles")
        else:
            print(f"[DreamEngine] Generate {args.type}: {args.name} - not yet implemented")
    
    elif args.command == "compose":
        if not engine.set_audio(args.audio):
            sys.exit(1)
        engine.load_scripts()
        if args.storyline:
            engine._apply_storyline(args.storyline)
        output = engine.compose(args.output, duration=args.duration)
        if output:
            print(f"[DreamEngine] SUCCESS: {output}")
        else:
            print("[DreamEngine] FAILED: Could not compose video")
            sys.exit(1)
    
    elif args.command == "status":
        print("[DreamEngine] Status:")
        print(f"  Scripts loaded: {len(engine.dream_generator.script_state.weapons)} weapons, "
              f"{len(engine.dream_generator.script_state.enemies)} enemies, "
              f"{len(engine.dream_generator.script_state.locations)} locations")
        print(f"  Assets dir: {ASSETS_DIR}")
        print(f"  Output dir: {OUTPUT_DIR}")
        print(f"  Audio loaded: {engine.audio_file is not None}")


if __name__ == "__main__":
    main()
