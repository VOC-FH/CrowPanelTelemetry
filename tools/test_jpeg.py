"""Check the production JPEG admission guard against actual baseline/progressive files."""
from pathlib import Path
import os
import subprocess
import sys
project = Path(__file__).resolve().parents[1]
dep = Path(sys.argv[1]).resolve()
build = Path(os.environ.get('BUILD', '/tmp/codex/build/CrowPanelTelemetry/deep-fix'))
source = (project/'Workers.cpp').read_text()
start = source.index('            ok = event.width <=')
end = source.index('            event.pixels =', start)
(build/'ImagePolicy-under-test.h').write_text(source[start:end])
subprocess.run(['c++', '-std=c++17', '-D__LINUX__', '-I'+str(dep/'src'), '-I'+str(build),
    str(project/'tests/jpeg.cpp'), str(dep/'src/JPEGDEC.cpp'), '-o', str(build/'jpeg')],
    check=True, env=dict(os.environ, TMPDIR=str(build)))
subprocess.run([str(build/'jpeg'), str(project/'tests/fixtures/baseline.jpg'),
    str(project/'tests/fixtures/progressive.jpg')], cwd=build, check=True)
print('PASS: real JPEGDEC with production guard: baseline decoded; progressive rejected')
