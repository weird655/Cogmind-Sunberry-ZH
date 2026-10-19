from pathlib import Path
import subprocess,os,json
r=Path(__file__).resolve().parent
(r/'bin').mkdir(exist_ok=True)
cc=Path(os.environ['COGMIND_CRT_TOOLCHAIN'])/'i686-w64-mingw32-clang.exe'
d=Path(os.environ.get('COGMIND_TRANSLATION_DIR',r/'test-data'));para=d/'zh_para.tsv';shorts=d/'zh_trans.tsv';extra=d/'zh_extra.tsv';items=d/'item_names.tsv';runtime=d/'runtime_patches.tsv'
suites={'message_policy_tests':[para,shorts,extra,items,runtime],'replacement_animation_tests':[],'live_cells_tests':[],'owner_index_tests':[],'content_tests':[para,shorts,extra,items,runtime],'core_tests':[para],'display_tests':[items,shorts,para,extra],'item_name_tests':[items],'canonical_item_names_tests':[],'repair_tests':[para],'repair_popup_tests':[items,shorts,para,extra],'menu_compact_tests':[],'linked_tests':[],'link_target_tests':[para,shorts],'terminal_tests':[para,shorts,extra,runtime],'terminal_normalize_tests':[para,shorts,extra,runtime]}
results=[]
for name,args in suites.items():
 out=r/'bin'/(name+'.exe')
 subprocess.run([str(cc),'-O2','-Wall','-Wextra','-Werror',str(r/'src'/(name+'.c')),str(r/'src/text_core.c'),str(r/'src/glyph_registry.c'),'-o',str(out)],check=True)
 result=subprocess.run([str(out),*map(str,args)],cwd=d,encoding='utf-8',errors='replace',capture_output=True)
 print(name,result.returncode,result.stdout.strip(),result.stderr.strip());results.append({'test':name,'exit':result.returncode,'stdout':result.stdout,'stderr':result.stderr})
 if result.returncode:raise SystemExit(result.returncode)
(r/'test-results.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
subprocess.run([str(cc),'-O2','-Wall','-Wextra','-Werror','-DCHINESE_DEMO','-DSHELL_PROBE','-shared',str(r/'src/probe.c'),str(r/'src/text_core.c'),str(r/'src/glyph_registry.c'),'-lgdi32','-Wl,--kill-at','-o',str(r/'bin/native_zh.dll')],check=True)

