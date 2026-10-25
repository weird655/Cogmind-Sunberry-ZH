"""Compile reviewed event recipes and reject unconstrained catch-all rules."""
from pathlib import Path
import json,re,argparse
root=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');args=p.parse_args()
rows=json.loads((root/'data/reviewed-events/event-rules.json').read_text(encoding='utf-8'))
marker=re.compile(r'%[123]|\{(?:name|ufd_name)\}|\[name\]')
mapping={'%1':0,'%2':1,'%3':2,'{name}':3,'[name]':3,'{ufd_name}':4}
kinds={'atom':'EV_ATOM','number':'EV_NUMBER','identity':'EV_IDENTITY','message':'EV_MESSAGE','lines':'EV_LINES','code':'EV_CODE','stats':'EV_STATS','actor':'EV_ACTOR','entity':'EV_ENTITY'}
out='/* Generated from reviewed native event templates. */\nstatic const EventDynamicRule event_dynamic_rules[]={\n';seen=set()
for row in rows:
 s,t=row['source'],row['target'];assert s not in seen;seen.add(s)
 assert s.count('\n')<=24 and not any(c in s+t for c in '\t\r')
 a,b=marker.findall(s),marker.findall(t);assert set(a)==set(b) and a
 assert all(a.count(x)==b.count(x) for x in set(a))
 assert len(a)<=6 and len(marker.sub('',s).strip())>=1,'no complete-message catch-all'
 assert sorted(re.findall(r'\d+',marker.sub('',s).replace('September 4, 2018','2018 9 4')))==sorted(re.findall(r'\d+',marker.sub('',t))),(s,'numeric literals')
 types=['EV_NONE']*5
 for x in set(a):types[mapping[x]]=kinds[row['parameters'][x]]
 assert row['records'] and row['evidence']
 out+=' {'+json.dumps(s)+','+json.dumps(t,ensure_ascii=False)+',{'+','.join(types)+'}},\n'
out+='};\n';target=root/'src/event_dynamic_rules.h'
if args.check:assert target.read_text(encoding='utf-8')==out,'Regenerate reviewed event header'
else:target.write_text(out,encoding='utf-8')
print('Verified reviewed event scaffolds:',len(rows))
