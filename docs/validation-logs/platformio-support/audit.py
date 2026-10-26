from pathlib import Path
import hashlib,json,subprocess,re
r=Path.cwd();t=Path('/tmp/codex/build/CrowPanelTelemetry/platformio-support');base=json.loads((t/'baseline.json').read_text());protected={}
for p,h in base['hashes'].items():
 if p.endswith(('.cpp','.h','.ino','.js')) or p.startswith(('tools/','tests/','patches/','.clang')) or p in ['Makefile','Config.h','docs/source-photo.png','docs/wiring.svg','docs/wiring.png','docs/radio-nets.json','docs/ui-preview.svg','docs/ui-preview.png','artifacts/CrowPanelTelemetry-delivery.zip'] or p.startswith('docs/review/'):
  assert hashlib.sha256((r/p).read_bytes()).hexdigest()==h,p;protected[p]=h
assert not (r/'CrowPanelTelemetry.ino.cpp').exists()
nm='/home/voc/.arduino15/packages/esp32/tools/esp-rv32/2601/bin/riscv32-esp-elf-nm';results=[]
for kind in ['arduino','pio']:
 for name in ['blank-prev3','enabled-prev3','enabled-postv3']:
  full=('arduino-'+name) if kind=='arduino' else ('pio-final-'+name)
  log=(t/(full+'.log')).read_text();elf=t/full/'CrowPanelTelemetry.ino.elf' if kind=='arduino' else t/'evidence-binaries'/full/'firmware.elf'
  symbols=subprocess.check_output([nm,'-C',str(elf)],text=True)
  assert not any(s in symbols for s in ['beforeUplink','afterUplink','SDMMCFS::begin','SDMMCFS::end'])
  if 'enabled' in name:assert 'PersistentNode::applicationDownlink' in symbols
  if kind=='arduino':
   flash=int(re.search(r'Sketch uses (\d+)',log)[1]);ram=int(re.search(r'Global variables use (\d+)',log)[1])
   for p in list(r.glob('*.cpp'))+list(r.glob('*.h')):
    data=(t/full/'sketch'/p.name).read_bytes()
    if data.startswith(b'#line 1 '):data=data.split(b'\n',1)[1]
    assert data==p.read_bytes(),str(p)
   for dep in ['Arduino_GFX','RadioLib','INA226','TinyGPSPlus','JPEGDEC','audio-driver']:assert str(t/'deps'/dep) in log,dep
  else:
   flash=int(re.search(r'Flash:.*?used (\d+) bytes',log)[1]);ram=int(re.search(r'RAM:.*?used (\d+) bytes',log)[1]);assert '[SUCCESS]' in log
   part=(t/'evidence-binaries'/full/'partitions.bin').read_bytes();assert part==(t/('arduino-'+name)/'CrowPanelTelemetry.ino.partitions.bin').read_bytes()
   for dep in ['GFX Library for Arduino','RadioLib','INA226','TinyGPSPlus','JPEGDEC','audio-driver']:assert dep in log,dep
   found=set(re.findall(r'/src/([^\s]+\.o)',log));assert found=={'Application.cpp.o','Ui.cpp.o','Workers.cpp.o','CrowPanelTelemetry.ino.cpp.o'},found
  results.append({'workflow':kind,'configuration':name,'flash':flash,'ram':ram,'elf_sha256':hashlib.sha256(elf.read_bytes()).hexdigest()})
for label,folder in [('radiolib-7.8.1','RadioLib'),('jpegdec-1.8.4','JPEGDEC')]:
 for rel,v in json.loads((r/'patches'/(label+'-hashes.json')).read_text()).items():assert hashlib.sha256((t/'deps'/folder/rel).read_bytes()).hexdigest()==v['after']
meta=json.loads((t/'metadata-prev3.json').read_text())['crowpanel-prev3'];defs=set(meta['defines'])
for expected in ['BOARD_HAS_PSRAM','ARDUINO_USB_MODE=0','ARDUINO_USB_CDC_ON_BOOT=0','DEV_EUI=1ULL','APP_KEY={0x01}','NWK_KEY={0x02}','INA_SHUNT_OHMS=0.1f','F_CPU=360000000L']:assert expected in defs,expected
# Check actual selected library includes and the local package-link specifications.
# PlatformIO groups include paths under includes.build. Check both environments.
for env in ['crowpanel-prev3','crowpanel-postv3']:
 metadata=json.loads((t/('metadata-prev3.json' if env.endswith('prev3') else 'metadata-postv3.json')).read_text())[env]
 expected_cpu='F_CPU=360000000L' if env.endswith('prev3') else 'F_CPU=400000000L'
 assert expected_cpu in metadata['defines'] and 'BOARD_HAS_PSRAM' in metadata['defines']
 expected_target='esp32p4_es' if env.endswith('prev3') else 'esp32p4'
 assert any('/framework-arduinoespressif32-libs/'+expected_target+'/' in path for path in metadata['includes']['build'])
 for folder in ['Arduino_GFX','RadioLib','INA226','TinyGPSPlus','JPEGDEC','audio-driver']:
  link=json.loads((t/'workspace/libdeps'/env/(folder+'.pio-link')).read_text())
  assert link['spec']['uri']=='symlink://'+str(t/'deps'/folder),(env,folder)
  assert any(str(t/'deps'/folder) in path for path in metadata['includes']['build']),folder

result={'baseline_commit':base['head'],'protected_files':protected,'build_matrix':results,'partitions':'PIO binary tables match Arduino for all three corresponding builds','sources':'only shared root firmware in PIO; canonical C++/headers match CLI build copies','symbols':'enabled downlink callback present, reservation hooks and SDMMCFS begin/end absent','patches':'exact after hashes retained','metadata':'same six prepared libraries selected, PSRAM/USB/CPU/dummy credential flags verified','core_version':subprocess.check_output(['pio','--version'],text=True).strip()}
(t/'final-audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(results,indent=2));print('PASS: source/pin/assets/review/library/patch/symbol/partition/metadata checks')
