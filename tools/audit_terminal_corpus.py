"""Run verified terminal records through the compiled production matcher, read-only."""
import argparse, collections, csv, json, re, struct, subprocess
from pathlib import Path
from terminal_records import parse_bank,compare_native

def run(probe,translations,particles,native_export,output):
    bank=parse_bank(particles.read_bytes())
    with native_export.open(encoding='utf-8-sig',newline='') as f:rows=list(csv.DictReader(f,delimiter='\t'))
    compare_native(bank,rows)
    body=[r for r in rows if int(r['field_offset'])!=4 and r['decoded_hex']]
    output.mkdir(parents=True,exist_ok=True);input_path=output/'requests.bin';result_path=output/'results.bin'
    with input_path.open('wb') as f:
        f.write(struct.pack('<I',len(body)))
        for row in body:
            text=bytes.fromhex(row['decoded_hex']);text.decode('ascii')
            if not text or len(text)>16384 or b'\0' in text:raise ValueError('unsupported decoded body')
            f.write(struct.pack('<I',len(text))+text)
    names=['zh_para.tsv','zh_trans.tsv','zh_extra.tsv','item_names.tsv','runtime_patches.tsv']
    subprocess.run([str(probe.resolve()),*[str((translations/n).resolve()) for n in names],str(input_path.resolve()),str(result_path.resolve())],check=True)
    data=result_path.read_bytes();at=0;results=[]
    for row in body:
        if at+8>len(data):raise ValueError('truncated probe result')
        matched,size=struct.unpack_from('<iI',data,at);at+=8
        if size>=32768 or at+size+4>len(data):raise ValueError('invalid probe result length')
        target=data[at:at+size].decode('utf-8');at+=size;linked=struct.unpack_from('<i',data,at)[0];at+=4
        source=bytes.fromhex(row['decoded_hex']).decode('ascii');record=bank['records'][int(row['ordinal'])]
        field=next(f for f in record['fields'] if f['field_offset']==int(row['field_offset']))
        required=bool(record['links'] and '[' in source and int(row['field_offset']) in (60,120))
        numbers=lambda text:collections.Counter(re.findall(r'\d+(?:\.\d+)*',text))
        results.append(dict(ordinal=row['ordinal'],field_offset=row['field_offset'],prefix_offset=hex(field['prefix_offset']),
                            original_english=source,chinese=target,match=matched,link_result=linked,
                            link_required=required,numeric_review=matched>0 and numbers(source)!=numbers(target)))
    if at!=len(data):raise ValueError('unexpected probe result tail')
    keys=list(results[0])
    for name,filtered in [('terminal_results.tsv',results),('terminal_missing.tsv',[r for r in results if r['match']==0]),
                          ('terminal_links_review.tsv',[r for r in results if r['match']>0 and r['link_required'] and r['link_result']!=1]),
                          ('terminal_numbers_review.tsv',[r for r in results if r['numeric_review']])]:
        with (output/name).open('w',encoding='utf-8-sig',newline='') as f:
            w=csv.DictWriter(f,keys,delimiter='\t');w.writeheader();w.writerows(filtered)
    summary=dict(records=len(bank['records']),verified_fields=len(rows),body_fields=len(results),
                 matched_fields=sum(r['match']>0 for r in results),distinct_missing=len({r['original_english'] for r in results if r['match']==0}),
                 matched_link_fields_requiring_review=sum(r['match']>0 and r['link_required'] and r['link_result']!=1 for r in results),
                 numeric_review_fields=sum(r['numeric_review'] for r in results),scope='verified terminal lore bank; not all game text')
    (output/'summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8');return summary

if __name__=='__main__':
    p=argparse.ArgumentParser()
    for name in ('probe','translations','particles','native-export','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();print(json.dumps(run(a.probe,a.translations,a.particles,a.native_export,a.output)))
