from pathlib import Path
import json,re,hashlib,argparse
parser=argparse.ArgumentParser();parser.add_argument('--exe',type=Path);args=parser.parse_args()
R=Path(__file__).resolve().parent;src=R/'src';seen=set();todo=[src/'probe.c']
while todo:
 p=todo.pop()
 if p in seen or not p.exists():continue
 seen.add(p)
 todo.extend(src/q for q in re.findall(r'#include\s+"([^\"]+)"',p.read_text(encoding='utf-8')) if q!='native_layout_steam.h')
required={int(x,16) for p in seen for x in re.findall(r'\b0x[0-9a-fA-F]{6,8}\b',p.read_text(encoding='utf-8')) if 0x401000<=int(x,16)<0xe00000}
registered={int(x,16) for x in re.findall(r'case (0x[0-9a-f]+):',(src/'native_layout_steam.h').read_text())}
missing=required-registered;assert not missing,('Unregistered native addresses:',[hex(a) for a in sorted(missing)])
expected='6d24c525c12de38f3fc21017b023f22ea6d3ec94e3b160d83ef0570327701666'
if args.exe:assert hashlib.sha256(args.exe.read_bytes()).hexdigest()==expected,'Unsupported Steam executable fingerprint'
out={'source_address_inventory_complete':True,'required':len(required),'registered':len(registered),'steam_exe_sha256':expected,'exe_checked':bool(args.exe),'limitations':'A complete address inventory is not a substitute for runtime validation.'}
(R/'layout-preflight.json').write_text(json.dumps(out,indent=2),encoding='utf-8');print('PASS Steam source address inventory:',len(required),'registered references')
