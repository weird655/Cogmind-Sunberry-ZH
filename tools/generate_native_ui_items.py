"""Generate a version-pinned item-name membership set without shipping text.

The hash is the existing record_text_hash variant (offset 1469598103934665603),
not text_core.hash_key's standard FNV-1a offset. Generation requires physical
resource validation. --check needs no game files; supplying both source options
also revalidates the physical records and collision proof.
"""
from __future__ import annotations
import argparse
import csv
import gzip
import hashlib
import json
import re
import struct
from pathlib import Path

EXPECTED_SHA256 = "fd1f00effa3ed69c8e8286cad9566183a6a54d803cc5040c51b97ce3defb5427"
COUNT = 1359
SEED = 1469598103934665603
PRIME = 1099511628211
MASK = (1 << 64) - 1
PATTERN = re.compile(r"item\[(0|[1-9][0-9]*)\]\.(name|display_name)")


def text_hash(raw: bytes) -> int:
    value = SEED
    for byte in raw:
        value = ((value ^ byte) * PRIME) & MASK
    return value


def physical_id(prefix: int, payload: int) -> str:
    location = ("particles.bin", EXPECTED_SHA256, prefix, payload)
    return hashlib.sha256(json.dumps(location, separators=(",", ":")).encode()).hexdigest()


def build_manifest(extracted: Path, game_data: Path) -> dict:
    source = game_data / "particles.bin"
    packed = source.read_bytes()
    if hashlib.sha256(packed).hexdigest() != EXPECTED_SHA256:
        raise ValueError("particles.bin is not the verified version; no generation permitted")
    data = gzip.decompress(packed)  # CRC32 and decompressed size are validated.
    names: dict[bytes, list[dict]] = {}
    paths: set[tuple[int, str]] = set()
    locations: set[tuple[int, int]] = set()
    with extracted.open(encoding="utf-8-sig", newline="") as file:
        for row in csv.DictReader(file, delimiter="\t"):
            if row.get("source_file") != "particles.bin":
                continue
            match = PATTERN.fullmatch(row.get("record_path", ""))
            if not match:
                continue
            ordinal, field = int(match[1]), match[2]
            key = (ordinal, field)
            if ordinal >= COUNT or key in paths:
                raise ValueError("duplicate or unsupported item record path")
            if row["source_sha256"] != EXPECTED_SHA256 or row["decode_status"] != "decoded":
                raise ValueError("stale or undecoded item record")
            prefix = int(row["prefix_offset"], 0)
            payload = int(row["payload_offset"], 0)
            length = int(row["byte_length"])
            if prefix < 0 or payload != prefix + 4 or length <= 0 or length > 160:
                raise ValueError("invalid item name LP boundary or length")
            if payload + length > len(data) or struct.unpack_from("<I", data, prefix)[0] != length:
                raise ValueError("truncated item payload or LP size mismatch")
            if (prefix, payload) in locations:
                raise ValueError("duplicate physical item name occurrence")
            raw = data[payload:payload + length]
            if any(byte < 32 or byte > 126 for byte in raw):
                raise ValueError("name is outside verified printable ASCII/UTF-8 subset")
            if raw.decode("ascii") != row["original_english"]:
                raise ValueError("extracted name differs from physical payload")
            if "raw_hex" in row and bytes.fromhex(row["raw_hex"]) != raw:
                raise ValueError("raw_hex differs from physical payload")
            paths.add(key)
            locations.add((prefix, payload))
            names.setdefault(raw, []).append({
                "ordinal": ordinal, "field": field, "record_path": row["record_path"],
                "prefix_offset": prefix, "payload_offset": payload,
                "physical_id": physical_id(prefix, payload),
            })
    expected = {(ordinal, field) for ordinal in range(COUNT) for field in ("name", "display_name")}
    if paths != expected:
        raise ValueError("require complete indices 0..1358 for both name and display_name")
    collision_check: dict[tuple[int, int], bytes] = {}
    entries = []
    for raw, records in names.items():
        pair = (len(raw), text_hash(raw))
        if pair in collision_check and collision_check[pair] != raw:
            raise ValueError("different names collide in length+record_text_hash")
        collision_check[pair] = raw
        entries.append({"length": pair[0], "hash": f"0x{pair[1]:016x}",
                        "records": sorted(records, key=lambda record: (record["ordinal"], record["field"]))})
    entries.sort(key=lambda entry: (entry["length"], int(entry["hash"], 16)))
    manifest = {
        "schema": "cogmind-native-ui-item-membership-v1",
        "source_file": "particles.bin", "source_sha256": EXPECTED_SHA256,
        "decompressed_sha256": hashlib.sha256(data).hexdigest(),
        "source_name_count": COUNT, "source_display_name_count": COUNT,
        "physical_record_count": 2 * COUNT, "unique_name_count": len(entries),
        "hash_algorithm": "record_text_hash FNV-1a-style uint64 variant",
        "hash_seed": SEED, "hash_prime": PRIME, "hash_mask_bits": 64,
        "sort_order": ["length", "hash"],
        "collision_check": "distinct raw names verified to have distinct (length, hash) pairs",
        "entries": entries,
    }
    validate_manifest(manifest)
    return manifest


def validate_manifest(manifest: dict) -> None:
    required = {"schema": "cogmind-native-ui-item-membership-v1", "source_file": "particles.bin",
                "source_sha256": EXPECTED_SHA256, "source_name_count": COUNT,
                "source_display_name_count": COUNT, "physical_record_count": 2 * COUNT,
                "hash_seed": SEED, "hash_prime": PRIME, "hash_mask_bits": 64,
                "sort_order": ["length", "hash"]}
    for key, value in required.items():
        if manifest.get(key) != value:
            raise ValueError(f"manifest metadata mismatch: {key}")
    entries = manifest["entries"]
    if len(entries) != manifest["unique_name_count"] or not entries:
        raise ValueError("unique name count mismatch")
    pairs, paths, locations = [], set(), set()
    physical_count = 0
    for entry in entries:
        length, hashed = entry["length"], int(entry["hash"], 16)
        if not isinstance(length, int) or not 1 <= length <= 160 or not 0 <= hashed <= MASK:
            raise ValueError("invalid length or uint64 hash")
        pairs.append((length, hashed))
        if not entry["records"]:
            raise ValueError("membership entry has no physical provenance")
        for record in entry["records"]:
            match = PATTERN.fullmatch(record["record_path"])
            if not match:
                raise ValueError("invalid physical record path")
            ordinal, field = int(match[1]), match[2]
            key = (ordinal, field)
            prefix, payload = record["prefix_offset"], record["payload_offset"]
            if record["ordinal"] != ordinal or record["field"] != field or ordinal >= COUNT or key in paths:
                raise ValueError("ambiguous or duplicate physical record identity")
            if prefix < 0 or payload != prefix + 4 or (prefix, payload) in locations:
                raise ValueError("invalid or duplicate physical boundary")
            if record["physical_id"] != physical_id(prefix, payload):
                raise ValueError("physical identity digest mismatch")
            paths.add(key); locations.add((prefix, payload)); physical_count += 1
    if pairs != sorted(pairs) or len(pairs) != len(set(pairs)):
        raise ValueError("membership pairs are unsorted or collide")
    expected = {(ordinal, field) for ordinal in range(COUNT) for field in ("name", "display_name")}
    if paths != expected or physical_count != 2 * COUNT:
        raise ValueError("physical name/display_name indices are incomplete")


def render_header(manifest: dict) -> str:
    validate_manifest(manifest)
    lines = ["#ifndef COGMIND_NATIVE_UI_ITEMS_H", "#define COGMIND_NATIVE_UI_ITEMS_H",
             "#include <stddef.h>", "#include <stdint.h>",
             "/* Generated membership only: no original game text or translations.",
             "   Version-pinned to particles.bin below. Hash matches record_text_hash:",
             "   offset 1469598103934665603 (NOT standard FNV offset), prime 1099511628211.",
             "   Exact bytes only; no whitespace, suffix, alias or case normalization. */",
             f'#define NATIVE_UI_ITEMS_PARTICLES_SHA256 "{EXPECTED_SHA256}"',
             f'#define NATIVE_UI_ITEM_NAME_COUNT {manifest["unique_name_count"]}u',
             "typedef struct {uint32_t length;uint64_t hash;} NativeUiItemName;",
             "static const NativeUiItemName native_ui_item_names[NATIVE_UI_ITEM_NAME_COUNT]={"]
    for entry in manifest["entries"]:
        lines.append(f' {{{entry["length"]}u,UINT64_C({entry["hash"]})}},')
    lines += ["};", "static inline int native_ui_item_name_known(const char *source,size_t length){",
              " if(!source||!length||length>160)return 0;",
              " uint64_t hash=UINT64_C(1469598103934665603);",
              " for(size_t i=0;i<length;i++){unsigned char c=(unsigned char)source[i];if(c<32||c>126)return 0;hash^=c;hash*=UINT64_C(1099511628211);}",
              " size_t lo=0,hi=NATIVE_UI_ITEM_NAME_COUNT;",
              " while(lo<hi){size_t mid=lo+(hi-lo)/2;const NativeUiItemName *entry=&native_ui_item_names[mid];",
              "  if(length<entry->length||(length==entry->length&&hash<entry->hash))hi=mid;",
              "  else if(length>entry->length||hash>entry->hash)lo=mid+1;else return 1;",
              " }return 0;", "}", "#endif", ""]
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1], help="44 project root")
    parser.add_argument("--manifest", type=Path, help="override data/reviewed-ui/item-membership.json")
    parser.add_argument("--header", type=Path, help="override src/native_ui_items.h")
    parser.add_argument("--extracted", type=Path, help="typed extracted TSV")
    parser.add_argument("--game-data", type=Path, help="read-only directory containing particles.bin")
    parser.add_argument("--check", action="store_true", help="validate JSON/header consistency without writing")
    args = parser.parse_args()
    manifest_path = args.manifest or args.root / "data/reviewed-ui/item-membership.json"
    header_path = args.header or args.root / "src/native_ui_items.h"
    if bool(args.extracted) != bool(args.game_data):
        parser.error("--extracted and --game-data must be supplied together")
    if args.check:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        validate_manifest(manifest)
        if header_path.read_text(encoding="utf-8") != render_header(manifest):
            raise ValueError("native_ui_items.h does not match item-membership.json")
        if args.extracted and build_manifest(args.extracted, args.game_data) != manifest:
            raise ValueError("manifest differs from physically verified current source")
    else:
        if not args.extracted:
            parser.error("generation requires --extracted and --game-data")
        manifest = build_manifest(args.extracted, args.game_data)
        manifest_path.parent.mkdir(parents=True, exist_ok=True)
        header_path.parent.mkdir(parents=True, exist_ok=True)
        manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        header_path.write_text(render_header(manifest), encoding="utf-8")
    print(f'PASS {manifest["unique_name_count"]} unique names; {manifest["physical_record_count"]} physical records; hash seed {SEED}; check={args.check}')


if __name__ == "__main__":
    main()
