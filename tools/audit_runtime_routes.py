"""Report observed adapter use; unseen is not a claim of a missing hook."""
import argparse,json,re
from pathlib import Path
from audit_text_routes import audit

def report(root,snapshot):
 static=audit(root)
 if not static['passed']:raise ValueError('static registry audit failed')
 rows={}
 for line in snapshot.read_text(encoding='utf-8').splitlines():
  if not line.startswith('ROUTE '):continue
  fields=dict(re.findall(r'([a-z-]+)=([^ ]+)',line));name=fields.pop('entry');counts={key:int(value) for key,value in fields.items()}
  if name in rows:raise ValueError('duplicate runtime entry')
  if counts['calls']!=sum(counts[k] for k in ['matched','missing','ambiguous','invalid']) or counts['bound']>counts['calls'] or counts['layout-failures']>counts['layouts']:raise ValueError('inconsistent runtime counters')
  rows[name]=counts
 expected={row['name'] for row in static['entries']}
 if set(rows)!=expected:raise ValueError('incomplete or incompatible runtime snapshot')
 return {'scope':'observed requests; not whole-game translation percentage','observed':[name for name,c in rows.items() if c['calls'] or c['layouts']],'not_yet_observed':[name for name,c in rows.items() if not c['calls'] and not c['layouts']],'record_bound':[name for name,c in rows.items() if c['bound']],'routes':rows}
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();data=report(a.root,a.snapshot);a.output.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps({k:v for k,v in data.items() if k!='routes'},ensure_ascii=False))
