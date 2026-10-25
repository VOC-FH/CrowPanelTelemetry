"""Check the production JPEG admission guard against actual baseline/progressive files."""
from pathlib import Path
import os
import hashlib
import json
import subprocess
import sys
project = Path(__file__).resolve().parents[1]
dep = Path(sys.argv[1]).resolve()
build = Path(os.environ.get('BUILD', '/tmp/codex/build/CrowPanelTelemetry/deep-fix'))
for name, item in json.loads((project/'patches/jpegdec-1.8.4-hashes.json').read_text()).items():
    assert hashlib.sha256((dep/name).read_bytes()).hexdigest() == item['after'], 'Patch/version mismatch: '+name
source = (project/'Workers.cpp').read_text()
start = source.index('            ok = event.width <=')
end = source.index('            event.pixels =', start)
(build/'ImagePolicy-under-test.h').write_text(source[start:end])
subprocess.run(['c++', '-std=c++17', '-D__LINUX__', '-O1', '-g',
    '-fsanitize=address,bounds', '-fno-sanitize-recover=all', '-I'+str(dep/'src'), '-I'+str(build),
    str(project/'tests/jpeg.cpp'), str(dep/'src/JPEGDEC.cpp'), '-o', str(build/'jpeg')],
    check=True, env=dict(os.environ, TMPDIR=str(build)))
baseline = (project/'tests/fixtures/baseline.jpg').read_bytes()
sof = baseline.index(b'\xff\xc0')
length = int.from_bytes(baseline[sof+2:sof+4], 'big')
header = baseline[sof+4:sof+2+length]
cases = []
for count in [0, 5, 6, 7, 8, 9, 10, 11, 255]:
    body = header[:5] + bytes([count]) + bytes([1, 0x11, 0])*count
    cases.append((f'components-{count}', body, len(body)+2))
for declared in [2, 7, 8, 10, length-1, length+1, 65535]:
    cases.append((f'length-{declared}', header, declared))
invalid = []
for name, body, declared in cases:
    path = build/(name+'.jpg')
    path.write_bytes(baseline[:sof] + b'\xff\xc0' + declared.to_bytes(2, 'big') +
                     body + baseline[sof+2+length:])
    invalid.append(str(path))
subprocess.run([str(build/'jpeg'), str(project/'tests/fixtures/baseline.jpg'),
    str(project/'tests/fixtures/progressive.jpg')] + invalid, cwd=build, check=True)
print('PASS: real JPEGDEC with production guard: baseline decoded; progressive and invalid SOF rejected under ASan/bounds')
