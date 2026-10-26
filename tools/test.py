"""Run independent checks even when another check fails."""
from pathlib import Path
import json
import os
import subprocess

root = Path(__file__).resolve().parents[1]
build = Path(os.environ.get('BUILD', '/tmp/codex/build/CrowPanelTelemetry/local'))
(build/'tmp').mkdir(parents=True, exist_ok=True)
env = dict(os.environ, TMPDIR=str(build/'tmp'))
failures = []
compile_result = subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I.',
    'tests/codec.cpp', '-o', str(build/'codec')], cwd=root, env=env)
if compile_result.returncode == 0:
    encoded = subprocess.run([str(build/'codec')], cwd=root, capture_output=True, text=True)
    print(encoded.stderr, end='')
    if encoded.returncode != 0:
        failures.append('C++ codec assertions')
    else:
        expected = json.loads((root/'tests/reference-packet.json').read_text())
        if json.loads(encoded.stdout) != expected:
            failures.append('C++ wire-format reference vector')
else:
    failures.append('C++ codec compile')
sd_compile = subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
    '-Itests/sd-stubs', '-I.', 'tests/sd-storage.cpp', '-o', str(build/'sd-storage')], cwd=root, env=env)
if sd_compile.returncode == 0:
    if subprocess.run([str(build/'sd-storage')], cwd=root, env=env).returncode != 0:
        failures.append('SD resource lifecycle')
else:
    failures.append('SD lifecycle test compile')
for name, command in [('TTN decoder' , ['node', 'tests/decoder.js', 'tests/reference-packet.json']),
                      ('Airtime', ['python3', 'tools/airtime.py']),
                      ('WiFi input/status', ['python3', 'tools/test_wifi_form.py']),
                      ('Amplifier playback', ['python3', 'tools/test_audio.py']),
                      ('WiFi worker status', ['python3', 'tools/test_wifi_worker.py'])]:
    result = subprocess.run(command, cwd=root, env=env)
    if result.returncode != 0:
        failures.append(name)
radio = os.environ.get('RADIOLIB_DIR')
if radio:
    for name, command in [
        ('Dependency patch integrity', ['python3', 'tools/test_patches.py', radio]),
        ('Real TinyGPSPlus status', ['python3', 'tools/test_gps.py', str(Path(radio).parent/'TinyGPSPlus')]),
        ('Real JPEGDEC admission', ['python3', 'tools/test_jpeg.py', str(Path(radio).parent/'JPEGDEC')]),
        ('Real INA226 acquisition', ['python3', 'tools/test_sensor.py', str(Path(radio).parent/'INA226')]),
        ('Real RadioLib integration', ['python3', 'tools/test_radio.py', radio])]:
        result = subprocess.run(command, cwd=root, env=env)
        if result.returncode != 0:
            failures.append(name)
else:
    failures.append('Real RadioLib integration: set RADIOLIB_DIR to patched 7.8.1')
print('FAILED: '  + ', '.join(failures) if failures else 'PASS: all independent checks')
raise SystemExit(1 if failures else 0)
