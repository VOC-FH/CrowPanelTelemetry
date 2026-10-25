"""Exercise the verbatim WiFi worker iteration at its status/Preferences boundary."""
from pathlib import Path
import os
import subprocess

project = Path(__file__).resolve().parents[1]
build = Path(os.environ.get('BUILD', '/tmp/codex/build/CrowPanelTelemetry/rereview-fix'))
build.mkdir(parents=True, exist_ok=True)
source = (project/'Workers.cpp').read_text()
start = source.index('        if (xQueueReceive(wifiCommands')
end = source.index('    }\n}\n\nstatic int jpegDraw', start)
(build/'WifiWorker-under-test.h').write_text(source[start:end])
command = ['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
           '-I'+str(project/'tests/arduino-stubs'), '-I'+str(build),
           str(project/'tests/wifi-worker.cpp'), '-o', str(build/'wifi-worker')]
subprocess.run(command, check=True, env=dict(os.environ, TMPDIR=str(build)))
subprocess.run([str(build/'wifi-worker')], check=True, cwd=build)
print('PASS: verbatim WiFi worker retains credential-save error in final status')
