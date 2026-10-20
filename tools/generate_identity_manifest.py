"""Build private numeric provenance metadata; no game strings in generated header."""
import argparse,csv,gzip,hashlib,json,re
from pathlib import Path
from record_identity import identify

def text_hash(raw):
 h=1469598103934665603
 for b in raw:h=((h^b)*1099511628211)&0xffffffffffffffff
 return h

def generate(extracted,data_dir,output):
 with extracted.open(encoding='utf-8-sig',newline='') as f:records=identify(list(csv.DictReader(f,delimiter='\t')),data_dir)
 packed=(data_dir/'particles.bin').read_bytes();data=gzip.decompress(packed);names={}
 for row in records:
  match=re.fullmatch(r'item\[(\d+)\]\.name',row['record_path'])
  if row['source_file']!='particles.bin' or not match:continue
  i=int(match[1]);raw=data[row['payload_offset']:row['payload_offset']+int.from_bytes(data[row['prefix_offset']:row['payload_offset']],'little')]
  if row['semantic_path_ambiguous'] or i in names or row['decode_status']!='decoded' or raw.decode('ascii')!=row['original_english']:raise ValueError('unverified item name record')
  names[i]=(row,raw)
 if sorted(names)!=list(range(1359)):raise ValueError('expected verified complete 1359-entry name bank')
 lines=['/* Generated private provenance, no raw English or translated text. */','#define RECORD_PARTICLES_SHA256 "'+hashlib.sha256(packed).hexdigest()+'"','#define RECORD_NAME_COUNT 1359','static const RecordName record_names[RECORD_NAME_COUNT]={']
 for i in sorted(names):
  row,raw=names[i];lines.append('{%du,%du,%du,%du,UINT64_C(0x%016x),"%s"},'%(i,row['prefix_offset'],row['payload_offset'],len(raw),text_hash(raw),row['identity']))
 lines.append('};');output.write_text('\n'.join(lines)+'\n',encoding='utf-8');return len(names)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--extracted',type=Path,required=True);p.add_argument('--data-dir',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();print('Verified identity metadata:',generate(a.extracted,a.data_dir,a.output))
