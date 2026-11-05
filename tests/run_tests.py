# Runs every test of this folder: python tests/run_tests.py
import glob, os, subprocess, sys

folder = os.path.dirname(os.path.abspath(__file__))
failed = 0
for test in sorted(glob.glob(os.path.join(folder, 'test_*.py'))):
    result = subprocess.run([sys.executable, '-B', '-I', test], capture_output=True, text=True)
    ok = result.returncode == 0
    print(('PASS ' if ok else 'FAIL ') + os.path.basename(test))
    if not ok:
        failed += 1
        print(result.stdout + result.stderr)
sys.exit(1 if failed else 0)
