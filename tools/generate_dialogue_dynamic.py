"""Compile reviewed scaffolds into a deterministic C table; never reads game files."""
from pathlib import Path
import argparse,json,re
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args()
rows=json.loads((root/'data/reviewed-terminal/dialogue-dynamic.json').read_text(encoding='utf-8'))
output='/* Generated from reviewed database scaffolds; never edit by hand. */\nstatic const DialogueDynamicRule dialogue_dynamic_rules[]={\n'
seen=set()
for row in rows:
 s,t,k=row['source'],row['target'],row['kind']
 assert s not in seen and s.count('%1')==t.count('%1') in (1,2);seen.add(s)
 assert k in ['atom','build','list','identity','slot']
 assert not any(c in s+t for c in '\t\r\n')
 assert re.findall(r'\d+',s.replace('%1',''))==re.findall(r'\d+',t.replace('%1',''))
 assert row['records']
 output+=' {'+','.join([json.dumps(s),json.dumps(t,ensure_ascii=False),'DD_'+k.upper()])+'},\n'
output+='};\n';target=root/'src/dialogue_dynamic_rules.h'
if a.check:assert target.read_text(encoding='utf-8')==output,'Regenerate dialogue_dynamic_rules.h from reviewed data'
else:target.write_text(output,encoding='utf-8')
print('Verified reviewed dynamic scaffolds:',len(rows))
