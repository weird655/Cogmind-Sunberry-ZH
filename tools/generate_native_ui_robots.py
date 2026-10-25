"""Generate version-pinned robot-name membership, with physical LP provenance.

Generation requires a verified native robot catalog and matching particles.bin.
--check validates the retained hash/identity manifest without needing game text.
Supplying --catalog and --game-data also repeats the physical-byte validation.
"""
from __future__ import annotations
import argparse
import gzip
import hashlib
import json
import struct
from pathlib import Path

EXPECTED_SHA256 = 'fd1f00effa3ed69c8e8286cad9566183a6a54d803cc5040c51b97ce3defb5427'
COUNT, SEED, PRIME = 337, 1469598103934665603, 1099511628211
MASK = (1 << 64) - 1


def text_hash(raw: bytes) -> int:
    value = SEED
    for byte in raw:
        value = ((value ^ byte) * PRIME) & MASK
    return value


def physical_id(prefix: int, payload: int) -> str:
    return hashlib.sha256(json.dumps(('particles.bin', EXPECTED_SHA256, prefix, payload), separators=(',', ':')).encode()).hexdigest()


def validate_manifest(manifest: dict) -> None:
    expected = dict(schema='cogmind-native-ui-robot-membership-v1', source_file='particles.bin',
                    source_sha256=EXPECTED_SHA256, record_count=COUNT, unique_name_count=COUNT,
                    native_field=4, hash_seed=SEED, hash_prime=PRIME, hash_mask_bits=64,
                    sort_order=['length', 'hash'])
    for key, value in expected.items():
        if manifest.get(key) != value:
            raise ValueError('manifest metadata mismatch: ' + key)
    pairs, ordinals, locations = [], set(), set()
    if len(manifest['entries']) != COUNT:
        raise ValueError('robot membership count mismatch')
    for row in manifest['entries']:
        length, hashed = row['length'], int(row['hash'], 16)
        if not isinstance(length, int) or not 1 <= length <= 160 or not 0 <= hashed <= MASK:
            raise ValueError('invalid name length or hash')
        ordinal, prefix, payload = row['ordinal'], row['prefix_offset'], row['payload_offset']
        if not isinstance(ordinal, int) or not 0 <= ordinal < COUNT or ordinal in ordinals:
            raise ValueError('duplicate or unsupported robot ordinal')
        if prefix < 0 or payload != prefix + 4 or (prefix, payload) in locations:
            raise ValueError('duplicate or invalid native LP boundary')
        if row['physical_id'] != physical_id(prefix, payload):
            raise ValueError('physical identity mismatch')
        pairs.append((length, hashed)); ordinals.add(ordinal); locations.add((prefix, payload))
    if pairs != sorted(pairs) or len(set(pairs)) != len(pairs) or ordinals != set(range(COUNT)):
        raise ValueError('unordered, ambiguous or incomplete robot names')


def build_manifest(catalog_path: Path, game_data: Path) -> dict:
    catalog = json.loads(catalog_path.read_text(encoding='utf-8'))
    if catalog['source_sha256'] != EXPECTED_SHA256 or catalog['count'] != COUNT:
        raise ValueError('stale robot catalog')
    packed = (game_data / 'particles.bin').read_bytes()
    if hashlib.sha256(packed).hexdigest() != EXPECTED_SHA256:
        raise ValueError('particles.bin is not the verified version')
    raw = gzip.decompress(packed)
    if struct.unpack_from('<I', raw, catalog['start'])[0] != COUNT:
        raise ValueError('native robot array size mismatch')
    entries, collision_check = [], {}
    for record in catalog['records']:
        fields = [f for f in record['strings'] if f['field'] == 4]
        if len(fields) != 1:
            raise ValueError('robot record requires one native name field')
        field = fields[0]
        prefix, payload = field['prefix_offset'], field['payload_offset']
        if payload != prefix + 4 or not record['start'] <= prefix < payload <= record['end']:
            raise ValueError('name boundary escapes robot record')
        size = struct.unpack_from('<I', raw, prefix)[0]
        name = raw[payload:payload + size]
        if payload + size > record['end'] or not name or len(name) > 160 or any(c < 32 or c > 126 for c in name):
            raise ValueError('invalid robot name bytes')
        if name.decode('ascii') != field['text']:
            raise ValueError('catalog text differs from native payload')
        pair = (len(name), text_hash(name))
        if pair in collision_check:
            raise ValueError('duplicate name or hash collision')
        collision_check[pair] = name
        entries.append(dict(length=pair[0], hash=f'0x{pair[1]:016x}', ordinal=record['ordinal'],
                            prefix_offset=prefix, payload_offset=payload, physical_id=physical_id(prefix, payload)))
    entries.sort(key=lambda r: (r['length'], int(r['hash'], 16)))
    manifest = dict(schema='cogmind-native-ui-robot-membership-v1', source_file='particles.bin',
                    source_sha256=EXPECTED_SHA256, decompressed_sha256=hashlib.sha256(raw).hexdigest(),
                    record_count=COUNT, unique_name_count=COUNT, native_field=4,
                    native_reader='0x9e1260 -> 0x5c2930', catalog_global='0xd25de0',
                    hash_seed=SEED, hash_prime=PRIME, hash_mask_bits=64,
                    sort_order=['length', 'hash'], entries=entries)
    validate_manifest(manifest)
    return manifest


def render_header(manifest: dict) -> str:
    validate_manifest(manifest)
    lines = ['#ifndef COGMIND_NATIVE_UI_ROBOTS_H', '#define COGMIND_NATIVE_UI_ROBOTS_H',
             '#include <stddef.h>', '#include <stdint.h>',
             '/* Generated native robot field +4 membership; no original game text.',
             '   Exact ASCII bytes only, with no alias, case or whitespace normalization. */',
             f'#define NATIVE_UI_ROBOTS_PARTICLES_SHA256 "{EXPECTED_SHA256}"',
             f'#define NATIVE_UI_ROBOT_NAME_COUNT {COUNT}u',
             'typedef struct {uint32_t length;uint64_t hash;} NativeUiRobotName;',
             'static const NativeUiRobotName native_ui_robot_names[NATIVE_UI_ROBOT_NAME_COUNT]={']
    lines += [f' {{{row["length"]}u,UINT64_C({row["hash"]})}},' for row in manifest['entries']]
    lines += ['};', 'static inline int native_robot_name_known(const char *source,size_t length){',
              ' if(!source||!length||length>160)return 0;',
              ' uint64_t hash=UINT64_C(1469598103934665603);',
              ' for(size_t i=0;i<length;i++){unsigned char c=(unsigned char)source[i];if(c<32||c>126)return 0;hash^=c;hash*=UINT64_C(1099511628211);}',
              ' size_t lo=0,hi=NATIVE_UI_ROBOT_NAME_COUNT;',
              ' while(lo<hi){size_t mid=lo+(hi-lo)/2;const NativeUiRobotName *entry=&native_ui_robot_names[mid];',
              '  if(length<entry->length||(length==entry->length&&hash<entry->hash))hi=mid;',
              '  else if(length>entry->length||hash>entry->hash)lo=mid+1;else return 1;',
              ' }return 0;', '}', '#endif', '']
    return '\n'.join(lines)


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    p.add_argument('--catalog', type=Path)
    p.add_argument('--game-data', type=Path)
    p.add_argument('--check', action='store_true')
    a = p.parse_args()
    if bool(a.catalog) != bool(a.game_data):
        p.error('--catalog and --game-data must be supplied together')
    mp, hp = a.root / 'data/reviewed-ui/robot-membership.json', a.root / 'src/native_ui_robots.h'
    if a.check:
        manifest = json.loads(mp.read_text(encoding='utf-8'))
        validate_manifest(manifest)
        if hp.read_text(encoding='utf-8') != render_header(manifest):
            raise ValueError('native_ui_robots.h does not match robot-membership.json')
        if a.catalog and build_manifest(a.catalog, a.game_data) != manifest:
            raise ValueError('manifest differs from verified current source')
    else:
        if not a.catalog:
            p.error('generation requires --catalog and --game-data')
        manifest = build_manifest(a.catalog, a.game_data)
        mp.parent.mkdir(parents=True, exist_ok=True)
        mp.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
        hp.write_text(render_header(manifest), encoding='utf-8')
    print(f'PASS {COUNT} robot names; hash seed {SEED}; check={a.check}')


if __name__ == '__main__':
    main()
