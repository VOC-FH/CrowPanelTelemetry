"""Exercise verbatim production GPS status with the real TinyGPSPlus parser."""
from pathlib import Path
import os
import subprocess
import sys
project = Path(__file__).resolve().parents[1]
dep = Path(sys.argv[1]).resolve()
build = Path(os.environ.get('BUILD', '/tmp/codex/build/CrowPanelTelemetry/deep-fix'))
build.mkdir(parents=True, exist_ok=True)
source = (project/'Application.cpp').read_text()
start = source.index('    uint32_t now =', source.index('void loopApplication'))
end = source.index('    if (static_cast<int32_t>(now - nextSample)', start)
(build/'Gps-under-test.h').write_text(source[start:end])
command = ['c++', '-std=c++17', '-Wall', '-Wextra', '-DARDUINO=10805',
    '-I'+str(project/'tests/arduino-stubs'), '-I'+str(dep/'src'), '-I'+str(build)]
for source, output, project_owned in [
    (project/'tests/gps.cpp', build/'gps-test.o', True),
    (dep/'src/TinyGPS++.cpp', build/'tinygps.o', False)]:
    subprocess.run(command + (['-Werror'] if project_owned else []) + ['-c', str(source), '-o', str(output)],
        check=True, env=dict(os.environ, TMPDIR=str(build)))
subprocess.run(['c++', str(build/'gps-test.o'), str(build/'tinygps.o'), '-o', str(build/'gps')], check=True,
    env=dict(os.environ, TMPDIR=str(build)))
subprocess.run([str(build/'gps')], cwd=build, check=True)
