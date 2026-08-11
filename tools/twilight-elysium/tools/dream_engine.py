#!/usr/bin/env python3
"""
Twilight Elysium Dream Engine
==============================
Reads MP3 audio + game scripts (Dark Souls JSON data) and procedurally
generates 3D renderings that "dream" the game world into existence.

Based on DS1/DS2/DS3 data: weapons, enemies, maps, combat.
"""

from __future__ import annotations
import json
import math
import random
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import numpy as np

from renderer import Vec3, Scene, SceneObject, Triangle, Shader, WireframeShader

REPO_ROOT = Path(__file__).resolve().parents[3]
CONTENT_DIR = REPO_ROOT / "Content"
ASSETS_DIR = REPO_ROOT / "assets"


class AudioDreamState:
    """State extracted from audio for dream generation."""
    def __init__(self):
        self.beat: bool = False
        self.energy: float = 0.0
        self.bass: float = 0.0
        self.mid: float = 0.0
        self.treble: float = 0.0
        self.spectrum: np.ndarray = np.zeros(64)
        self.time: float = 0.0
        self.phase: float = 0.0


class Directive:
    """Narrative directive that can override generation behavior."""
    def __init__(self, kind: str, payload: Optional[Dict] = None):
        self.kind = kind
        self.payload = payload or {}

    def __contains__(self, key: str) -> bool:
        return key in self.payload

    def get(self, key: str, default: Any = None) -> Any:
        return self.payload.get(key, default)


class ScriptDreamState:
    """State extracted from game scripts for dream generation."""
    def __init__(self):
        self.weapons: List[Dict] = []
        self.enemies: List[Dict] = []
        self.locations: List[Dict] = []
        self.combat_events: List[Dict] = []
        self.character_events: List[Dict] = []
        self.current_location: Optional[Dict] = None
        self.current_enemy: Optional[Dict] = None
        self.current_weapon: Optional[Dict] = None
        self.directives: List[Directive] = []

    def apply_directives(self) -> None:
        """Mutate state based on loaded directives."""
        if not self.directives:
            return
        for d in self.directives:
            kind = d.kind.lower()
            if kind == "hero_journey" or kind == "monomyth":
                self.weapons = self.weapons[: max(0, len(self.weapons) - 2)]
                self.enemies = self.enemies[: max(0, len(self.enemies) - 1)]
                self.locations = self.locations[: max(1, len(self.locations) - 1)]
            elif kind == "tragedy":
                self.enemies = self.enemies * 2
                self.weapons = self.weapons[:1]
            elif kind == "boss_fight":
                if self.enemies:
                    self.enemies = self.enemies[:1]
                    self.current_enemy = self.enemies[0]
            elif kind == "exploration" or kind == "peaceful":
                self.enemies = []
                self.weapons = []
            elif kind == "character_event":
                name = d.get("name")
                if name:
                    matched = [e for e in self.character_events if name.lower() in str(e).lower()]
                    if matched:
                        self.current_enemy = matched[0]
            elif kind == "force_character":
                ctype = d.get("type", "knight")
                forced = {
                    "name": d.get("name", ctype),
                    "type": ctype,
                    "weapon": d.get("weapon", "longsword"),
                    "position": d.get("position", [0, 0, 0]),
                }
                self.current_enemy = forced


class DreamGenerator:
    """
    Procedurally generates 3D scenes from audio + game script data.
    This is the 'dreaming' layer — music and scripts become geometry.
    """
    
    def __init__(self):
        self.audio_state = AudioDreamState()
        self.script_state = ScriptDreamState()
        self.dream_objects: List[SceneObject] = []
        self.dream_particles: List[Tuple[float, float, float, float]] = []  # x,y,z,life
        self.seed: float = 0.0
        
    def load_scripts(self):
        """Load Dark Souls script data."""
        # Load weapons from Dark Souls remake trilogy
        weapons_candidates = [
            CONTENT_DIR / "Data" / "DS1_Weapons.json",
            REPO_ROOT.parent / "DarkSoulsRemakeTrilogy" / "Content" / "Data" / "DS1_Weapons.json",
            REPO_ROOT.parent / "DarkSoulsRemakeTrilogy" / "Content" / "Data" / "DS2_Weapons.json",
            REPO_ROOT.parent / "DarkSoulsRemakeTrilogy" / "Content" / "Data" / "DS3_Weapons.json",
        ]
        for weapons_file in weapons_candidates:
            if weapons_file.exists():
                try:
                    data = json.loads(weapons_file.read_text())
                    items = data if isinstance(data, list) else data.get("weapons", [])
                    self.script_state.weapons.extend(items)
                except Exception:
                    pass
        
        # Load enemies from Dark Souls remake trilogy
        enemies_candidates = [
            CONTENT_DIR / "Data" / "DS1_Enemies.json",
            REPO_ROOT.parent / "DarkSoulsRemakeTrilogy" / "Content" / "Data" / "DS1_Enemies.json",
            REPO_ROOT.parent / "DarkSoulsRemakeTrilogy" / "Content" / "Data" / "DS2_Enemies.json",
            REPO_ROOT.parent / "DarkSoulsRemakeTrilogy" / "Content" / "Data" / "DS3_Enemies.json",
        ]
        for enemies_file in enemies_candidates:
            if enemies_file.exists():
                try:
                    data = json.loads(enemies_file.read_text())
                    items = data if isinstance(data, list) else data.get("enemies", [])
                    self.script_state.enemies.extend(items)
                except Exception:
                    pass
        
        # Load maps from Dark Souls remake trilogy
        maps_candidates = [
            CONTENT_DIR / "Maps",
            REPO_ROOT.parent / "DarkSoulsRemakeTrilogy" / "Content" / "Maps",
        ]
        for maps_dir in maps_candidates:
            if maps_dir.exists():
                for map_file in maps_dir.glob("*.json"):
                    try:
                        data = json.loads(map_file.read_text())
                        self.script_state.locations.append({
                            "name": map_file.stem,
                            "data": data
                        })
                    except Exception:
                        pass
        
        # Set initial location
        if self.script_state.locations:
            self.script_state.current_location = self.script_state.locations[0]
    
    def update_audio(self, audio_analyzer, time: float):
        """Update dream state from audio analysis."""
        self.audio_state.time = time
        self.audio_state.phase = time * 2.0 * math.pi
        
        if audio_analyzer:
            self.audio_state.beat = audio_analyzer.is_beat(time)
            self.audio_state.energy = audio_analyzer.get_energy(time)
            spec = audio_analyzer.get_spectrum_at_time(time)
            if len(spec) >= 64:
                self.audio_state.spectrum = spec[:64]
            elif len(spec) > 0:
                self.audio_state.spectrum = np.pad(spec, (0, 64 - len(spec)))
            
            # Frequency bands
            if len(spec) >= 32:
                self.audio_state.bass = np.mean(spec[:8])
                self.audio_state.mid = np.mean(spec[8:32])
                self.audio_state.treble = np.mean(spec[32:])
            else:
                self.audio_state.bass = self.audio_state.mid = self.audio_state.treble = 0.0
    
    def _create_weapon_dream(self, weapon: Dict, t: float) -> SceneObject:
        """Create a procedural visualization of a weapon from DS script data."""
        name = weapon.get("name", "Unknown")
        weapon_class = weapon.get("weaponClass", weapon.get("type", "StraightSword"))
        base_damage = weapon.get("baseDamage", 100.0)
        weight = weapon.get("weight", 3.0)
        
        triangles = []
        damage_scale = max(0.6, min(2.5, base_damage / 120.0))
        length = max(0.8, 4.0 - weight / 3.0)
        length *= (0.9 + 0.2 * math.sin(t * 0.5))
        
        cls = weapon_class.lower()
        if any(k in cls for k in ["greatsword", "ultra", "claymore"]):
            blade_len = length * 1.4
            triangles.extend([
                Triangle(Vec3(0, 0, 0), Vec3(0, blade_len, 0), Vec3(0.25, 0, 0)),
                Triangle(Vec3(0, 0, 0), Vec3(0, blade_len, 0), Vec3(-0.25, 0, 0)),
                Triangle(Vec3(0, blade_len, 0), Vec3(0.25, 0, 0), Vec3(-0.25, 0, 0)),
            ])
        elif any(k in cls for k in ["katana", "curved", " uchigatana"]):
            triangles.extend([
                Triangle(Vec3(0, 0, 0), Vec3(0, length * 1.1, 0.2), Vec3(0.2, 0, 0)),
                Triangle(Vec3(0, 0, 0), Vec3(0, length * 1.1, 0.2), Vec3(-0.2, 0, 0)),
            ])
        elif any(k in cls for k in ["spear", "halberd", "lance"]):
            triangles.extend([
                Triangle(Vec3(0, 0, 0), Vec3(0, length * 1.5, 0), Vec3(0.12, 0, 0)),
                Triangle(Vec3(0, 0, 0), Vec3(0, length * 1.5, 0), Vec3(-0.12, 0, 0)),
                Triangle(Vec3(0, length * 1.5, 0), Vec3(0.25, length * 1.6, 0), Vec3(0, length * 1.2, 0)),
            ])
        elif any(k in cls for k in ["shield", "tower", "greatshield"]):
            height = 1.2 if "tower" in cls or "great" in cls else 0.8
            triangles.extend([
                Triangle(Vec3(0, 0, 0), Vec3(1.0, 0, 0), Vec3(0.5, height, 0)),
                Triangle(Vec3(0, 0, 0), Vec3(1.0, 0, 0), Vec3(0.5, -0.3, 0)),
                Triangle(Vec3(0.5, height, 0), Vec3(1.0, 0, 0), Vec3(0.5, -0.3, 0)),
            ])
        else:
            triangles.extend([
                Triangle(Vec3(0, 0, 0), Vec3(0, length, 0), Vec3(0.2, 0, 0)),
                Triangle(Vec3(0, 0, 0), Vec3(0, length, 0), Vec3(-0.2, 0, 0)),
            ])
        
        scale = max(0.4, min(1.8, damage_scale))
        return SceneObject(
            triangles=triangles,
            shader_id="wireframe",
            position=Vec3(math.sin(t * 0.3) * 4, 2.0, math.cos(t * 0.3) * 4),
            rotation=Vec3(t * 0.5, t * 0.3, 0.0),
            scale=scale
        )
    
    def _create_enemy_dream(self, enemy: Dict, t: float) -> SceneObject:
        """Create a procedural visualization of an enemy."""
        name = enemy.get("name", "Unknown")
        enemy_type = enemy.get("type", "hollow")
        
        triangles = []
        size = 1.0 + 0.2 * math.sin(t * 0.7)
        
        # Basic humanoid shape
        # Torso
        triangles.extend([
            Triangle(Vec3(-0.5, 0, 0), Vec3(0.5, 0, 0), Vec3(0, 1.2, 0)),
            Triangle(Vec3(-0.5, 0, 0), Vec3(0.5, 0, 0), Vec3(0, 0, 0)),
            Triangle(Vec3(0, 1.2, 0), Vec3(0.5, 0, 0), Vec3(0, 0, 0)),
        ])
        # Head
        triangles.extend([
            Triangle(Vec3(-0.3, 1.2, 0), Vec3(0.3, 1.2, 0), Vec3(0, 1.8, 0)),
        ])
        # Arms
        arm_swing = math.sin(t * 2.0) * 0.3
        triangles.extend([
            Triangle(Vec3(-0.5, 0.8, 0), Vec3(-0.9, 0.5 + arm_swing, 0), Vec3(-0.5, 1.0, 0)),
            Triangle(Vec3(0.5, 0.8, 0), Vec3(0.9, 0.5 - arm_swing, 0), Vec3(0.5, 1.0, 0)),
        ])
        # Legs
        walk = math.sin(t * 3.0) * 0.2
        triangles.extend([
            Triangle(Vec3(-0.2, 0, 0), Vec3(-0.2, -1.2, walk), Vec3(0, 0, 0)),
            Triangle(Vec3(0.2, 0, 0), Vec3(0.2, -1.2, -walk), Vec3(0, 0, 0)),
        ])
        
        return SceneObject(
            triangles=triangles,
            shader_id="wireframe",
            position=Vec3(math.cos(t * 0.2) * 6, 0, math.sin(t * 0.2) * 6),
            rotation=Vec3(0.0, t * 0.2, 0.0),
            scale=float(size)
        )
    
    def _create_location_dream(self, location: Dict, t: float) -> SceneObject:
        """Create procedural environment from map data."""
        triangles = []
        data = location.get("data", {})
        
        # Ground plane with vertices displaced by audio
        grid = 8
        size = 15.0
        for i in range(grid):
            for j in range(grid):
                x0 = (i / grid - 0.5) * size
                z0 = (j / grid - 0.5) * size
                x1 = ((i + 1) / grid - 0.5) * size
                z1 = ((j + 1) / grid - 0.5) * size
                
                # Displacement from audio spectrum
                idx = (i * grid + j) % 64
                disp = self.audio_state.spectrum[idx] if idx < len(self.audio_state.spectrum) else 0.0
                y0 = disp * 2.0 + math.sin(x0 * 0.5 + t) * 0.3
                y1 = disp * 2.0 + math.sin(x1 * 0.5 + t) * 0.3
                y2 = disp * 2.0 + math.sin(z0 * 0.5 + t) * 0.3
                y3 = disp * 2.0 + math.sin(z1 * 0.5 + t) * 0.3
                
                triangles.append(Triangle(
                    Vec3(x0, y0, z0),
                    Vec3(x1, y1, z0),
                    Vec3(x0, y2, z1)
                ))
                triangles.append(Triangle(
                    Vec3(x1, y1, z0),
                    Vec3(x1, y3, z1),
                    Vec3(x0, y2, z1)
                ))
        
        # Bonfire pillars
        if data.get("bonfires"):
            for bf in data.get("bonfires", [])[:3]:
                pos = bf.get("position", [0, 0, 0])
                bx, by, bz = pos
                # Pillar
                triangles.extend([
                    Triangle(Vec3(bx - 0.3, by, bz), Vec3(bx + 0.3, by, bz), Vec3(bx, by + 2.0, bz)),
                    Triangle(Vec3(bx - 0.3, by, bz), Vec3(bx + 0.3, by, bz), Vec3(bx, by, bz + 0.3)),
                ])
        
        return SceneObject(
            triangles=triangles,
            shader_id="wireframe",
            position=Vec3(0.0, 0.0, 0.0),
            rotation=Vec3(0.0, t * 0.1, 0.0),
            scale=1.0
        )
    
    def _create_particle_burst(self, t: float) -> List[SceneObject]:
        """Create audio-reactive particles."""
        particles = []
        count = min(20, int(self.audio_state.energy * 30))
        
        for i in range(count):
            angle = (i / max(1, count)) * 2.0 * math.pi + t
            radius = 2.0 + self.audio_state.bass * 5.0
            x = math.cos(angle) * radius
            z = math.sin(angle) * radius
            y = self.audio_state.treble * 3.0 + math.sin(t * 2.0 + i) * 0.5
            
            # Small triangle particle
            size = 0.1 + self.audio_state.energy * 0.2
            triangles = [
                Triangle(Vec3(0, 0, 0), Vec3(size, size, 0), Vec3(-size, size, 0)),
            ]
            particles.append(SceneObject(
                triangles=triangles,
                shader_id="wireframe",
                position=Vec3(x, y, z),
                scale=1.0
            ))
        
        return particles
    
    def dream(self, t: float) -> Scene:
        """
        Generate a dream scene from current audio + script state.
        This is the main entry point — call every frame.
        """
        scene = Scene(background_color=(5, 3, 10))
        objects = []
        
        # Choose what to manifest based on audio energy
        if self.audio_state.beat and self.audio_state.energy > 0.5:
            # High energy: spawn enemies
            if self.script_state.enemies:
                idx = int(t * 0.5) % len(self.script_state.enemies)
                enemy = self.script_state.enemies[idx]
                obj = self._create_enemy_dream(enemy, t)
                objects.append(obj)
        
        if self.audio_state.mid > 0.3:
            # Mid frequencies: spawn weapons
            if self.script_state.weapons:
                idx = int(t * 0.3) % len(self.script_state.weapons)
                weapon = self.script_state.weapons[idx]
                obj = self._create_weapon_dream(weapon, t)
                objects.append(obj)
        
        # Always render current location
        if self.script_state.current_location:
            loc_obj = self._create_location_dream(self.script_state.current_location, t)
            objects.append(loc_obj)
        
        # Particles on beats
        if self.audio_state.beat:
            particles = self._create_particle_burst(t)
            objects.extend(particles)
        
        scene.objects.extend(objects)
        return scene


def create_dream_scene(audio_analyzer, time: float) -> Scene:
    """Convenience function for pipeline integration."""
    dream = DreamGenerator()
    dream.load_scripts()
    dream.update_audio(audio_analyzer, time)
    return dream.dream(time)
