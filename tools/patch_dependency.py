"""Apply a reviewed, hash-checked patch to an exact isolated dependency tree."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

project = Path(__file__).resolve().parents[1]
dependency = sys.argv[1]
if dependency not in ('radiolib-7.8.1', 'jpegdec-1.8.4'):
    raise SystemExit('Select radiolib-7.8.1 or jpegdec-1.8.4')
root = Path(sys.argv[2]).resolve()
expected = json.loads((project/('patches/'+dependency+'-hashes.json')).read_text())
actual = {name: hashlib.sha256((root/name).read_bytes()).hexdigest() for name in expected}
if all(actual[name] == item['after'] for name, item in expected.items()):
    print('PASS: '+dependency+' patch already applied')
elif all(actual[name] == item['before'] for name, item in expected.items()):
    subprocess.run(['patch', '--batch', '-p1', '-i', str(project/('patches/'+dependency+'.patch'))], cwd=root, check=True)
    for name, item in expected.items():
        assert hashlib.sha256((root/name).read_bytes()).hexdigest() == item['after'], name
    print('PASS: exact '+dependency+' patch applied and verified')
else:
    raise SystemExit('Refusing unknown/mixed '+dependency+' sources; use the exact unmodified dependency')
