import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import unittest
import subprocess

class TestTwilightElysiumDragoonScaffold(unittest.TestCase):
    def test_cmake_builds(self):
        result = subprocess.run(
            ["cmake", "--build", "build", "-j1"],
            cwd="/home/sin/Projects/twilight-elysium",
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, msg=result.stderr)

    def test_te_test_binary_runs(self):
        result = subprocess.run(
            ["/home/sin/Projects/twilight-elysium/build/te-test"],
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, msg=result.stderr)

    def test_new_headers_exist(self):
        root = Path("/home/sin/Projects/twilight-elysium/src")
        required = [
            "game/legend_of_dragoon/character/character.h",
            "game/legend_of_dragoon/combat/combat.h",
            "game/legend_of_dragoon/world/world.h",
            "game/legend_of_dragoon/stage/stage.h",
            "game/legend_of_dragoon/legend_of_dragoon.h",
            "platform/runtime.h",
            "platform/local_runtime.h",
            "platform/satalight_runtime.h",
        ]
        for rel in required:
            self.assertTrue((root / rel).exists(), msg=f"missing {rel}")

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
