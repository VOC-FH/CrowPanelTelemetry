from pathlib import Path
import subprocess,os,json
T=Path('/tmp/codex/build/CrowPanelTelemetry/protocol-parser-fixes')
results=[]
for name,variant,enabled in [('blank-prev3','prev3',False),('enabled-prev3','prev3',True),('enabled-postv3','postv3',True)]:
    tmp=T/(name+'-tmp');tmp.mkdir(exist_ok=True)
    cmd=['/tmp/codex/build/CrowPanelTelemetry/rereview-fix/cli/arduino-cli','compile','--verbose','--jobs','3','--fqbn','esp32:esp32:esp32p4:FlashSize=16M,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB,ChipVariant='+variant,'--libraries',str(T/'deps'),'--build-path',str(T/name)]
    if enabled:cmd+=['--build-property','compiler.cpp.extra_flags=-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f']
    cmd+=['.'];print(name,'start',flush=True)
    with (T/(name+'.log')).open('w') as f:
        f.write('Command: '+repr(cmd)+'\n');f.flush()
        r=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,env=dict(os.environ,TMPDIR=str(tmp),ARDUINO_BUILD_CACHE_PATH=str(T/(name+'-cache'))))
    results.append({'configuration':name,'command':cmd,'exit':r.returncode})
    (T/'final-build-results.json').write_text(json.dumps(results,indent=2)+'\n')
    print(name,'exit',r.returncode,flush=True)
