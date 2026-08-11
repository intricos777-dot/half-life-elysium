#!/usr/bin/env python3
"""
HL1 Elysium Dream Launcher
============================
Boots the Twilight Elysium dream engine with Half-Life-themed data and scripts.
Supports casual/legend difficulties.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
ENGINE_TOOLS = REPO_ROOT / "tools" / "twilight-elysium" / "tools"
GAME_CONTENT = REPO_ROOT / "Content"

if str(ENGINE_TOOLS) not in sys.path:
    sys.path.insert(0, str(ENGINE_TOOLS))

from dream import ContentDreamEngine
from dream_engine import DreamGenerator


class HL1DreamEngine(ContentDreamEngine):
    """Dream engine preset for the Half-Life theme."""

    def __init__(self, difficulty: str = "casual"):
        super().__init__()
        self.dream_generator = DreamGenerator()
        self.dream_generator.script_state.weapons = []
        self.dream_generator.script_state.enemies = []
        self.dream_generator.script_state.locations = []
        self.dream_generator.load_scripts()
        if not self.dream_generator.script_state.weapons and not self.dream_generator.script_state.enemies:
            print("[HL1] No script data loaded; Content/Data JSONs are optional.")
        self._load_game_data()
        self.difficulty = self._load_difficulty(difficulty)

    def _load_difficulty(self, name: str) -> dict:
        f = GAME_CONTENT / "Data" / "Difficulties.json"
        if not f.exists():
            return {"profile": name, "settings": {}}
        data = json.loads(f.read_text())
        profiles = data.get("profiles", {})
        name = name if name in profiles else data.get("default", "casual")
        return {"profile": name, "settings": profiles.get(name, {})}


    def _load_game_data(self) -> None:
        """Load this game's own Content/Data JSONs into the dream state."""
        data_dir = GAME_CONTENT / "Data"
        for f in sorted(data_dir.glob("*.json")):
            if f.name == "Difficulties.json":
                continue
            try:
                items = json.loads(f.read_text())
            except Exception:
                continue
            if not isinstance(items, list):
                continue
            low = f.stem.lower()
            if "weapon" in low:
                self.dream_generator.script_state.weapons.extend(items)
            elif "enem" in low or "class" in low or "mode" in low:
                self.dream_generator.script_state.enemies.extend(items)
            elif "map" in low:
                self.dream_generator.script_state.locations.extend(
                    {"name": m.get("name", f.stem), "data": m} for m in items
                )



def main() -> int:
    print("[HL1] Starting Half-Life Elysium Dream Engine...")
    difficulty = "casual"
    if len(sys.argv) > 1 and sys.argv[1] in ("casual", "legend"):
        difficulty = sys.argv[1]
    engine = HL1DreamEngine(difficulty=difficulty)
    diff = engine.difficulty
    print(f"[HL1] Difficulty: {diff['profile']}")
    if diff["settings"]:
        print(f"[HL1]   player  -> {diff['settings'].get('player')}")
        print(f"[HL1]   combat  -> {diff['settings'].get('combat')}")
    scene = engine.dream()
    print(f"[HL1] Generated scene with {len(scene.objects)} objects")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
