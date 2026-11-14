"""CTest wrapper: make one exclusive empty root, retain it on success/failure."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv)!=2:
    raise SystemExit('expected one owned test executable')
base=os.environ.get('GF_TEST_TMPDIR')
root=Path(tempfile.mkdtemp(prefix='gf-account-save-test-',dir=base))
print('Owned fixture retained at',root,flush=True)
raise SystemExit(subprocess.run([sys.argv[1],str(root)],check=False).returncode)
