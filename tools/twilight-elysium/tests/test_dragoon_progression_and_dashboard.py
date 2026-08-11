import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import unittest, subprocess
import json

class TestDragoonExpansions(unittest.TestCase):
    def test_cmake_builds(self):
        result = subprocess.run(["cmake", "--build", "build", "-j1"], cwd="/home/sin/Projects/twilight-elysium", capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, msg=result.stderr)

    def test_progression_headers_exist(self):
        root = Path("/home/sin/Projects/twilight-elysium/src")
        required = [
            "game/legend_of_dragoon/progression/progression.h",
            "game/legend_of_dragoon/menu/menu.h",
            "renderer/material/material.h",
            "platform/distribution/distributor.h",
        ]
        for rel in required:
            self.assertTrue((root / rel).exists(), msg=f"missing {rel}")

    def test_dashboard_page_exists(self):
        p = Path("/home/sin/Projects/space-edge-network/dashboard/src/pages/DragoonRuntime.tsx")
        self.assertTrue(p.exists(), msg=f"missing dashboard page: {p}")

    def test_dashboard_api_exists(self):
        p = Path("/home/sin/Projects/space-edge-network/dashboard/src/services/api/dragoon.ts")
        self.assertTrue(p.exists(), msg=f"missing dashboard API: {p}")

    def test_no_external_ai_provider_refs(self):
        root = Path("/home/sin/Projects/twilight-elysium/src")
        bad = ["openai", "firecrawl", "fal ", "elevenlabs", "minimax", "xai", "mistral", "gemini", "replit", "cohere", "anthropic"]
        for path in root.rglob("*"):
            if path.suffix.lower() in {".cpp", ".h", ".hpp", ".inl"}:
                text = path.read_text(encoding="utf-8", errors="ignore").lower()
                for token in bad:
                    self.assertNotIn(token, text, msg=f"suspect external provider ref in {path}: {token}")

if __name__ == "__main__":
    unittest.main()
