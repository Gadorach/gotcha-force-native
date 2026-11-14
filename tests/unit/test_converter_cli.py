from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


class ConverterCliTests(unittest.TestCase):
    def test_owned_banner_no_input_mutation_or_output_overwrite(self):
        converter = Path(__file__).resolve().parents[2] / "tools/convert/texture.py"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            banner = root / "owned.bnr"
            source = b"BNR1" + bytes(0x1960 - 4)
            banner.write_bytes(source)
            output = root / "out/image.png"
            first = subprocess.run([sys.executable, "-B", str(converter), str(banner), str(output)],
                                   capture_output=True, text=True)
            self.assertEqual(first.returncode, 0, first.stderr)
            pixels = output.read_bytes()
            self.assertTrue(pixels.startswith(b"\x89PNG\r\n\x1a\n"))
            second = subprocess.run([sys.executable, "-B", str(converter), str(banner), str(output)],
                                    capture_output=True, text=True)
            self.assertNotEqual(second.returncode, 0)
            self.assertEqual(output.read_bytes(), pixels)
            self.assertEqual(banner.read_bytes(), source)


if __name__ == "__main__":
    unittest.main()
