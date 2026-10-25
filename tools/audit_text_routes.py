"""Static routing guard, not a claim of complete game text coverage."""
from pathlib import Path
import argparse,json,re,sys

def audit(root):
 src=root/'src';header=(src/'text_context.h').read_text(encoding='utf-8')
 enum=re.search(r'typedef enum \{(TEXT_ENTRY_DISPLAY.*?)\} TextEntry;',header,re.S)
 ids=[x.strip() for x in enum.group(1).split(',') if x.strip()!='TEXT_ENTRY_COUNT']
 specs=re.findall(r'\{(TEXT_GROUP_\w+),(TEXT_SEM_\w+),"([^"]+)"\}',header)
 errors=[];uses={entry:[] for entry in ids}
 if len(ids)!=len(specs) or len(set(ids))!=len(ids):errors.append('registry enum/table mismatch')
 if len({s[2] for s in specs})!=len(specs):errors.append('duplicate registry names')
 # Pure matching helpers deliberately remain below the shared service boundary.
 helpers={'text_core.h','text_core.c','translation_overlay.h','canonical_item_names.h','content_names.h','dialogue_dynamic.h','event_dynamic.h','event_stats.h','native_ui_text.h','display_text.h','item_name_text.h','linked_text.h','repair_text.h','ui_text.h','terminal_text.h','message_resolver.h'}
 offline={'content_audit.c','perf_bench.c'}
 bypass=re.compile(r'\b(?:zh_translate(?:_exact|_normalized_exact|_unique_prefix|_overlay)?|display_translate|terminal_translate|dialogue_dynamic_translate|event_dynamic_translate|canonical_item_name_translate|message_resolve(?:_profile|_terminal)?)\s*\(')
 for p in sorted(src.iterdir()):
  if p.suffix not in {'.h','.c'} or p.name.endswith('_tests.c') or p.name in helpers|offline:continue
  text=p.read_text(encoding='utf-8');text=re.sub(r'/\*.*?\*/|//[^\n]*',lambda m:'\n'*m.group(0).count('\n'),text,flags=re.S)
  for number,line in enumerate(text.splitlines(),1):
   if bypass.search(line):errors.append(f'{p.name}:{number}: untyped translation bypass')
   for token in set(re.findall(r'\bTEXT_ENTRY_[A-Z_]+\b',line)):
    if token in uses:uses[token].append(f'{p.name}:{number}')
    elif token!='TEXT_ENTRY_COUNT':errors.append(f'{p.name}:{number}: unknown entry {token}')
 for entry,locations in uses.items():
  if not locations:errors.append(f'unused registered entry: {entry}')
 return {'scope':'static native adapter routes; not whole-game coverage','entries':[{'id':i,'group':s[0],'semantic':s[1],'name':s[2],'uses':uses[i]} for i,s in zip(ids,specs)],'errors':errors,'passed':not errors}

if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);parser.add_argument('--json',type=Path);a=parser.parse_args();report=audit(a.root)
 if a.json:a.json.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 print(('PASS' if report['passed'] else 'FAIL')+f" route audit: {len(report['entries'])} entries; {len(report['errors'])} errors")
 for error in report['errors']:print(error)
 sys.exit(not report['passed'])
