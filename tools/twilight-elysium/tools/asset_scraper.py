#!/usr/bin/env python3
"""
Twilight Elysium Asset Scraper
================================
Scrapes openly licensed shaders and 3D models from the web
and saves them to the Twilight Elysium git repository.

Sources:
  - Shadertoy (CC-BY-NC, CC-BY, CC0)
  - glTF Sample Models (Khronos, MIT/CC0)
  - OpenGameArt.org (CC0, CC-BY, MIT, GPL)
  - Poly Haven (CC0)

Usage:
    scraper.py --type shaders
    scraper.py --type models
    scraper.py --type all
    scraper.py --dry-run
"""

import os
import sys
import json
import time
import hashlib
import urllib.request
import urllib.error
import urllib.parse
import re
import argparse
from pathlib import Path
from datetime import datetime, timezone
from typing import Dict, List, Optional, Tuple

# Twilight Elysium repo root
REPO_ROOT = Path(__file__).resolve().parents[3]
ASSETS_DIR = REPO_ROOT / "assets"
SHADERS_DIR = ASSETS_DIR / "shaders"
MODELS_DIR = ASSETS_DIR / "models"
TEXTURES_DIR = ASSETS_DIR / "textures"
MATERIALS_DIR = ASSETS_DIR / "materials"

# Manifest file
MANIFEST_FILE = ASSETS_DIR / "asset_manifest.json"

# Rate limiting
REQUEST_DELAY = 1.0  # seconds between requests
USER_AGENT = "TwilightElysium-AssetScraper/1.0 (https://github.com/intricos777/twilight-elysium)"

# License whitelist
ALLOWED_LICENSES = {
    "CC0", "MIT", "BSD", "Apache", "Public Domain",
    "CC-BY", "CC-BY-SA", "CC-BY-NC", "CC-BY-NC-SA",
    "LGPL", "GPL"
}


class AssetManifest:
    """Tracks all downloaded assets with metadata."""
    
    def __init__(self):
        self.assets: List[Dict] = []
        self.load()
    
    def load(self):
        if MANIFEST_FILE.exists():
            self.assets = json.loads(MANIFEST_FILE.read_text())
    
    def save(self):
        MANIFEST_FILE.parent.mkdir(parents=True, exist_ok=True)
        MANIFEST_FILE.write_text(json.dumps(self.assets, indent=2))
    
    def add(self, asset_type: str, name: str, source: str, url: str,
            license_type: str, file_path: str, sha256: str,
            description: str = "", author: str = ""):
        entry = {
            "type": asset_type,
            "name": name,
            "description": description,
            "author": author,
            "source": source,
            "url": url,
            "license": license_type,
            "file_path": file_path,
            "sha256": sha256,
            "downloaded_at": datetime.now(timezone.utc).isoformat()
        }
        self.assets.append(entry)
        self.save()
    
    def has(self, sha256: str) -> bool:
        return any(a["sha256"] == sha256 for a in self.assets)


manifest = AssetManifest()


def download_file(url: str, dest: Path, delay: float = REQUEST_DELAY) -> Optional[bytes]:
    """Download a file with rate limiting."""
    time.sleep(delay)
    
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            data = resp.read()
            dest.write_bytes(data)
            return data
    except urllib.error.HTTPError as e:
        print(f"  [ERROR] HTTP {e.code}: {url}")
        return None
    except Exception as e:
        print(f"  [ERROR] {type(e).__name__}: {url}")
        return None


def sha256_of(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sanitize_filename(name: str) -> str:
    """Sanitize a string for use as a filename."""
    name = re.sub(r'[^\w\s\-_.]', '_', name)
    name = re.sub(r'\s+', '_', name)
    return name[:200]  # limit length


# ---------------------------------------------------------------------------
# Source: glTF Sample Models (Khronos)
# ---------------------------------------------------------------------------

GLTF_SAMPLE_BASE = "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/main/2.0/"

GLTF_MODELS = [
    ("Box", "Simple box geometry", "MIT"),
    ("BoxTextured", "Box with texture", "MIT"),
    ("Triangle", "Minimal triangle", "MIT"),
    ("Avocado", "Avocado model", "MIT"),
    ("BoomBox", "Boom box with PBR materials", "MIT"),
    ("BrainStem", "Brain stem scan", "MIT"),
    ("Corset", "Corset model", "MIT"),
    ("DamagedHelmet", "PBR damaged helmet", "MIT"),
    ("DragonAttenuation", "Dragon with attenuation", "MIT"),
    ("Duck", "Rubber duck", "MIT"),
    ("FlightHelmet", "Flight helmet PBR", "MIT"),
    ("InterpolationTest", "Animation interpolation test", "MIT"),
    ("Lantern", "Lantern model", "MIT"),
    ("MetalRoughSpheres", "Metal/roughness sphere test", "MIT"),
    ("MetalRoughSpheresNoTexture", "Spheres without textures", "MIT"),
    ("NormalTangentMirrorTest", "Normal/tangent test", "MIT"),
    ("NormalTangentTest", "Normal/tangent test", "MIT"),
    ("OrientationTest", "Orientation test", "MIT"),
    ("PointLight", "Point light test", "MIT"),
    ("ReciprocatingSaw", "Animated saw", "MIT"),
    ("RiggedFigure", "Rigged figure", "MIT"),
    ("RiggedSimple", "Simple rigged figure", "MIT"),
    ("SpecGlossVsMetalRough", "Specular vs metal-rough", "MIT"),
    ("Sponza", "Sponza atrium (subset)", "MIT"),
    (" Suzanne", "Monkey head", "MIT"),
    ("TextureCoordinateTest", "UV test", "MIT"),
    ("TextureSettingsTest", "Texture settings test", "MIT"),
    ("TwoSidedPlane", "Two-sided plane", "MIT"),
    ("VC", "Vertex color test", "MIT"),
    ("VertexColorTest", "Vertex color test", "MIT"),
    ("WaterBottle", "Water bottle PBR", "MIT"),
]

GLTF_FORMATS = [".gltf", ".glb", ".bin", ".png", ".jpg", ".jpeg"]


def scrape_gltf_models(dry_run: bool = False) -> List[str]:
    """Download Khronos glTF sample models."""
    downloaded = []
    
    for model_name, description, license_type in GLTF_MODELS:
        model_dir = MODELS_DIR / "gltf" / sanitize_filename(model_name)
        model_dir.mkdir(parents=True, exist_ok=True)
        
        print(f"[glTF] {model_name} - {description}")
        
        # Try both gltf and glb formats
        for fmt in ["gltf", "glb"]:
            base_url = f"{GLTF_SAMPLE_BASE}{model_name}/"
            filename = f"{model_name}.{fmt}"
            dest = model_dir / filename
            
            if dest.exists() and dest.stat().st_size > 0:
                print(f"  [SKIP] {filename} exists")
                downloaded.append(str(dest))
                break
            
            if dry_run:
                print(f"  [DRY-RUN] Would download: {base_url}{filename}")
                downloaded.append(str(dest))
                break
            
            data = download_file(base_url + filename, dest)
            if data:
                sha = sha256_of(data)
                if not manifest.has(sha):
                    manifest.add(
                        asset_type="model",
                        name=model_name,
                        description=description,
                        author="Khronos Group",
                        source="Khronos glTF Sample Models",
                        url=base_url + filename,
                        license_type=license_type,
                        file_path=str(dest.relative_to(REPO_ROOT)),
                        sha256=sha
                    )
                downloaded.append(str(dest))
                print(f"  [OK] {filename} ({len(data)} bytes)")
                break
            else:
                print(f"  [FAIL] {filename}")
        
        # Download textures if gltf format
        gltf_file = model_dir / f"{model_name}.gltf"
        if gltf_file.exists():
            try:
                gltf_data = json.loads(gltf_file.read_text())
                for img in gltf_data.get("images", []):
                    uri = img.get("uri", "")
                    if uri and not (model_dir / uri).exists():
                        img_url = base_url + uri
                        img_dest = model_dir / uri
                        img_data = download_file(img_url, img_dest, delay=0.5)
                        if img_data:
                            downloaded.append(str(img_dest))
                            print(f"  [TEX] {uri}")
            except Exception as e:
                print(f"  [WARN] Texture parse error: {e}")
    
    return downloaded


# ---------------------------------------------------------------------------
# Source: Shadertoy (via API)
# ---------------------------------------------------------------------------

SHADERTOY_API = "https://www.shadertoy.com/api/v1/shaders"
SHADERTOY_QUERIES = [
    "pbr",
    "raymarching",
    "lighting",
    "water",
    "fire",
    "clouds",
    "terrain",
    "volumetric"
]


def scrape_shadertoy(dry_run: bool = False) -> List[str]:
    """Download shaders from Shadertoy."""
    downloaded = []
    
    for query in SHADERTOY_QUERIES:
        print(f"[Shadertoy] Query: {query}")
        
        # Search shaders
        search_url = f"{SHADERTOY_API}?query={urllib.parse.quote(query)}&key=anonymous&num=5"
        
        if dry_run:
            print(f"  [DRY-RUN] Would search: {search_url}")
            continue
        
        time.sleep(REQUEST_DELAY)
        try:
            req = urllib.request.Request(search_url, headers={"User-Agent": USER_AGENT})
            with urllib.request.urlopen(req, timeout=30) as resp:
                data = json.loads(resp.read())
        except Exception as e:
            print(f"  [ERROR] Search failed: {e}")
            continue
        
        results = data.get("Results", [])
        if not results:
            print(f"  [INFO] No results for: {query}")
            continue
        
        for shader in results[:3]:  # Top 3 per query
            shader_id = shader.get("id", "")
            shader_name = shader.get("name", shader_id)
            author = shader.get("username", "unknown")
            license_type = shader.get("license", "Unknown")
            
            # Check license
            if not any(l.lower() in license_type.lower() for l in ALLOWED_LICENSES):
                print(f"  [SKIP] {shader_name} - license: {license_type}")
                continue
            
            print(f"  [SHADER] {shader_name} by {author}")
            
            # Fetch shader details
            shader_url = f"{SHADERTOY_API}/{shader_id}?key=anonymous"
            time.sleep(REQUEST_DELAY)
            try:
                req = urllib.request.Request(shader_url, headers={"User-Agent": USER_AGENT})
                with urllib.request.urlopen(req, timeout=30) as resp:
                    shader_data = json.loads(resp.read())
            except Exception as e:
                print(f"    [ERROR] Fetch failed: {e}")
                continue
            
            # Extract shader code
            shader_pass = shader_data.get("Shader", {}).get("renderpass", [{}])[0]
            code = shader_pass.get("code", "")
            if not code:
                print(f"    [SKIP] No code")
                continue
            
            # Save shader
            shader_filename = sanitize_filename(f"{author}_{shader_name}") + ".glsl"
            shader_path = SHADERS_DIR / "shadertoy" / shader_filename
            shader_path.parent.mkdir(parents=True, exist_ok=True)
            
            # Add attribution header
            header = f"""// Shadertoy Shader
// Name: {shader_name}
// Author: {author}
// Source: https://www.shadertoy.com/view/{shader_id}
// License: {license_type}
// Downloaded: {datetime.now(timezone.utc).isoformat()}
//
// Original shader code follows:

"""
            
            full_code = header + code
            shader_path.write_text(full_code)
            
            sha = sha256_of(full_code.encode())
            if not manifest.has(sha):
                manifest.add(
                    asset_type="shader",
                    name=shader_name,
                    description=f"Shadertoy shader: {query}",
                    author=author,
                    source="Shadertoy",
                    url=f"https://www.shadertoy.com/view/{shader_id}",
                    license_type=license_type,
                    file_path=str(shader_path.relative_to(REPO_ROOT)),
                    sha256=sha
                )
            
            downloaded.append(str(shader_path))
            print(f"    [OK] {shader_filename}")
    
    return downloaded


# ---------------------------------------------------------------------------
# Source: Poly Haven (CC0 textures/models)
# ---------------------------------------------------------------------------

POLY_HAVEN_BASE = "https://dl.polyhaven.org/file/ph-assets"

POLY_HAVEN_TEXTURES = [
    ("concrete", "Concrete texture"),
    ("wood", "Wood texture"),
    ("metal", "Metal texture"),
    ("stone", "Stone texture"),
    ("brick", "Brick texture"),
    ("grass", "Grass texture"),
    ("sand", "Sand texture"),
    ("snow", "Snow texture"),
]


def scrape_poly_haven(dry_run: bool = False) -> List[str]:
    """Download CC0 textures from Poly Haven."""
    downloaded = []
    
    for texture_name, description in POLY_HAVEN_TEXTURES:
        print(f"[Poly Haven] {texture_name}")
        
        # Poly Haven direct download links (1k resolution)
        tex_dir = TEXTURES_DIR / "polyhaven" / sanitize_filename(texture_name)
        tex_dir.mkdir(parents=True, exist_ok=True)
        
        for variant in ["diffuse", "roughness", "normal", "displacement"]:
            filename = f"{texture_name}_{variant}_1k.jpg"
            dest = tex_dir / filename
            
            if dest.exists() and dest.stat().st_size > 0:
                print(f"  [SKIP] {filename}")
                downloaded.append(str(dest))
                continue
            
            url = f"{POLY_HAVEN_BASE}/textures/jpg/{texture_name}/{variant}/1k/{filename}"
            
            if dry_run:
                print(f"  [DRY-RUN] Would download: {url}")
                downloaded.append(str(dest))
                continue
            
            data = download_file(url, dest, delay=0.5)
            if data:
                sha = sha256_of(data)
                if not manifest.has(sha):
                    manifest.add(
                        asset_type="texture",
                        name=f"{texture_name}_{variant}",
                        description=description,
                        author="Poly Haven",
                        source="Poly Haven",
                        url=url,
                        license_type="CC0",
                        file_path=str(dest.relative_to(REPO_ROOT)),
                        sha256=sha
                    )
                downloaded.append(str(dest))
                print(f"  [OK] {filename} ({len(data)} bytes)")
            else:
                print(f"  [FAIL] {filename}")
    
    return downloaded


# ---------------------------------------------------------------------------
# Source: OpenGameArt.org
# ---------------------------------------------------------------------------

OPENGAMEART_SEARCH = "https://opengameart.org/art_search/advanced?keys={query}&title=1&desc=1&username=1&term_node_tid_depth=All&term_node_tid_depth_1=All&term_node_tid_depth_2=All&term_node_tid_depth_3=All&term_node_tid_depth_4=All&term_node_tid_depth_5=All&term_node_tid_depth_6=All&sort_by=count&sort_order=DESC"

OPENGAMEART_QUERIES = [
    "low poly tree",
    "low poly rock",
    "low poly house",
    "fantasy sword",
    "potion bottle",
    "chest"
]


def scrape_opengameart(dry_run: bool = False) -> List[str]:
    """Download CC0/CC-BY models from OpenGameArt.org."""
    downloaded = []
    
    for query in OPENGAMEART_QUERIES:
        print(f"[OpenGameArt] Query: {query}")
        
        if dry_run:
            print(f"  [DRY-RUN] Would search: {query}")
            continue
        
        # OpenGameArt requires browser automation for search results
        # For now, we log the intent and skip
        print(f"  [INFO] OpenGameArt requires browser automation - skipping")
        print(f"  [INFO] Manual URL: https://opengameart.org/art_search/advanced?keys={urllib.parse.quote(query)}")
    
    return downloaded


# ---------------------------------------------------------------------------
# Asset organizer
# ---------------------------------------------------------------------------

def organize_assets(downloaded: List[str]):
    """Organize downloaded assets into Twilight Elysium structure."""
    print("\n[Organizer] Organizing assets...")
    
    # Create directory structure
    dirs = {
        "shaders": ["shadertoy", "hlsl", "glsl", "spirv"],
        "models": ["gltf", "obj", "fbx", "blend"],
        "textures": ["polyhaven", "generated", "ui"],
        "materials": ["pbr", "unlit", "terrain"]
    }
    
    for category, subdirs in dirs.items():
        for subdir in subdirs:
            (ASSETS_DIR / category / subdir).mkdir(parents=True, exist_ok=True)
    
    # Create asset index file
    index_file = ASSETS_DIR / "ASSET_INDEX.md"
    index_content = f"""# Asset Index
Generated: {datetime.now(timezone.utc).isoformat()}

## Shaders
See `shaders/` directory for GLSL/HLSL shader files.

## Models
See `models/` directory for glTF/OBJ/FBX models.

## Textures
See `textures/` directory for PBR texture maps.

## Materials
See `materials/` directory for material definitions.

## Manifest
See `asset_manifest.json` for full asset metadata.
"""
    
    index_file.write_text(index_content)
    print(f"[Organizer] Created {index_file}")
    
    # Stats
    shader_count = len(list(SHADERS_DIR.rglob("*.glsl"))) + len(list(SHADERS_DIR.rglob("*.hlsl")))
    model_count = len(list(MODELS_DIR.rglob("*.gltf"))) + len(list(MODELS_DIR.rglob("*.glb"))) + len(list(MODELS_DIR.rglob("*.obj")))
    texture_count = len(list(TEXTURES_DIR.rglob("*.jpg"))) + len(list(TEXTURES_DIR.rglob("*.png")))
    
    print(f"[Organizer] Stats:")
    print(f"  Shaders: {shader_count}")
    print(f"  Models: {model_count}")
    print(f"  Textures: {texture_count}")


def commit_to_git(message: str):
    """Commit downloaded assets to git."""
    print(f"\n[Git] Committing: {message}")
    
    os.chdir(REPO_ROOT)
    
    # Git add
    os.system("git add assets/")
    
    # Git commit
    os.system(f'git commit -m "feat(assets): {message}"')
    
    print("[Git] Done")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="Twilight Elysium Asset Scraper")
    parser.add_argument("--type", choices=["shaders", "models", "textures", "all"],
                       default="all", help="Asset type to scrape")
    parser.add_argument("--dry-run", action="store_true",
                       help="Don't actually download, just show what would happen")
    parser.add_argument("--commit", action="store_true",
                       help="Commit downloaded assets to git")
    
    args = parser.parse_args()
    
    print("=" * 60)
    print("Twilight Elysium Asset Scraper")
    print("=" * 60)
    print(f"Repo: {REPO_ROOT}")
    print(f"Type: {args.type}")
    print(f"Dry-run: {args.dry_run}")
    print()
    
    downloaded = []
    
    if args.type in ["shaders", "all"]:
        downloaded.extend(scrape_shadertoy(args.dry_run))
    
    if args.type in ["models", "all"]:
        downloaded.extend(scrape_gltf_models(args.dry_run))
    
    if args.type in ["textures", "all"]:
        downloaded.extend(scrape_poly_haven(args.dry_run))
    
    # Organize
    organize_assets(downloaded)
    
    # Commit
    if args.commit and not args.dry_run:
        commit_to_git(f"scraped {len(downloaded)} {args.type}")
    
    print("\n" + "=" * 60)
    print(f"Downloaded: {len(downloaded)} assets")
    print(f"Manifest: {MANIFEST_FILE}")
    print("=" * 60)


if __name__ == "__main__":
    main()
