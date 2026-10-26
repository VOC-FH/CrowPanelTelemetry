"""Exercise the production amplifier startup and playback sequence with transport doubles."""
from pathlib import Path
import os
import subprocess

project = Path(__file__).resolve().parents[1]
build = Path(os.environ.get('BUILD', '/tmp/codex/build/CrowPanelTelemetry/local'))
build.mkdir(parents=True, exist_ok=True)
setup = (project/'Application.cpp').read_text()
start = setup.index('    pinMode(Pins::Amp', setup.index('void setupApplication()'))
end = setup.index('    Serial.begin', start)
assert start < setup.index('codec.begin'), 'Amplifier shutdown must precede codec initialization'
(build/'Amplifier-start-under-test.h').write_text(setup[start:end])
source = (project/'Workers.cpp').read_text()
start = source.index('        if (ok && audioReady)', source.index('static void audioTask'))
end = source.index('        free(sound.bytes);', start)
(build/'Audio-under-test.h').write_text(source[start:end])
subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I'+str(project),
    '-I'+str(build), str(project/'tests/audio.cpp'), '-o', str(build/'audio')],
    check=True, env=dict(os.environ, TMPDIR=str(build)))
subprocess.run([str(build/'audio')], check=True)
