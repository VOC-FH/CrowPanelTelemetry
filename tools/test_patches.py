"""Check exact-source apply, idempotence and modified-source refusal in isolated trees."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys

project = Path(__file__).resolve().parents[1]
deps = Path(sys.argv[1]).resolve().parent
build = Path(os.environ.get('BUILD', '/tmp/codex/build/CrowPanelTelemetry/rereview-fix'))
for dependency, folder in [('radiolib-7.8.1', 'RadioLib'), ('jpegdec-1.8.4', 'JPEGDEC')]:
    expected = json.loads((project/('patches/'+dependency+'-hashes.json')).read_text())
    check = build/'patch-check'/folder
    if check.exists():
        shutil.rmtree(check)
    for name, item in expected.items():
        data = (deps/folder/name).read_bytes()
        assert hashlib.sha256(data).hexdigest() == item['after'], name
        (check/name).parent.mkdir(parents=True, exist_ok=True)
        (check/name).write_bytes(data)
    subprocess.run(['patch', '--batch', '-R', '-p1', '-i',
                    str(project/('patches/'+dependency+'.patch'))], cwd=check, check=True)
    for name, item in expected.items():
        assert hashlib.sha256((check/name).read_bytes()).hexdigest() == item['before'], name
    command = ['python3', str(project/'tools/patch_dependency.py'), dependency, str(check)]
    subprocess.run(command, check=True)
    subprocess.run(command, check=True)
    changed = check/next(iter(expected))
    changed.write_bytes(changed.read_bytes()+b'\n')
    hashes = {name: hashlib.sha256((check/name).read_bytes()).hexdigest() for name in expected}
    assert subprocess.run(command, capture_output=True).returncode != 0
    assert hashes == {name: hashlib.sha256((check/name).read_bytes()).hexdigest() for name in expected}
print('PASS: both dependency patches: exact-source apply, idempotence, modified-source refusal')
