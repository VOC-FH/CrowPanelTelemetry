from pathlib import Path
import subprocess,os,json,concurrent.futures
T=Path('/tmp/codex/build/CrowPanelTelemetry/platformio-support')
def build(config):
 name,variant,enabled=config;tmp=T/(name+'-tmp');tmp.mkdir(exist_ok=True)
 cmd=['/tmp/codex/build/CrowPanelTelemetry/rereview-fix/cli/arduino-cli','compile','--verbose','--jobs','2','--fqbn','esp32:esp32:esp32p4:FlashSize=16M,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB,ChipVariant='+variant,'--libraries',str(T/'deps'),'--build-path',str(T/name)]
 if enabled:cmd+=['--build-property','compiler.cpp.extra_flags=-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f']
 cmd+=['.']
 with (T/(name+'.log')).open('w') as f:
  f.write('Command: '+repr(cmd)+'\n');f.flush();result=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,env=dict(os.environ,TMPDIR=str(tmp),ARDUINO_BUILD_CACHE_PATH=str(T/(name+'-cache'))))
 print(name,result.returncode,flush=True);return {'configuration':name,'command':cmd,'exit':result.returncode}
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as ex:results=list(ex.map(build,[('arduino-blank-prev3','prev3',False),('arduino-enabled-prev3','prev3',True),('arduino-enabled-postv3','postv3',True)]))
(T/'arduino-build-results.json').write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(any(r['exit'] for r in results))
