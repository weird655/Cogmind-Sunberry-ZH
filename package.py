from pathlib import Path
import argparse,hashlib,json,shutil,zipfile,subprocess,sys
r=Path(__file__).resolve().parent;p=argparse.ArgumentParser();p.add_argument('--translations',type=Path,required=True);a=p.parse_args();d=a.translations.resolve()
subprocess.run([sys.executable,str(r/'build.py'),'--translations',str(d)],check=True)
version=(r/'VERSION').read_text().strip();stage=r/'build/package';stage.mkdir(parents=True,exist_ok=True);files={}
for rel,source in [('autoload/physfs.dll',r/'build/physfs.dll'),('CogmindZH/bin/native_zh.dll',r/'build/native_zh.dll')]+[('CogmindZH/'+n,d/n) for n in ['zh_para.tsv','item_names.tsv','zh_trans.tsv','zh_extra.tsv','runtime_patches.tsv']]:files[rel]=source.read_bytes()
files['CogmindZH/VERSION']=(version+'\n').encode();manifest={'version':version,'files':[{'relative':n,'sha256':hashlib.sha256(data).hexdigest()} for n,data in files.items()]}
with zipfile.ZipFile(stage/'payload.zip','w',zipfile.ZIP_DEFLATED) as z:
 for n,data in files.items():z.writestr(n,data)
 z.writestr('manifest.json',json.dumps(manifest,indent=2))
for source in (r/'installer').iterdir():
 if source.is_file():shutil.copy2(source,stage/source.name)
for n in ['LICENSE','NOTICE.md','VERSION']:shutil.copy2(r/n,stage/n)
archive=r/'build'/('Cogmind-Sunberry-ZH-v'+version);shutil.make_archive(str(archive),'zip',stage);print(str(archive)+'.zip')
