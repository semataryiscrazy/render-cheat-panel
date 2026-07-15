#!/usr/bin/env python3
"""
dll_to_header.py — Convert a DLL to a C header file for embedding.

Usage:
    python dll_to_header.py cheat.dll ../ghost/loader.h

This generates a header file with the DLL bytes as a const unsigned char array,
for use with the ghost agent's reflective loader (-DCHEAT_DLL_EMBEDDED).
"""

import os
import sys
import binascii

def dll_to_header(dll_path, output_path):
    if not os.path.exists(dll_path):
        print(f"[ERROR] DLL not found: {dll_path}")
        sys.exit(1)

    with open(dll_path, "rb") as f:
        data = f.read()

    dll_name = os.path.basename(dll_path).replace(".", "_")
    array_name = f"{dll_name}_bytes"

    lines = []
    lines.append("#pragma once")
    lines.append("#include <cstdint>")
    lines.append("")
    lines.append(f"// Auto-generated from {dll_path}")
    lines.append(f"// Size: {len(data)} bytes ({len(data) / 1024:.1f} KB)")
    lines.append("")
    lines.append(f"const unsigned char {array_name}[] = {{")
    
    # Write bytes in rows of 16
    hex_str = binascii.hexlify(data).decode("ascii")
    for i in range(0, len(hex_str), 32):
        chunk = hex_str[i:i+32]
        bytes_str = ", ".join(f"0x{chunk[j:j+2]}" for j in range(0, len(chunk), 2))
        lines.append(f"    {bytes_str},")

    lines.append("};")
    lines.append("")
    lines.append(f"const size_t {dll_name}_size = sizeof({array_name});")
    lines.append("")

    with open(output_path, "w") as f:
        f.write("\n".join(lines))

    print(f"[OK] {len(data)} bytes written to {output_path}")
    print(f"     Array name: {array_name}")
    print(f"     Size: {len(data) / 1024:.1f} KB")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python dll_to_header.py <input.dll> <output.h>")
        print("  Example: python dll_to_header.py cheat.dll ../ghost/loader.h")
        sys.exit(1)
    
    dll_to_header(sys.argv[1], sys.argv[2])
