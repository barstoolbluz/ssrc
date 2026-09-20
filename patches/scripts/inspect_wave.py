#!/usr/bin/env python3
"""Independent RIFF/RF64/Wave64 structure inspector for SSRC regression work."""
import struct
import sys
from pathlib import Path

W64_RIFF = bytes.fromhex("726966662e91cf11a5d628db04c10000")
W64_WAVE = bytes.fromhex("77617665f3acd3118cd100c04f8edb8a")
W64_FMT  = bytes.fromhex("666d7420f3acd3118cd100c04f8edb8a")
W64_FACT = bytes.fromhex("66616374f3acd3118cd100c04f8edb8a")
W64_DATA = bytes.fromhex("64617461f3acd3118cd100c04f8edb8a")

def u16(b, p): return struct.unpack_from("<H", b, p)[0]
def u32(b, p): return struct.unpack_from("<I", b, p)[0]
def u64(b, p): return struct.unpack_from("<Q", b, p)[0]

def inspect(path):
    b = Path(path).read_bytes()
    print(f"{path}: {len(b)} bytes")
    if b[:16] == W64_RIFF:
        if len(b) < 40 or b[24:40] != W64_WAVE:
            raise ValueError("invalid Wave64 root")
        root = u64(b, 16)
        print(f"  container=W64 root_size={root} root_matches_file={root == len(b)}")
        pos = 40
        block_align = None
        data_bytes = None
        fact = None
        while pos + 24 <= len(b):
            gid = b[pos:pos+16]
            size = u64(b, pos+16)
            if size < 24 or pos + size > len(b): raise ValueError("invalid W64 chunk")
            body = size - 24
            name = "fmt" if gid == W64_FMT else "fact" if gid == W64_FACT else "data" if gid == W64_DATA else gid.hex()
            print(f"  chunk={name} offset={pos} size={size} body={body} aligned={(pos % 8) == 0}")
            if gid == W64_FMT:
                tag, ch, rate, _, block_align, bits = struct.unpack_from("<HHIIHH", b, pos+24)
                print(f"    fmt tag={tag} channels={ch} rate={rate} block_align={block_align} bits={bits}")
            elif gid == W64_FACT:
                fact = u64(b, pos+24) if body == 8 else None
                print(f"    fact_frames={fact}")
            elif gid == W64_DATA:
                data_bytes = body
            pos += (size + 7) & ~7
        if data_bytes is not None and block_align:
            frames = data_bytes // block_align
            print(f"  data_bytes={data_bytes} data_frames={frames} fact_matches={fact == frames if fact is not None else False}")
        return

    if b[:4] not in (b"RIFF", b"RF64") or b[8:12] != b"WAVE":
        raise ValueError("not RIFF/RF64 WAVE or Wave64")
    rf64 = b[:4] == b"RF64"
    print(f"  container={'RF64' if rf64 else 'RIFF'} root_u32=0x{u32(b,4):08x}")
    pos = 12
    block_align = None
    ds64_data = None
    ds64_samples = None
    data_bytes = None
    fact = None
    while pos + 8 <= len(b):
        cid = b[pos:pos+4]
        size32 = u32(b, pos+4)
        name = cid.decode("ascii", "replace")
        body = size32
        if rf64 and cid == b"data" and size32 == 0xffffffff:
            if ds64_data is None: raise ValueError("RF64 data before ds64")
            body = ds64_data
        if pos + 8 + body > len(b): raise ValueError("chunk exceeds file")
        print(f"  chunk={name} offset={pos} size32=0x{size32:08x} body={body}")
        p = pos + 8
        if cid == b"ds64":
            riff64, ds64_data, ds64_samples, table = struct.unpack_from("<QQQI", b, p)
            print(f"    ds64_riff_size={riff64} ds64_data_size={ds64_data} ds64_sample_count={ds64_samples} table_len={table}")
        elif cid == b"fmt ":
            tag, ch, rate, _, block_align, bits = struct.unpack_from("<HHIIHH", b, p)
            print(f"    fmt tag={tag} channels={ch} rate={rate} block_align={block_align} bits={bits}")
        elif cid == b"fact":
            fact = u32(b, p) if body == 4 else None
            print(f"    fact_u32={fact if fact is not None else 'unexpected-width'}")
        elif cid == b"data":
            data_bytes = body
        pos = p + body + (body & 1)
    if data_bytes is not None and block_align:
        frames = data_bytes // block_align
        if rf64:
            print(f"  data_bytes={data_bytes} data_frames={frames} ds64_matches={ds64_samples == frames} fact_sentinel={fact == 0xffffffff if fact is not None else False}")
        else:
            print(f"  data_bytes={data_bytes} data_frames={frames} fact_matches={fact == frames if fact is not None else False}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(f"usage: {sys.argv[0]} FILE [FILE ...]")
    for arg in sys.argv[1:]: inspect(arg)
