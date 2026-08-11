#!/usr/bin/env python3
"""
Twilight Elysium Universal Renderer
=====================================
Software renderer that translates shader/model assets into video frames.
No GPU/OpenGL/Vulkan required - pure numpy + PIL.

Pipeline:
  MP3 -> Audio Analyzer -> Beat/Spectrum data
  Assets -> Asset Importer -> Scene graph
  Scene + Audio -> Renderer -> Frames
  Frames + Audio -> Video Composer -> MP4
"""

import os
import sys
import json
import math
import struct
import subprocess
import tempfile
import hashlib
from pathlib import Path
from typing import List, Tuple, Optional, Dict
from dataclasses import dataclass, field

import numpy as np
from PIL import Image, ImageDraw, ImageFont

REPO_ROOT = Path.home() / "Projects" / "twilight-elysium"
ASSETS_DIR = REPO_ROOT / "assets"
SHADERS_DIR = ASSETS_DIR / "shaders"
MODELS_DIR = ASSETS_DIR / "models"
TEXTURES_DIR = ASSETS_DIR / "textures"
OUTPUT_DIR = REPO_ROOT / "output"
RENDER_DIR = OUTPUT_DIR / "frames"

# Video settings
WIDTH = 1920
HEIGHT = 1080
FPS = 30


# ---------------------------------------------------------------------------
# Data types
# ---------------------------------------------------------------------------

@dataclass
class Vec3:
    x: float = 0.0
    y: float = 0.0
    z: float = 0.0

    def __add__(self, other):
        return Vec3(self.x + other.x, self.y + other.y, self.z + other.z)

    def __sub__(self, other):
        return Vec3(self.x - other.x, self.y - other.y, self.z - other.z)

    def __mul__(self, s):
        return Vec3(self.x * s, self.y * s, self.z * s)

    def dot(self, other):
        return self.x * other.x + self.y * other.y + self.z * other.z

    def cross(self, other):
        return Vec3(
            self.y * other.z - self.z * other.y,
            self.z * other.x - self.x * other.z,
            self.x * other.y - self.y * other.x
        )

    def length(self):
        return math.sqrt(self.x ** 2 + self.y ** 2 + self.z ** 2)

    def normalize(self):
        l = self.length()
        if l > 0:
            return Vec3(self.x / l, self.y / l, self.z / l)
        return Vec3()

    def to_tuple(self):
        return (self.x, self.y, self.z)


@dataclass
class Triangle:
    v0: Vec3
    v1: Vec3
    v2: Vec3
    color: Tuple[float, float, float] = (1.0, 1.0, 1.0)


@dataclass
class SceneObject:
    triangles: List[Triangle] = field(default_factory=list)
    position: Vec3 = field(default_factory=Vec3)
    rotation: Vec3 = field(default_factory=Vec3)
    scale: float = 1.0
    shader_id: str = "default"


@dataclass
class Scene:
    objects: List[SceneObject] = field(default_factory=list)
    camera: Vec3 = field(default_factory=lambda: Vec3(0, 0, 5))
    camera_target: Vec3 = field(default_factory=Vec3)
    background_color: Tuple[int, int, int] = (10, 10, 20)


# ---------------------------------------------------------------------------
# Shader system - translates shader code into frame effects
# ---------------------------------------------------------------------------

class Shader:
    """Base shader class."""
    def __init__(self, name: str, source: str = ""):
        self.name = name
        self.source = source
        self.uniforms: Dict[str, float] = {}

    def apply(self, frame: np.ndarray, time: float, audio_data: np.ndarray) -> np.ndarray:
        return frame


class GradientShader(Shader):
    """Animated gradient shader."""
    def apply(self, frame: np.ndarray, time: float, audio_data: np.ndarray) -> np.ndarray:
        h, w = frame.shape[:2]
        for y in range(0, h, 4):
            for x in range(0, w, 4):
                r = int((math.sin(time + x * 0.01) + 1) * 127)
                g = int((math.sin(time * 0.7 + y * 0.01) + 1) * 127)
                b = int((math.cos(time * 0.5 + (x + y) * 0.005) + 1) * 127)
                frame[y:y + 4, x:x + 4] = [r, g, b]
        return frame


class PlasmaShader(Shader):
    """Classic plasma effect shader."""
    def apply(self, frame: np.ndarray, time: float, audio_data: np.ndarray) -> np.ndarray:
        h, w = frame.shape[:2]
        for y in range(0, h, 2):
            for x in range(0, w, 2):
                v1 = math.sin(x * 0.02 + time)
                v2 = math.sin(y * 0.02 + time * 0.5)
                v3 = math.sin((x + y) * 0.01 + time * 0.3)
                v4 = math.sin(math.sqrt(x * x + y * y) * 0.01 + time * 0.7)

                v = (v1 + v2 + v3 + v4) / 4.0
                r = int((math.sin(v * 3.14159) + 1) * 127)
                g = int((math.sin(v * 3.14159 + 2.094) + 1) * 127)
                b = int((math.sin(v * 3.14159 + 4.188) + 1) * 127)

                frame[y:y + 2, x:x + 2] = [r, g, b]
        return frame


class AudioReactiveShader(Shader):
    """Audio-reactive shader using spectrum data."""
    def apply(self, frame: np.ndarray, time: float, audio_data: np.ndarray) -> np.ndarray:
        if len(audio_data) == 0:
            return frame

        h, w = frame.shape[:2]

        # Compute bass/mid/treble energy
        bass = np.mean(audio_data[:len(audio_data) // 4]) if len(audio_data) > 0 else 0
        mid = np.mean(audio_data[len(audio_data) // 4:len(audio_data) // 2]) if len(audio_data) > 1 else 0
        treble = np.mean(audio_data[len(audio_data) // 2:]) if len(audio_data) > 2 else 0

        bass = min(1.0, max(0.0, bass * 2.0))
        mid = min(1.0, max(0.0, mid * 2.0))
        treble = min(1.0, max(0.0, treble * 2.0))

        for y in range(0, h, 4):
            for x in range(0, w, 4):
                wave = math.sin(x * 0.01 + time * 2.0) * bass
                wave += math.sin(y * 0.015 + time * 1.5) * mid
                wave += math.cos((x - y) * 0.01 + time * 3.0) * treble

                r = int((math.sin(wave) + 1) * 127 * (0.5 + bass))
                g = int((math.sin(wave + 2.0) + 1) * 127 * (0.5 + mid))
                b = int((math.sin(wave + 4.0) + 1) * 127 * (0.5 + treble))

                frame[y:y + 4, x:x + 4] = [min(255, r), min(255, g), min(255, b)]

        return frame


class WireframeShader(Shader):
    """3D wireframe renderer for models."""
    def __init__(self, name: str = "wireframe"):
        super().__init__(name)

    def project(self, v: Vec3, w: int, h: int, fov: float = 600.0) -> Optional[Tuple[int, int]]:
        if v.z <= 0.1:
            return None
        scale = fov / v.z
        x = int(w / 2 + v.x * scale)
        y = int(h / 2 - v.y * scale)
        if 0 <= x < w and 0 <= y < h:
            return (x, y)
        return None

    def rotate(self, v: Vec3, rx: float, ry: float, rz: float) -> Vec3:
        # Rotate around X
        y, z = v.y * math.cos(rx) - v.z * math.sin(rx), v.y * math.sin(rx) + v.z * math.cos(rx)
        v = Vec3(v.x, y, z)
        # Rotate around Y
        x, z = v.x * math.cos(ry) + v.z * math.sin(ry), -v.x * math.sin(ry) + v.z * math.cos(ry)
        v = Vec3(x, v.y, z)
        # Rotate around Z
        x, y = v.x * math.cos(rz) - v.y * math.sin(rz), v.x * math.sin(rz) + v.y * math.cos(rz)
        return Vec3(x, y, v.z)

    def draw_triangle(self, draw, v0: Vec3, v1: Vec3, v2: Vec3, color: Tuple[int, int, int]):
        p0 = self.project(v0, WIDTH, HEIGHT)
        p1 = self.project(v1, WIDTH, HEIGHT)
        p2 = self.project(v2, WIDTH, HEIGHT)

        if p0 and p1 and p2:
            draw.polygon([p0, p1, p2], outline=color, width=1)

    def apply(self, frame: np.ndarray, time: float, audio_data: np.ndarray) -> np.ndarray:
        img = Image.fromarray(frame)
        draw = ImageDraw.Draw(img)

        # Create a rotating cube
        size = 1.5
        cube_verts = [
            Vec3(-size, -size, -size), Vec3(size, -size, -size),
            Vec3(size, size, -size), Vec3(-size, size, -size),
            Vec3(-size, -size, size), Vec3(size, -size, size),
            Vec3(size, size, size), Vec3(-size, size, size)
        ]
        cube_edges = [
            (0, 1), (1, 2), (2, 3), (3, 0),
            (4, 5), (5, 6), (6, 7), (7, 4),
            (0, 4), (1, 5), (2, 6), (3, 7)
        ]

        rx = time * 0.5
        ry = time * 0.3
        rz = time * 0.2

        rotated = [self.rotate(v, rx, ry, rz) for v in cube_verts]
        translated = [v + Vec3(0, 0, 4) for v in rotated]

        color = (100, 200, 255)
        for edge in cube_edges:
            p0 = self.project(translated[edge[0]], WIDTH, HEIGHT)
            p1 = self.project(translated[edge[1]], WIDTH, HEIGHT)
            if p0 and p1:
                draw.line([p0, p1], fill=color, width=2)

        return np.array(img)


# ---------------------------------------------------------------------------
# Shader Loader - imports shaders from Twilight Elysium asset repo
# ---------------------------------------------------------------------------

SHADER_REGISTRY = {
    "gradient": GradientShader,
    "plasma": PlasmaShader,
    "audio_reactive": AudioReactiveShader,
    "wireframe": WireframeShader,
}


def load_shaders_from_assets() -> Dict[str, Shader]:
    """Load shaders from Twilight Elysium asset repository."""
    shaders = {}

    # Register built-in shaders
    for name, shader_class in SHADER_REGISTRY.items():
        shaders[name] = shader_class(name)

    # Load glsl shaders from assets
    if SHADERS_DIR.exists():
        for shader_file in SHADERS_DIR.rglob("*.glsl"):
            try:
                source = shader_file.read_text()
                shader_name = shader_file.stem
                shaders[shader_name] = Shader(shader_name, source)
                print(f"[Shader] Loaded: {shader_name}")
            except Exception as e:
                print(f"[Shader] Failed to load {shader_file}: {e}")

    return shaders


# ---------------------------------------------------------------------------
# Model Loader - imports 3D models from Twilight Elysium asset repo
# ---------------------------------------------------------------------------

def load_models_from_assets() -> List[SceneObject]:
    """Load 3D models from assets directory."""
    objects = []

    if not MODELS_DIR.exists():
        return objects

    # Load glTF models
    for gltf_file in MODELS_DIR.rglob("*.gltf"):
        try:
            data = json.loads(gltf_file.read_text())
            obj = parse_gltf(data, gltf_file.parent)
            if obj:
                objects.append(obj)
                print(f"[Model] Loaded glTF: {gltf_file.name}")
        except Exception as e:
            print(f"[Model] Failed to load {gltf_file}: {e}")

    # Load OBJ models
    for obj_file in MODELS_DIR.rglob("*.obj"):
        try:
            obj = parse_obj(obj_file)
            if obj:
                objects.append(obj)
                print(f"[Model] Loaded OBJ: {obj_file.name}")
        except Exception as e:
            print(f"[Model] Failed to load {obj_file}: {e}")

    # Add procedural demo objects if no models loaded
    if not objects:
        print("[Model] No models found, adding procedural objects")
        objects.extend(create_procedural_objects())

    return objects


def parse_gltf(data: dict, base_path: Path) -> Optional[SceneObject]:
    """Parse glTF JSON into SceneObject."""
    triangles = []

    try:
        for mesh in data.get("meshes", []):
            for primitive in mesh.get("primitives", []):
                indices = primitive.get("indices", [])
                positions = primitive.get("attributes", {}).get("POSITION", [])

                if len(positions) >= 9:
                    # Triangulate
                    for i in range(0, len(positions) - 8, 9):
                        v0 = Vec3(*positions[i:i + 3])
                        v1 = Vec3(*positions[i + 3:i + 6])
                        v2 = Vec3(*positions[i + 6:i + 9])
                        triangles.append(Triangle(v0, v1, v2))
    except Exception as e:
        print(f"  [WARN] glTF parse error: {e}")

    if triangles:
        return SceneObject(triangles=triangles, shader_id="wireframe")
    return None


def parse_obj(file_path: Path) -> Optional[SceneObject]:
    """Parse OBJ file into SceneObject."""
    triangles = []
    verts = []

    try:
        for line in file_path.read_text().splitlines():
            if line.startswith("v "):
                parts = line.split()
                if len(parts) >= 4:
                    verts.append(Vec3(float(parts[1]), float(parts[2]), float(parts[3])))
            elif line.startswith("f "):
                parts = line.split()
                if len(parts) >= 4:
                    # Simple triangulation
                    idx = [int(p.split("/")[0]) - 1 for p in parts[1:4]]
                    if all(0 <= i < len(verts) for i in idx):
                        triangles.append(Triangle(verts[idx[0]], verts[idx[1]], verts[idx[2]]))
    except Exception as e:
        print(f"  [WARN] OBJ parse error: {e}")

    if triangles:
        return SceneObject(triangles=triangles, shader_id="wireframe")
    return None


def create_procedural_objects() -> List[SceneObject]:
    """Create procedural demo objects."""
    objects = []

    # Cube
    s = 1.0
    cube = SceneObject(
        triangles=[
            Triangle(Vec3(-s, -s, -s), Vec3(s, -s, -s), Vec3(s, s, -s)),
            Triangle(Vec3(-s, -s, -s), Vec3(s, s, -s), Vec3(-s, s, -s)),
            Triangle(Vec3(-s, -s, s), Vec3(s, -s, s), Vec3(s, s, s)),
            Triangle(Vec3(-s, -s, s), Vec3(s, s, s), Vec3(-s, s, s)),
            Triangle(Vec3(-s, -s, -s), Vec3(-s, s, -s), Vec3(-s, s, s)),
            Triangle(Vec3(-s, -s, -s), Vec3(-s, s, s), Vec3(-s, -s, s)),
            Triangle(Vec3(s, -s, -s), Vec3(s, s, -s), Vec3(s, s, s)),
            Triangle(Vec3(s, -s, -s), Vec3(s, s, s), Vec3(s, -s, s)),
            Triangle(Vec3(-s, -s, -s), Vec3(s, -s, -s), Vec3(s, -s, s)),
            Triangle(Vec3(-s, -s, -s), Vec3(s, -s, s), Vec3(-s, -s, s)),
            Triangle(Vec3(-s, s, -s), Vec3(s, s, -s), Vec3(s, s, s)),
            Triangle(Vec3(-s, s, -s), Vec3(s, s, s), Vec3(-s, s, s)),
        ],
        shader_id="wireframe",
        position=Vec3(-2.5, 0, 4)
    )
    objects.append(cube)

    # Pyramid
    h = 2.0
    b = 1.2
    pyramid = SceneObject(
        triangles=[
            Triangle(Vec3(0, h, 0), Vec3(-b, -h, b), Vec3(b, -h, b)),
            Triangle(Vec3(0, h, 0), Vec3(b, -h, b), Vec3(b, -h, -b)),
            Triangle(Vec3(0, h, 0), Vec3(b, -h, -b), Vec3(-b, -h, -b)),
            Triangle(Vec3(0, h, 0), Vec3(-b, -h, -b), Vec3(-b, -h, b)),
            Triangle(Vec3(-b, -h, b), Vec3(b, -h, b), Vec3(b, -h, -b)),
            Triangle(Vec3(-b, -h, b), Vec3(b, -h, -b), Vec3(-b, -h, -b)),
        ],
        shader_id="wireframe",
        position=Vec3(2.5, 0, 4)
    )
    objects.append(pyramid)

    return objects


# ---------------------------------------------------------------------------
# Audio Analyzer - extracts audio features for sync
# ---------------------------------------------------------------------------

class AudioAnalyzer:
    """Analyzes MP3 audio for beat detection and spectrum data."""

    def __init__(self, mp3_path: Path):
        self.mp3_path = mp3_path
        self.samples: np.ndarray = np.array([])
        self.sample_rate: int = 44100
        self.beats: List[float] = []
        self.spectrum_frames: List[np.ndarray] = []

    def extract(self) -> bool:
        """Extract audio samples from MP3 using ffmpeg."""
        try:
            # Use ffmpeg to extract raw PCM
            cmd = [
                "ffmpeg", "-i", str(self.mp3_path),
                "-f", "s16le", "-ac", "1", "-ar", "44100",
                "-acodec", "pcm_s16le", "-"
            ]
            result = subprocess.run(cmd, capture_output=True, timeout=30)
            if result.returncode != 0:
                print(f"[Audio] ffmpeg error: {result.stderr.decode()[:200]}")
                return False

            # Parse raw PCM
            raw = result.stdout
            self.samples = np.frombuffer(raw, dtype=np.int16).astype(np.float32) / 32768.0
            self.sample_rate = 44100

            # Detect beats
            self._detect_beats()
            # Pre-compute spectrum frames
            self._compute_spectrum()

            return True
        except Exception as e:
            print(f"[Audio] Extraction failed: {e}")
            return False

    def _detect_beats(self, hop_length: int = 512):
        """Simple beat detection via energy envelope."""
        if len(self.samples) == 0:
            return

        frame_size = 1024
        energies = []

        for i in range(0, len(self.samples) - frame_size, hop_length):
            frame = self.samples[i:i + frame_size]
            energies.append(np.sqrt(np.mean(frame ** 2)))

        energies = np.array(energies)
        if len(energies) == 0:
            return

        threshold = np.mean(energies) * 1.5
        hop_time = hop_length / self.sample_rate

        for i, e in enumerate(energies):
            if e > threshold:
                self.beats.append(i * hop_time)

    def _compute_spectrum(self, frame_size: int = 2048):
        """Pre-compute FFT spectrum frames."""
        if len(self.samples) == 0:
            return

        hop = frame_size // 2
        for i in range(0, len(self.samples) - frame_size, hop):
            frame = self.samples[i:i + frame_size]
            windowed = frame * np.hanning(len(frame))
            fft = np.abs(np.fft.rfft(windowed))
            self.spectrum_frames.append(fft)

    def get_spectrum_at_time(self, time: float) -> np.ndarray:
        """Get spectrum data at a specific time."""
        frame_idx = int(time * self.sample_rate / 1024)
        if 0 <= frame_idx < len(self.spectrum_frames):
            return self.spectrum_frames[frame_idx]
        return np.array([])

    def is_beat(self, time: float, tolerance: float = 0.05) -> bool:
        """Check if there's a beat at the given time."""
        for b in self.beats:
            if abs(b - time) < tolerance:
                return True
        return False

    def get_energy(self, time: float) -> float:
        """Get audio energy at a specific time."""
        frame_idx = int(time * self.sample_rate / 1024)
        if 0 <= frame_idx < len(self.spectrum_frames):
            return np.mean(self.spectrum_frames[frame_idx])
        return 0.0


# ---------------------------------------------------------------------------
# Software Renderer
# ---------------------------------------------------------------------------

class SoftwareRenderer:
    """Software renderer that renders scenes to frames."""

    def __init__(self, width: int = WIDTH, height: int = HEIGHT):
        self.width = width
        self.height = height
        self.shaders = load_shaders_from_assets()
        self.scene = Scene()

    def set_scene(self, scene: Scene):
        self.scene = scene

    def render_frame(self, time: float, audio_analyzer: Optional[AudioAnalyzer] = None) -> np.ndarray:
        """Render a single frame."""
        # Start with background
        frame = np.full((self.height, self.width, 3), self.scene.background_color, dtype=np.uint8)

        # Apply active shader
        active_shader = self.shaders.get("wireframe", WireframeShader())
        if audio_analyzer and audio_analyzer.spectrum_frames:
            spectrum = audio_analyzer.get_spectrum_at_time(time)
            frame = active_shader.apply(frame, time, spectrum)
        else:
            frame = active_shader.apply(frame, time, np.array([]))

        # Render 3D objects
        for obj in self.scene.objects:
            if obj.shader_id in self.shaders:
                shader = self.shaders[obj.shader_id]
                frame = shader.apply(frame, time, np.array([]))

        return frame

    def render_frames(self, duration: float, fps: int, audio_analyzer: Optional[AudioAnalyzer] = None) -> List[np.ndarray]:
        """Render all frames for a video."""
        total_frames = int(duration * fps)
        frames = []

        print(f"[Renderer] Rendering {total_frames} frames at {fps}fps...")

        for i in range(total_frames):
            time = i / fps
            frame = self.render_frame(time, audio_analyzer)
            frames.append(frame)

            if (i + 1) % 30 == 0:
                print(f"[Renderer] Frame {i + 1}/{total_frames}")

        print(f"[Renderer] Complete: {len(frames)} frames")
        return frames


# ---------------------------------------------------------------------------
# Video Composer - assembles frames + audio into MP4
# ---------------------------------------------------------------------------

class VideoComposer:
    """Composes final MP4 from rendered frames and audio."""

    def __init__(self, output_path: Path, fps: int = FPS):
        self.output_path = output_path
        self.fps = fps

    def compose(self, frames: List[np.ndarray], audio_path: Optional[Path] = None):
        """Compose frames and audio into final video."""
        if not frames:
            print("[Composer] No frames to compose")
            return False

        # Save frames as temporary images
        with tempfile.TemporaryDirectory() as tmpdir:
            frame_pattern = os.path.join(tmpdir, "frame_%06d.png")

            print(f"[Composer] Saving {len(frames)} frames...")
            for i, frame in enumerate(frames):
                img = Image.fromarray(frame)
                img.save(frame_pattern % i)

            # Build ffmpeg command
            cmd = [
                "ffmpeg", "-y",
                "-framerate", str(self.fps),
                "-i", frame_pattern,
                "-pix_fmt", "yuv420p",
                "-c:v", "libx264",
                "-preset", "fast",
                "-crf", "18"
            ]

            if audio_path and audio_path.exists():
                cmd.extend([
                    "-i", str(audio_path),
                    "-c:a", "aac",
                    "-b:a", "192k",
                    "-shortest"
                ])

            cmd.append(str(self.output_path))

            print(f"[Composer] Encoding video: {self.output_path}")
            result = subprocess.run(cmd, capture_output=True, timeout=120)

            if result.returncode == 0:
                print(f"[Composer] Video created: {self.output_path}")
                return True
            else:
                print(f"[Composer] ffmpeg error: {result.stderr.decode()[:500]}")
                return False


# ---------------------------------------------------------------------------
# Pipeline orchestrator
# ---------------------------------------------------------------------------

def run_pipeline(mp3_path: Path, output_name: str, scene: Optional[Scene] = None):
    """Run the full rendering pipeline: MP3 -> Frames -> MP4."""
    print("=" * 60)
    print("Twilight Elysium Universal Renderer")
    print("=" * 60)
    print(f"Input: {mp3_path}")
    print(f"Output: {output_name}")
    print()

    # Create output directory
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # Step 1: Analyze audio
    print("[Pipeline] Step 1: Analyzing audio...")
    analyzer = AudioAnalyzer(mp3_path)
    if not analyzer.extract():
        print("[Pipeline] Audio extraction failed, rendering without audio sync")
        duration = 10.0
        analyzer = None
    else:
        duration = len(analyzer.samples) / analyzer.sample_rate
        print(f"[Pipeline] Duration: {duration:.1f}s, Beats: {len(analyzer.beats)}")

    # Step 2: Load scene
    print("\n[Pipeline] Step 2: Loading scene...")
    if scene is None:
        scene = Scene()
        # Load assets from Twilight Elysium repo
        objects = load_models_from_assets()
        scene.objects.extend(objects)

    renderer = SoftwareRenderer()
    renderer.set_scene(scene)

    # Step 3: Render frames
    print("\n[Pipeline] Step 3: Rendering frames...")
    frames = renderer.render_frames(duration, FPS, analyzer)

    # Step 4: Compose video
    print("\n[Pipeline] Step 4: Composing video...")
    output_path = OUTPUT_DIR / f"{output_name}.mp4"
    composer = VideoComposer(output_path)
    success = composer.compose(frames, mp3_path if analyzer else None)

    # Step 5: Commit to git
    if success:
        print("\n[Pipeline] Step 5: Committing to git...")
        os.system(f"cd {REPO_ROOT} && git add output/ && git commit -m 'feat: render {output_name}.mp4'")

    print("\n" + "=" * 60)
    if success:
        print(f"SUCCESS: {output_path}")
    else:
        print("FAILED: Video composition failed")
    print("=" * 60)

    return success


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    if len(sys.argv) < 2:
        print("Usage: renderer.py <mp3_file> [output_name]")
        print("  mp3_file: Path to input MP3")
        print("  output_name: Output video name (default: output)")
        sys.exit(1)

    mp3_path = Path(sys.argv[1])
    if not mp3_path.exists():
        print(f"Error: MP3 file not found: {mp3_path}")
        sys.exit(1)

    output_name = sys.argv[2] if len(sys.argv) > 2 else mp3_path.stem

    run_pipeline(mp3_path, output_name)


if __name__ == "__main__":
    main()
