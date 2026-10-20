from pathlib import Path
import argparse,os,subprocess,sys,shutil,hashlib,re
r=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(description='Build the exact-version Cogmind translation adapter; no game/data redistribution.')
parser.add_argument('--translations',type=Path)
parser.add_argument('--identity-extracted',type=Path,help='Optional schema-extracted TSV for verified item record binding')
parser.add_argument('--terminal-native-export',type=Path,help='Optional verified native terminal field export for physical record binding')
parser.add_argument('--game-data',type=Path,help='Read-only matching original data directory')
parser.add_argument('--smoke',action='store_true',help='Run synthetic tests without translation tables or a game installation')
parser.add_argument('--tests',action='store_true',help='Run all current-corpus regression tests before building')
a=parser.parse_args();out=r/'build';out.mkdir(exist_ok=True)
tool=os.environ.get('COGMIND_CRT_TOOLCHAIN');cc=str(Path(tool)/'i686-w64-mingw32-clang.exe') if tool else shutil.which('i686-w64-mingw32-clang.exe')
if not cc or not Path(cc).is_file():parser.error('Set COGMIND_CRT_TOOLCHAIN to the LLVM MinGW bin directory.')
os.environ['COGMIND_CRT_TOOLCHAIN']=str(Path(cc).parent)
subprocess.run([sys.executable,str(r/'tools/audit_text_routes.py')],check=True)
def run(args,**kwargs):subprocess.run([str(x) for x in args],check=True,**kwargs)
if a.smoke:
 for name in ['owner_index_tests','canonical_item_names_tests','linked_tests','replacement_animation_tests','live_cells_tests']:
  exe=out/(name+'.exe');run([cc,'-O2','-Wall','-Wextra','-Werror',r/'src'/(name+'.c'),r/'src/text_core.c',r/'src/glyph_registry.c','-o',exe]);run([exe],cwd=out)
 print('Synthetic smoke tests passed.');sys.exit(0)
d=a.translations.resolve() if a.translations else None
names=['zh_para.tsv','item_names.tsv','zh_trans.tsv','zh_extra.tsv','runtime_patches.tsv']
if d is None or any(not (d/n).is_file() for n in names):parser.error('--translations must name a directory containing the five required TSV files.')
if a.tests:
 os.environ['COGMIND_TRANSLATION_DIR']=str(d);run([sys.executable,r/'verify.py'])
if a.terminal_native_export and not a.identity_extracted:parser.error('--terminal-native-export requires --identity-extracted')
identity_flags=[]
if a.identity_extracted:
 if not a.game_data:parser.error('--identity-extracted requires --game-data')
 run([sys.executable,r/'tools/generate_identity_manifest.py','--extracted',a.identity_extracted.resolve(),'--data-dir',a.game_data.resolve(),'--output',out/'record_identity_generated.h',*(['--terminal-native-export',a.terminal_native_export.resolve()] if a.terminal_native_export else [])])
 identity_flags=['-DRECORD_IDENTITY_MANIFEST','-I'+str(out)]
core=out/'native_zh.dll'
run([cc,'-O2','-Wall','-Wextra','-Werror','-DCHINESE_DEMO','-DSHELL_PROBE',*identity_flags,'-shared',r/'src/probe.c',r/'src/text_core.c',r/'src/glyph_registry.c','-lgdi32','-lbcrypt','-Wl,--kill-at','-o',core])
loader=out/'loader';shutil.copytree(r/'loader',loader,dirs_exist_ok=True)
hashes=[hashlib.sha256(p.read_bytes()).hexdigest() for p in [core]+[d/n for n in names]]
header=loader/'payload_hashes.h';text=header.read_text();matches=list(re.finditer(r'\{L"[^\"]+","([0-9a-f]{64})"\}',text))
if len(matches)!=6:raise RuntimeError('Expected exactly six Chinese hash entries')
for m,h in zip(reversed(matches),reversed(hashes)):text=text[:m.start(1)]+h+text[m.end(1):]
header.write_text(text)
run([cc,'-O2','-Wall','-Wextra','-Werror','-static','-shared',loader/'autoload.c',loader/'physfs_proxy.def','-lbcrypt','-o',out/'physfs.dll'])
(out/'ChineseHashes.ini').write_text('[Chinese]\n'+''.join('SHA'+str(i)+'='+h+'\n' for i,h in enumerate(hashes)))
print('Production build ready in build/. Use package.py to create the standalone installer; no Mod prerequisite.')
