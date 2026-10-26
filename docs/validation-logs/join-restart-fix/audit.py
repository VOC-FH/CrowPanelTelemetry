from pathlib import Path
import hashlib,json,subprocess
r=Path.cwd();t=Path('/tmp/codex/build/CrowPanelTelemetry/join-restart-fix');base=json.loads((t/'baseline.json').read_text());h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
protected=['Config.h','App.h','Application.cpp','Ui.cpp','Telemetry.h','ttn-decoder.js','tests/reference-packet.json','docs/source-photo.png','docs/wiring.svg','docs/wiring.png','docs/radio-nets.json','docs/ui-preview.svg','docs/ui-preview.png','artifacts/CrowPanelTelemetry-delivery.zip','patches/radiolib-7.8.1.patch','patches/radiolib-7.8.1-hashes.json','patches/jpegdec-1.8.4.patch','patches/jpegdec-1.8.4-hashes.json','.clang-format','.clang-format-ignore','.clang-tidy','.clangd']
protected += [p for p in base['hashes'] if p.startswith('docs/review/') and p!='docs/review/README.md']
for p in protected:assert h(r/p)==base['hashes'][p],p
sizes={}
for name in ['blank-prev3','enabled-prev3','enabled-postv3']:
 for source in list(r.glob('*.cpp'))+list(r.glob('*.h')):
  data=(t/name/'sketch'/source.name).read_bytes()
  if data.startswith(b'#line 1 '):data=data.split(b'\n',1)[1]
  assert data==source.read_bytes(),(name,source)
 log=(t/(name+'.log')).read_text()
 for dep in ['RadioLib','JPEGDEC','INA226','TinyGPSPlus','Arduino_GFX','audio-driver']:assert str(t/'deps'/dep) in log
 nm='/home/voc/.arduino15/packages/esp32/tools/esp-rv32/2601/bin/riscv32-esp-elf-nm'
 symbols=subprocess.check_output([nm,'-C',str(t/name/'CrowPanelTelemetry.ino.elf')],text=True)
 for forbidden in ['beforeUplink','afterUplink','SDMMCFS::begin','SDMMCFS::end']:assert forbidden not in symbols
 if name!='blank-prev3':assert 'PersistentNode::applicationDownlink' in symbols
 import re
 sizes[name]={'flash':int(re.search(r'Sketch uses (\d+)',log)[1]),'ram':int(re.search(r'Global variables use (\d+)',log)[1])}
result={'baseline_commit':base['head'],'protected_hashes':{p:h(r/p) for p in protected},'sizes':sizes,'build_sources':'canonical C++/headers match all configurations except Arduino initial #line','library_selection':'all six exact external libraries selected under task deps','symbols':'downlink callback present in enabled; no reservation hooks or SDMMCFS begin/end'}
(t/'audit.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS: protected files, prior review records, all three build sources/libraries/symbols')
counts={n:{'added':0,'removed':0} for n in ['application','tests','tooling','dependency_source']}
for line in subprocess.check_output(['git','diff','--numstat'],text=True).splitlines():
 a,d,p=line.split('\t');category='tests' if p.startswith('tests/') else 'tooling' if p.startswith('tools/') else 'application' if p.endswith(('.ino','.cpp','.h')) or p=='ttn-decoder.js' else None
 if category:counts[category]['added']+=int(a);counts[category]['removed']+=int(d)
(t/'accounting.json').write_text(json.dumps({'baseline_commit':base['head'],'lines':counts,'scope':'Only session-restart join-budget correction; prior review records preserved.'},indent=2)+'\n');print(counts)
