import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import unittest, subprocess

class TestDragoonFullContentAndRuntime(unittest.TestCase):
    def test_cmake_builds(self):
        result = subprocess.run(["cmake", "--build", "build", "-j1"], cwd="/home/sin/Projects/twilight-elysium", capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, msg=result.stderr)

    def test_te_test_binary_runs(self):
        result = subprocess.run(["/home/sin/Projects/twilight-elysium/build/te-test", "--auto"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, msg=result.stderr)

    def test_dragoon_data_headers_exist(self):
        root = Path("/home/sin/Projects/twilight-elysium/src")
        required = [
            "game/legend_of_dragoon/data/dragoon_data.h",
            "game/legend_of_dragoon/battle/battle.h",
            "renderer/camera/camera.h",
            "renderer/shader/shader_pipeline.h",
            "platform/offload/quantum_offload.h",
        ]
        for rel in required:
            self.assertTrue((root / rel).exists(), msg=f"missing {rel}")

    def test_dashboard_offload_page_exists(self):
        p = Path("/home/sin/Projects/space-edge-network/dashboard/src/pages/DragoonOffload.tsx")
        self.assertTrue(p.exists(), msg=f"missing dashboard page: {p}")

    def test_dashboard_dragoon_api_has_offload(self):
        p = Path("/home/sin/Projects/space-edge-network/dashboard/src/services/api/dragoon.ts")
        text = p.read_text(encoding="utf-8")
        self.assertIn("offload", text.lower())

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
