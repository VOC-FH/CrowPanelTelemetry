"""Exercise the production acquisition block with real INA226 and a Wire transport double."""
from pathlib import Path
import os
import subprocess
import sys
project = Path(__file__).resolve().parents[1]
dep = Path(sys.argv[1]).resolve()
build = Path(os.environ.get('BUILD', '/tmp/codex/build/CrowPanelTelemetry/deep-fix'))
build.mkdir(parents=True, exist_ok=True)
source = (project/'Application.cpp').read_text()
start = source.index('            model.sensor =', source.index('bool currentSlot'))
end = source.index('            if (model.collected == 0)', start)
(build/'Sensor-under-test.h').write_text(source[start:end])
failures = []
for shunt in ['0.1f', '0.0f']:
    binary = build/('sensor-'+shunt)
    command = ['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-DINA_SHUNT_OHMS='+shunt,
        '-I'+str(project/'tests/arduino-stubs'), '-I'+str(dep), '-I'+str(project), '-I'+str(build),
        str(project/'tests/sensor.cpp'), str(dep/'INA226.cpp'), '-o', str(binary)]
    result = subprocess.run(command, env=dict(os.environ, TMPDIR=str(build)))
    if result.returncode == 0:
        result = subprocess.run([str(binary)], cwd=build)
    if result.returncode != 0:
        failures.append(shunt)
raise SystemExit(1 if failures else 0)
