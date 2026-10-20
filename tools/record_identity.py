"""Version-scoped record identities from schema extraction, never a strings scan."""
import argparse,collections,csv,gzip,hashlib,json,struct
from pathlib import Path

def identify(rows,data_dir):
 files={};physical=set();paths=collections.Counter();records=[]
 for row in rows:
  name=row['source_file']
  if Path(name).name!=name or name not in ['particles.bin','messages.bin']+[f'dialogue{i}.bin' for i in range(1,7)]:raise ValueError('unsupported source file')
  if name not in files:
   packed=(data_dir/name).read_bytes();files[name]=(hashlib.sha256(packed).hexdigest(),gzip.decompress(packed))
  digest,data=files[name]
  if digest!=row['source_sha256']:raise ValueError('stale extraction: '+name)
  prefix=int(row['prefix_offset'],0);payload=int(row['payload_offset'],0);length=int(row['byte_length'])
  if prefix<0 or payload!=prefix+4 or payload+length>len(data) or length<0 or struct.unpack_from('<I',data,prefix)[0]!=length:raise ValueError('invalid LP boundary: '+name+':'+row['record_path'])
  location=(name,digest,prefix,payload)
  if location in physical:raise ValueError('duplicate physical occurrence')
  physical.add(location);path=(name,digest,row['record_path']);paths[path]+=1
  record={'identity':hashlib.sha256(json.dumps(location,separators=(',',':')).encode()).hexdigest(),'source_file':name,'source_sha256':digest,'prefix_offset':prefix,'payload_offset':payload,'record_path':row['record_path'],'decode_status':row['decode_status'],'original_english':row['original_english']}
  records.append(record)
 for record in records:record['semantic_path_ambiguous']=paths[(record['source_file'],record['source_sha256'],record['record_path'])]>1
 return records

def candidates(records,english):
 # No normalization across semantic records and no first-result selection.
 matches=[r for r in records if r['decode_status']=='decoded' and r['original_english']==english]
 return {'status':'missing' if not matches else 'unique' if len(matches)==1 else 'ambiguous','records':matches}

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--extracted',type=Path,required=True);p.add_argument('--data-dir',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 with a.extracted.open(encoding='utf-8-sig',newline='') as f:records=identify(list(csv.DictReader(f,delimiter='\t')),a.data_dir)
 with a.output.open('w',encoding='utf-8',newline='\n') as f:
  for record in records:f.write(json.dumps(record,ensure_ascii=False)+'\n')
 print(json.dumps({'physical_records':len(records),'ambiguous_semantic_path_occurrences':sum(r['semantic_path_ambiguous'] for r in records),'cross_version_identity':False,'runtime_binding':False}))
