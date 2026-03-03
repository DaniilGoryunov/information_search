#!/usr/bin/env python3

import os
import sys


def main():
    texts_dir = sys.argv[1] if len(sys.argv) > 1 else "data/texts"

    files = sorted([f for f in os.listdir(texts_dir) if f.endswith(".txt")])
    doc_count = len(files)
    total_bytes = 0
    sizes = []

    for fname in files:
        path = os.path.join(texts_dir, fname)
        sz = os.path.getsize(path)
        total_bytes += sz
        sizes.append(sz)

    avg_size = total_bytes / doc_count if doc_count > 0 else 0
    min_size = min(sizes) if sizes else 0
    max_size = max(sizes) if sizes else 0

    print(f"Number of documents: {doc_count}")
    print(f"Total corpus size: {total_bytes:,} bytes ({total_bytes / 1048576:.2f} MB)")
    print(f"Average document size: {avg_size:.0f} bytes")
    print(f"Min document size: {min_size} bytes")
    print(f"Max document size: {max_size} bytes")


if __name__ == "__main__":
    main()
