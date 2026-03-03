#!/usr/bin/env python3

import sqlite3
import os
import sys
import re
from bs4 import BeautifulSoup


def clean_html(html):
    soup = BeautifulSoup(html, "lxml")
    for tag in soup(["script", "style", "nav", "footer", "header", "aside",
                     "iframe", "noscript", "svg", "form"]):
        tag.decompose()

    text = soup.get_text(separator="\n")
    lines = []
    for line in text.splitlines():
        line = line.strip()
        if line:
            lines.append(line)
    text = "\n".join(lines)
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text


def extract_title(html):
    soup = BeautifulSoup(html, "lxml")
    h1 = soup.find("h1")
    if h1:
        return h1.get_text(strip=True)
    title = soup.find("title")
    if title:
        return title.get_text(strip=True)
    return "Untitled"


def main():
    db_path = sys.argv[1] if len(sys.argv) > 1 else "data/corpus.db"
    out_dir = sys.argv[2] if len(sys.argv) > 2 else "data/texts"
    os.makedirs(out_dir, exist_ok=True)

    conn = sqlite3.connect(db_path)
    cursor = conn.execute("SELECT id, url, html, title, source FROM documents")

    count = 0
    total_raw = 0
    total_clean = 0

    for row in cursor:
        doc_id, url, html, title, source = row
        if not html:
            continue

        total_raw += len(html.encode("utf-8", errors="replace"))
        clean = clean_html(html)
        if len(clean) < 200:
            continue

        if not title:
            title = extract_title(html)

        header = f"TITLE: {title}\nURL: {url}\nSOURCE: {source}\n\n"
        full_text = header + clean

        filename = f"{doc_id:06d}.txt"
        filepath = os.path.join(out_dir, filename)
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(full_text)

        total_clean += len(full_text.encode("utf-8"))
        count += 1
        if count % 5000 == 0:
            print(f"Extracted {count} documents...")

    conn.close()
    print(f"\nDone: {count} documents extracted")
    print(f"Raw HTML total: {total_raw / 1048576:.1f} MB")
    print(f"Clean text total: {total_clean / 1048576:.1f} MB")
    print(f"Average clean text size: {total_clean // max(count, 1)} bytes")


if __name__ == "__main__":
    main()
