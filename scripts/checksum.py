#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys, argparse

parser = argparse.ArgumentParser(description="Checksum Checker")
parser.add_argument(
    "values", type=str, nargs="*", help="Hex values by byte to checksum"
)
args = parser.parse_args()
values = [int(v, 16) for v in args.values]


def checksum():
    ret = 0
    for v in values:
        ret ^= v
    print(f"Checksum: 0x{ret:02X} (decimal {ret})", file=sys.stderr)
    return ret

ret = 0

if len(values) == 0:
    try:
        for line in sys.stdin:
            values.extend(int(v, 16) for v in line.split())
            ret = checksum()
    except KeyboardInterrupt:
        print()
else:
    ret = checksum()

if ret == 0:
    print("Checksum is valid.")
else:
    print("Checksum is invalid.")
    exit(1)
