"""Read-only, version-pinned terminal lore bank; no printable/LP scanning.

Native loader 792401 passes D35B58 to 9E18A0; element reader 45B740.
The bank anchor is separately cross-checked against all native field exports.
This parses this bank only and makes no claim about the surrounding file gaps.
"""
import gzip, hashlib, struct

PARTICLES_SHA = 'fd1f00effa3ed69c8e8286cad9566183a6a54d803cc5040c51b97ce3defb5427'
BANK_OFFSET = 0xc7e0d3
FIELD_OFFSETS = (4, 0x3c, 0x58, 0x78, 0x94)

def parse_bank(packed):
    if hashlib.sha256(packed).hexdigest() != PARTICLES_SHA:
        raise ValueError('unsupported particles fingerprint')
    data = gzip.decompress(packed)
    p = BANK_OFFSET
    def u32():
        nonlocal p
        if p + 4 > len(data): raise ValueError('truncated scalar')
        n = struct.unpack_from('<I', data, p)[0]; p += 4
        return n
    def text(ordinal, field):
        nonlocal p
        prefix = p; n = u32()
        if n > 16384 or p + n > len(data): raise ValueError('invalid text length')
        raw = data[p:p+n]; p += n
        return dict(ordinal=ordinal, field_offset=field, prefix_offset=prefix,
                    payload_offset=prefix+4, raw_hex=raw.hex())
    count = u32()
    if count != 917: raise ValueError('unsupported terminal bank count')
    records = []
    for ordinal in range(count):
        start = p
        actual_id = u32()
        if actual_id != ordinal: raise ValueError(f'terminal ordinal mismatch {ordinal} at {start:x}: {actual_id}')
        fields = [text(ordinal, 4)]
        numbers = [u32() for _ in range(7)]
        fields.extend([text(ordinal, 0x3c), text(ordinal, 0x58)])
        extra = u32()
        fields.extend([text(ordinal, 0x78), text(ordinal, 0x94)])
        n = u32()
        if n > 32: raise ValueError('unsupported link count')
        # 45B720 reads only the first word of each 8-byte runtime link object.
        links = [u32() for _ in range(n)]
        # Serialized IDs can include sentinels; do not reinterpret as native child IDs.
        records.append(dict(ordinal=ordinal, start=start, end=p,
                            numbers=numbers, extra=extra, links=links, fields=fields))
    return dict(source_sha256=PARTICLES_SHA, bank_start=BANK_OFFSET,
                bank_end=p, records=records, complete_bank=True, complete_file=False)

def compare_native(bank, exported):
    expected = {(r['ordinal'], f['field_offset']): f['raw_hex']
                for r in bank['records'] for f in r['fields']}
    actual = {}
    for row in exported:
        key = (int(row['ordinal']), int(row['field_offset']))
        if key in actual or int(row['native_id']) != key[0]:
            raise ValueError('duplicate or remapped native record')
        actual[key] = row['raw_hex']
    if actual != expected: raise ValueError('native/file terminal field mismatch')
    return len(expected)
