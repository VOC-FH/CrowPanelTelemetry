from pathlib import Path
import subprocess,os,json,shutil
T=Path('/tmp/codex/build/CrowPanelTelemetry/platformio-support');results=[]
for name,variant,enabled in [('pio-final-blank-prev3','crowpanel-prev3',False),('pio-final-enabled-prev3','crowpanel-prev3',True),('pio-final-enabled-postv3','crowpanel-postv3',True)]:
 env=dict(os.environ,DEPS=str(T/'deps'),PLATFORMIO_CORE_DIR=str(T/'core'),PLATFORMIO_WORKSPACE_DIR=str(T/'workspace'),TMPDIR=str(T))
 env.pop('PLATFORMIO_BUILD_FLAGS',None)
 if enabled:env['PLATFORMIO_BUILD_FLAGS']='-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f'
 cmd=['pio','run','-e',variant,'-j','3']
 with (T/(name+'.log')).open('w') as out:
  out.write('Command: '+repr(cmd)+'\n');out.write('PLATFORMIO_BUILD_FLAGS='+env.get('PLATFORMIO_BUILD_FLAGS','')+'\n');out.flush();code=subprocess.run(cmd,env=env,stdout=out,stderr=subprocess.STDOUT).returncode
 results.append({'configuration':name,'environment':variant,'command':cmd,'build_flags':env.get('PLATFORMIO_BUILD_FLAGS',''),'exit':code});print(name,code,flush=True)
 if code==0:
  dest=T/'evidence-binaries'/name;dest.mkdir(parents=True,exist_ok=True)
  for n in ['firmware.elf','firmware.bin','partitions.bin','bootloader.bin']:shutil.copy2(T/'workspace/build'/variant/n,dest/n)
 (T/'final-pio-build-results.json').write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(any(x['exit'] for x in results))
