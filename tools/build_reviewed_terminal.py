"""Build own supplemental rules from an exact, locally verified native export."""
from pathlib import Path
import argparse,csv,hashlib,json,re
from collections import Counter

def generate(export,records,supplement):
 rows={}
 for row in csv.DictReader(export.open(encoding='utf-8-sig'),delimiter='\t'):
  identity=(int(row['ordinal']),int(row['field_offset']))
  if identity in rows:raise ValueError('duplicate export identity')
  rows[identity]=row
 rules={}
 def add(key,value):
  if any(c in key+value for c in '\n\r\t'):raise ValueError('unescaped control character')
  if key in rules and rules[key]!=value:raise ValueError('conflicting translation')
  rules[key]=value
 for record in json.loads(records.read_text(encoding='utf-8')):
  row=rows[(record['ordinal'],record['field_offset'])];source=row['original_english'];target=record['target']
  if row['prefix_offset']!=record['prefix_offset']:raise ValueError('native field offset mismatch')
  if hashlib.sha256(source.encode()).hexdigest()!=record['source_sha256']:raise ValueError('native source fingerprint mismatch')
  if Counter(re.findall(r'\d+(?:\.\d+)*',source))!=Counter(re.findall(r'\d+(?:\.\d+)*',target)):raise ValueError('numeric role mismatch')
  source=' '.join(source.split()) if '\n' in source or '\r' in source else source
  add(source,target)
  markers=re.findall(r'<(?:(?:m|r)hack=[^>]+|CODE)>',source)
  if len(markers)==1:
   marker=markers[0]
   if marker not in target:raise ValueError('dynamic marker lost')
   add(source.replace(marker,'{*}'),target.replace(marker,'{1}'))
 for key,value in json.loads(supplement.read_text(encoding='utf-8')).items():add(key,value)
 return rules

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--native-export',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
 data=Path(__file__).resolve().parents[1]/'data/reviewed-terminal'
 rules=generate(args.native_export,data/'records.json',data/'interface-supplement.json')
 args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(''.join(k+'\t'+v+'\n' for k,v in rules.items()),encoding='utf-8');print('Generated',len(rules),'verified supplemental rules')
if __name__=='__main__':main()
