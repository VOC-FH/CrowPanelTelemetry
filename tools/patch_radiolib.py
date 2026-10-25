"""Apply only the reviewed patch to an exact RadioLib 7.8.1 dependency directory."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

project = Path(__file__).resolve().parents[1]
root = Path(sys.argv[1]).resolve()
expected = json.loads((project/'patches/radiolib-7.8.1-hashes.json').read_text())
actual = {name: hashlib.sha256((root/name).read_bytes()).hexdigest() for name in expected}
if all(actual[name] == item['after'] for name, item in expected.items()):
    print('PASS: RadioLib patch already applied')
elif all(actual[name] == item['before'] for name, item in expected.items()):
    subprocess.run(['patch', '--batch', '-p1', '-i', str(project/'patches/radiolib-7.8.1.patch')], cwd=root, check=True)
    for name, item in expected.items():
        assert hashlib.sha256((root/name).read_bytes()).hexdigest() == item['after'], name
    print('PASS: exact RadioLib 7.8.1 patch applied and verified')
else:
    raise SystemExit('Refusing unknown/mixed RadioLib sources; use an unmodified 7.8.1 dependency')
