"""Run the production node against real patched RadioLib with fake RF, clock and NVS."""
from pathlib import Path
import os
import hashlib
import json
import subprocess
import sys

project = Path(__file__).resolve().parents[1]
root = Path(sys.argv[1]).resolve()
for name, item in json.loads((project/'patches/radiolib-7.8.1-hashes.json').read_text()).items():
    assert hashlib.sha256((root/name).read_bytes()).hexdigest() == item['after'], 'Patch/version mismatch: '+name
build = Path('/tmp/codex/build/CrowPanelTelemetry/radio-tests')
build.mkdir(parents=True, exist_ok=True)
source = (project/'Workers.cpp').read_text()
node = source[source.index('class PersistentNode'):source.index('static void radioTask')]
(build/'PersistentNode-under-test.h').write_text(node)
start = source.index('        if (ready && node.isActivated() && offset == 10')
end = source.index('        if (!ready && DEV_EUI', start)
(build/'Radio-service-under-test.h').write_text(source[start:end])
files = [root/'src/Hal.cpp', root/'src/Module.cpp', root/'src/utils/Cryptography.cpp',
         root/'src/utils/Utils.cpp', root/'src/protocols/PhysicalLayer/PhysicalLayer.cpp']
files += sorted((root/'src/protocols/LoRaWAN').glob('*.cpp'))
command = ['c++', '-std=c++17', '-O1', '-ffunction-sections', '-fdata-sections',
           '-I'+str(root/'src'), '-I'+str(project), '-I'+str(build), str(project/'tests/radio.cpp')]
command += [str(path) for path in files]
command += ['-Wl,--gc-sections', '-o', str(build/'radio')]
subprocess.run(command, check=True, env=dict(os.environ, TMPDIR=str(build)))
subprocess.run([str(build/'radio')], check=True, timeout=30)
