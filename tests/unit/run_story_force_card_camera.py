"""Run the canonical camera test from disposable source-shaped values."""
import struct
import sys
from pathlib import Path
from subprocess import run
from tempfile import TemporaryDirectory


def main():
    with TemporaryDirectory(prefix="gf-card-camera-") as directory:
        root = Path(directory)
        camera = (bytes(12) + struct.pack(">3f", 0, 0, 0) +
                  struct.pack(">3f", 0, 0, -100) +
                  struct.pack(">3f", 0, 100, 0) + bytes(36))
        values = struct.pack(">3f", 0, -100, 100)
        (root / "camera.bin").write_bytes(camera)
        (root / "values.bin").write_bytes(values)
        return run([Path(sys.argv[1]).resolve(strict=True),
                    str(root / "camera.bin"), str(root / "values.bin")],
                   timeout=30).returncode


if __name__ == "__main__":
    sys.exit(main())
