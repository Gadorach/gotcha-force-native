"""Give a test an exclusive nonexistent output filename within owned temp storage."""
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit("expected one test executable")
root = Path(tempfile.mkdtemp(prefix="gf-native-output-test-"))
print("Owned test output retained at", root, flush=True)
raise SystemExit(subprocess.run([sys.argv[1], str(root / "output.txt")], check=False).returncode)
