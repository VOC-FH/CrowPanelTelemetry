from pathlib import Path
import hashlib,json,subprocess,difflib,re,base64
r=Path.cwd(); t=Path('/tmp/codex/build/CrowPanelTelemetry/protocol-parser-fixes'); base=json.loads((t/'baseline.json').read_text())
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
protected=['Config.h','Application.cpp','Ui.cpp','Telemetry.h','ttn-decoder.js','tests/reference-packet.json','docs/source-photo.png','docs/ui-preview.svg','docs/ui-preview.png','docs/wiring.svg','docs/wiring.png','docs/radio-nets.json','.clang-format','.clang-format-ignore','.clang-tidy','.clangd','artifacts/CrowPanelTelemetry-delivery.zip']
protected += [p for p in base['hashes'] if p.startswith('docs/review/') and p!='docs/review/README.md']
for p in protected: assert h(r/p)==base['hashes'][p],p
for config in ['blank-prev3','enabled-prev3','enabled-postv3']:
 for p in list(r.glob('*.cpp'))+list(r.glob('*.h')):
  copy=t/config/'sketch'/p.name
  data=copy.read_bytes()
  if data.startswith(b'#line 1 '): data=data.split(b'\n',1)[1]
  assert data==p.read_bytes(),str(copy)
 log=(t/(config+'.log')).read_text()
 for dep in ['RadioLib','JPEGDEC','INA226','TinyGPSPlus','Arduino_GFX','audio-driver']:
  assert str(t/'deps'/dep) in log,(config,dep)
 nm=Path('/home/voc/.arduino15/packages/esp32/tools/esp-rv32/2601/bin/riscv32-esp-elf-nm')
 symbols=subprocess.check_output([str(nm),'-C',str(t/config/'CrowPanelTelemetry.ino.elf')],text=True)
 assert 'beforeUplink' not in symbols and 'afterUplink' not in symbols and 'SDMMCFS::begin' not in symbols and 'SDMMCFS::end' not in symbols
 if config!='blank-prev3': assert 'PersistentNode::applicationDownlink' in symbols
patches=''.join((r/'patches'/p).read_text() for p in ['radiolib-7.8.1.patch','jpegdec-1.8.4.patch'])
for s in ['CROWPANEL_RADIOLIB_PATCH','beforeUplink','afterUplink','// CrowPanel']: assert s not in patches,s
svg=(r/'docs/wiring.svg').read_text()
embedded=re.findall(r'data:image/png;base64,([^"\s]+)',svg)
assert any(hashlib.sha256(base64.b64decode(s)).hexdigest()==h(r/'docs/source-photo.png') for s in embedded)
num=subprocess.check_output(['git','diff','--numstat'],text=True)
counts={g:{'added':0,'removed':0} for g in ['application','tests','tooling','dependency_source']};files={}
for line in num.splitlines():
 a,d,p=line.split('\t'); group='tests' if p.startswith('tests/') else 'tooling' if p.startswith('tools/') else 'application' if p.endswith(('.ino','.h','.cpp')) or p=='ttn-decoder.js' else None
 if group:
  counts[group]['added']+=int(a);counts[group]['removed']+=int(d);files[p]={'added':int(a),'removed':int(d)}
for old in (t/'dependency-baseline').rglob('*'):
 if old.is_file():
  rel=old.relative_to(t/'dependency-baseline');new=t/'deps'/rel
  diff=list(difflib.unified_diff(old.read_text().splitlines(),new.read_text().splitlines(),n=0))
  a=sum(s.startswith('+') and not s.startswith('+++') for s in diff);d=sum(s.startswith('-') and not s.startswith('---') for s in diff)
  counts['dependency_source']['added']+=a;counts['dependency_source']['removed']+=d;files[str(rel)]={'added':a,'removed':d}
result={'baseline_commit':base['head'],'lines':counts,'files':files,'scope':'Cumulative final five-fix task; supersedes, does not add to, protocol_parser_checkpoint. Actual dependency source lines, excluding patch context.'}
(t/'final-accounting.json').write_text(json.dumps(result,indent=2)+'\n')
(t/'final-audit.json').write_text(json.dumps({'protected_hashes':{p:h(r/p) for p in protected},'build_copies':'all root CPP/headers identical in all three builds after removing Arduino-inserted initial #line directive','library_selection':'all six external pinned dependencies selected from task deps in all builds','symbols':'enabled applicationDownlink present; no before/after hooks or SDMMCFS begin/end','photo_embedding':'byte-identical','patch_markers':'none'},indent=2)+'\n')
print(json.dumps(counts,indent=2));print('PASS: protected assets, all three build sources/libraries/symbols, photo embedding, patch scope markers')
