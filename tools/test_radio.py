"""Run the production node against real patched RadioLib with fake RF, clock and NVS."""
from pathlib import Path
import os
import hashlib
import json
import subprocess
import sys
import contextlib
import io
import runpy

project = Path(__file__).resolve().parents[1]
root = Path(sys.argv[1]).resolve()
for name, item in json.loads((project/'patches/radiolib-7.8.1-hashes.json').read_text()).items():
    assert hashlib.sha256((root/name).read_bytes()).hexdigest() == item['after'], 'Patch/version mismatch: '+name
build = Path(os.environ.get('BUILD', '/tmp/codex/build/CrowPanelTelemetry/radio-tests'))
build.mkdir(parents=True, exist_ok=True)
with contextlib.redirect_stdout(io.StringIO()):
    airtime = runpy.run_path(str(project/'tools/airtime.py'))['airtime']
rows = [', '.join(str(round(airtime(length-13, sf)*1000000)) for length in range(256))
        for sf in range(7, 13)]
(build/'Airtime-reference.h').write_text('static const unsigned referenceToA[6][256] = {\n' +
    ',\n'.join('{'+row+'}' for row in rows) + '\n};\n')
source = (project/'Workers.cpp').read_text()
node = source[source.index('class PersistentNode'):source.index('static void radioTask')]
(build/'PersistentNode-under-test.h').write_text(node)
start = source.index('        if (ready && node.isActivated() && offset == 10')
end = source.index('        if (!ready && DEV_EUI', start)
(build/'Radio-service-under-test.h').write_text(source[start:end])
start = source.index('    if (ready) {', source.index('Fill DEV_EUI'))
end = source.index('    Batch batch;', start)
(build/'Radio-init-under-test.h').write_text(source[start:end])
start = source.index('        if (ready && node.canTransmit() && !node.isActivated())')
end = source.index('        if (ready && node.isActivated() && offset == 10', start)
(build/'Radio-join-under-test.h').write_text(source[start:end])
start = source.index('        if (!ready && DEV_EUI')
end = source.index('        vTaskDelay(', start)
(build/'Radio-stop-under-test.h').write_text(source[start:end])

files = [root/'src/Hal.cpp', root/'src/Module.cpp', root/'src/utils/Cryptography.cpp',
         root/'src/utils/Utils.cpp', root/'src/protocols/PhysicalLayer/PhysicalLayer.cpp']
files += sorted((root/'src/protocols/LoRaWAN').glob('*.cpp'))
command = ['c++', '-std=c++17', '-O1', '-DDEV_EUI=1ULL', '-ffunction-sections', '-fdata-sections',
           '-I'+str(root/'src'), '-I'+str(project), '-I'+str(build), str(project/'tests/radio.cpp')]
command += [str(path) for path in files]
command += ['-Wl,--gc-sections', '-o', str(build/'radio')]
subprocess.run(command, check=True, env=dict(os.environ, TMPDIR=str(build)))
subprocess.run([str(build/'radio')], check=True, timeout=30, cwd=build)
