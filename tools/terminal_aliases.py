"""Generate exact aliases only for proven same-record bracket variants.

Export must come from TERMINAL_RESOURCE_PROBE using the current dictionaries.
This tool does not translate, fuzzily search, or merge records with similar names.
"""
import argparse, csv, hashlib, json
from pathlib import Path
from terminal_records import parse_bank, compare_native

def aliases(bank, exported):
    compare_native(bank, exported)
    by_id = {}
    for row in exported:
        by_id.setdefault(int(row['ordinal']), {})[int(row['field_offset'])] = row
    choices = {}; evidence = []
    for ordinal, fields in by_id.items():
        for a,b in ((60,88),(120,148)):
            left,right = fields[a],fields[b]
            x,y = (bytes.fromhex(r['decoded_hex']) for r in (left,right))
            # Only the native markup differs; no whitespace, case or word edits.
            if not x or not y or x.replace(b'[',b'').replace(b']',b'') != y.replace(b'[',b'').replace(b']',b''): continue
            for source,anchor in ((left,right),(right,left)):
                if int(source['match']) != 0 or int(anchor['match']) <= 0: continue
                raw = bytes.fromhex(source['decoded_hex']); target = bytes.fromhex(anchor['target_hex'])
                if any(c in raw+target for c in (b'\x00',b'\t',b'\n',b'\r')): continue
                english=raw.decode('ascii'); chinese=target.decode('utf-8')
                if not any('\u3400' <= c <= '\u9fff' for c in chinese): continue
                choices.setdefault(english,set()).add(chinese)
                evidence.append(dict(ordinal=ordinal, source_field=int(source['field_offset']),
                                     anchor_field=int(anchor['field_offset']), original_english=english,
                                     chinese=chinese, source_sha256=bank['source_sha256']))
    unique={s:next(iter(v)) for s,v in choices.items() if len(v)==1}
    return unique,[r for r in evidence if r['original_english'] in unique]

def main():
    p=argparse.ArgumentParser();p.add_argument('--particles',type=Path,required=True)
    p.add_argument('--native-export',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();packed=a.particles.read_bytes();bank=parse_bank(packed)
    with a.native_export.open(encoding='utf-8-sig',newline='') as f: rows=list(csv.DictReader(f,delimiter='\t'))
    result,evidence=aliases(bank,rows)
    a.output.mkdir(parents=True,exist_ok=True)
    with (a.output/'terminal_aliases.tsv').open('w',encoding='utf-8',newline='') as f:
        for source,target in sorted(result.items()): f.write(source+'\t'+target+'\n')
    with (a.output/'terminal_paragraphs.tsv').open('w',encoding='utf-8-sig',newline='') as f:
        keys=['ordinal','field_offset','prefix_offset','payload_offset','original_english','match','chinese','link_restore']
        w=csv.DictWriter(f,keys,delimiter='\t');w.writeheader()
        for row in rows:
            if int(row['field_offset'])==4 or not row['decoded_hex']: continue
            record=bank['records'][int(row['ordinal'])];field=next(v for v in record['fields'] if v['field_offset']==int(row['field_offset']))
            w.writerow(dict(ordinal=row['ordinal'],field_offset=row['field_offset'],prefix_offset=hex(field['prefix_offset']),payload_offset=hex(field['payload_offset']),original_english=bytes.fromhex(row['decoded_hex']).decode('ascii'),match=row['match'],chinese=bytes.fromhex(row['target_hex']).decode('utf-8'),link_restore=row['link_restore']))
    summary=dict(records=len(bank['records']),verified_fields=compare_native(bank,rows),
                 alias_count=len(result),bank_start=hex(bank['bank_start']),bank_end=hex(bank['bank_end']),
                 source_sha256=hashlib.sha256(packed).hexdigest(),complete_file=False,evidence=evidence)
    (a.output/'terminal_audit.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in summary.items() if k!='evidence'}))
if __name__=='__main__':main()
