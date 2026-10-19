from pathlib import Path
import argparse,hashlib,json,shutil,zipfile,subprocess,sys,uuid
r=Path(__file__).resolve().parent;p=argparse.ArgumentParser();p.add_argument('--translations',type=Path,required=True);a=p.parse_args();d=a.translations.resolve()
subprocess.run([sys.executable,str(r/'build.py'),'--translations',str(d)],check=True)
version=(r/'VERSION').read_text().strip();stage=r/'build'/('package-simple-'+uuid.uuid4().hex);folder=stage/'cogmind汉化261019';resources=folder/'资源';resources.mkdir(parents=True)
assert stage.resolve().is_relative_to((r/'build').resolve())
files={}
for rel,source in [('autoload/physfs.dll',r/'build/physfs.dll'),('CogmindZH/bin/native_zh.dll',r/'build/native_zh.dll')]+[('CogmindZH/'+n,d/n) for n in ['zh_para.tsv','item_names.tsv','zh_trans.tsv','zh_extra.tsv','runtime_patches.tsv']]:files[rel]=source.read_bytes()
files['CogmindZH/VERSION']=(version+'\n').encode()
for rel,data in files.items():
 target=resources/rel;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
manifest={'version':version,'files':[{'relative':n,'sha256':hashlib.sha256(data).hexdigest()} for n,data in files.items()]};(resources/'manifest.json').write_text(json.dumps(manifest,indent=2))
for name in ['安装汉化.cmd','使用说明.txt']:shutil.copy2(r/'installer'/name,folder/name)
shutil.copy2(r/'installer/Install.ps1',resources/'Install.ps1')
for name in ['LICENSE','NOTICE.md','VERSION']:shutil.copy2(r/name,resources/name)
archive=r/'build'/('Cogmind-Sunberry-ZH-v'+version);shutil.make_archive(str(archive),'zip',stage);print(str(archive)+'.zip')
