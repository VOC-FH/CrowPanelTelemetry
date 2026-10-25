"""Compile the production Connect decision and both WiFi status expressions."""
from pathlib import Path
import os
import re
import subprocess
project = Path(__file__).resolve().parents[1]
build = Path(os.environ.get('BUILD', '/tmp/codex/build/CrowPanelTelemetry/deep-fix'))
build.mkdir(parents=True, exist_ok=True)
source = (project/'Ui.cpp').read_text()
app = (project/'App.h').read_text()
start = app.index('struct WifiCommand')
end = app.index('enum class MediaCommand', start)
(build/'WifiCommand-under-test.h').write_text(app[start:end])
helper = ''
if 'static const char* wifiInputError()' in source:
    start = source.index('static const char* wifiInputError()')
    end = source.index('static void drawKeyboard()', start)
    helper = source[start:end]
start = source.index('        } else if (Rect{680, 510, 150, 52}')
condition = source[source.index('            if (', start):].splitlines()[0].strip()[4:-3]
(build/'WifiForm-under-test.h').write_text(helper+'static bool connectAllowed() { return '+condition+'; }\n')
status = re.findall(r'^\s*lines\(32, 225,.*;', source, re.MULTILINE)
assert len(status) == 2
(build/'WifiStatus-under-test.h').write_text('\n'.join(status)+'\n')
subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
    '-I'+str(project/'tests/arduino-stubs'), '-I'+str(build), str(project/'tests/wifi-form.cpp'),
    '-o', str(build/'wifi-form')], check=True, env=dict(os.environ, TMPDIR=str(build)))
subprocess.run([str(build/'wifi-form')], cwd=build, check=True)
print('PASS: real Connect decision/status: empty SSID, short/open/8-character/64-hex/non-hex passwords and periodic updates')
