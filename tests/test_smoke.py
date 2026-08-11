#!/usr/bin/env python3
"""Smoke test: launcher boots and loads game data + difficulties."""
import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]

def test_difficulties_present() -> None:
    diff = json.loads((REPO / "Content" / "Data" / "Difficulties.json").read_text())
    assert "casual" in diff["profiles"] and "legend" in diff["profiles"]
    assert diff["profiles"]["casual"]["player"]["health"] == 150
    assert diff["profiles"]["legend"]["player"]["health"] == 100

def test_launcher_boots() -> None:
    for diff in ("casual", "legend"):
        r = subprocess.run([sys.executable, str(REPO / "tools" / "hl1_dream.py"), diff],
                           capture_output=True, text=True)
        assert r.returncode == 0, r.stderr
        assert f"Difficulty: {diff}" in r.stdout

if __name__ == "__main__":
    test_difficulties_present()
    test_launcher_boots()
    print("half-life-elysium smoke tests pass")
