#!/usr/bin/env python3

import asyncio
import aiohttp
import sqlite3
import hashlib
import time
import os
import sys
import re
import xml.etree.ElementTree as ET
from urllib.parse import urlparse

DB_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "corpus.db")
MAX_WORKERS = 60
TIMEOUT = aiohttp.ClientTimeout(total=20, connect=8)
TARGET_DOCS = 35000
USER_AGENT = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36"

SITEMAP_SOURCES = [
    {
        "name": "motortrend",
        "sitemaps": [
            "https://www.motortrend.com/sitemap_index.xml",
        ],
        "url_filters": ["/news/", "/reviews/", "/first-look/", "/first-drive/", "/cars/"],
        "url_excludes": ["/video/", "/gallery/", "/photo/", ".jpg", ".png"],
    },
    {
        "name": "autoblog",
        "sitemaps": [
            "https://www.autoblog.com/sitemap.xml",
        ],
        "url_filters": [],
        "url_excludes": ["/buy/", "/sell/", "/video/", "/photos/", "/gallery/", ".jpg", ".png", "/car-insurance/", "/refinance/"],
    },
    {
        "name": "caranddriver",
        "sitemaps": [
            "https://www.caranddriver.com/sitemap_index.xml",
        ],
        "url_filters": ["/news/", "/reviews/", "/features/", "/cars/"],
        "url_excludes": ["/video/", "/gallery/", ".jpg", ".png"],
    },
    {
        "name": "topspeed",
        "sitemaps": [
            "https://www.topspeed.com/sitemap_index.xml",
        ],
        "url_filters": [],
        "url_excludes": ["/video/", "/gallery/", ".jpg", ".png", "/category/"],
    },
    {
        "name": "motor1",
        "sitemaps": [
            "https://www.motor1.com/sitemap-index.xml",
        ],
        "url_filters": ["/news/", "/reviews/", "/features/"],
        "url_excludes": ["/video/", "/gallery/", "/photo/", ".jpg", ".png"],
    },
    {
        "name": "carbuzz",
        "sitemaps": [
            "https://carbuzz.com/sitemap_index.xml",
        ],
        "url_filters": ["/news/", "/features/", "/cars/"],
        "url_excludes": ["/video/", ".jpg", ".png"],
    },
    {
        "name": "thedrive",
        "sitemaps": [
            "https://www.thedrive.com/sitemap.xml",
        ],
        "url_filters": ["/news/", "/car-reviews/", "/tech/"],
        "url_excludes": ["/video/", ".jpg", ".png"],
    },
    {
        "name": "autoevolution",
        "sitemaps": [
            "https://www.autoevolution.com/sitemap-index.xml",
        ],
        "url_filters": ["/news/", "/reviews/"],
        "url_excludes": ["/video/", ".jpg", ".png", "/photo/"],
    },
]

NS = {"sm": "http://www.sitemaps.org/schemas/sitemap/0.9"}


def init_db():
    os.makedirs(os.path.dirname(DB_PATH), exist_ok=True)
    conn = sqlite3.connect(DB_PATH)
    conn.execute("""
        CREATE TABLE IF NOT EXISTS documents (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            url TEXT UNIQUE NOT NULL,
            html TEXT NOT NULL,
            title TEXT,
            source TEXT NOT NULL,
            crawled_at INTEGER NOT NULL,
            content_hash TEXT NOT NULL
        )
    """)
    conn.execute("CREATE INDEX IF NOT EXISTS idx_url ON documents(url)")
    conn.commit()
    return conn


def get_existing_urls(conn):
    rows = conn.execute("SELECT url FROM documents").fetchall()
    return set(r[0] for r in rows)


def url_matches(url, source_cfg):
    for exc in source_cfg["url_excludes"]:
        if exc in url.lower():
            return False
    if not source_cfg["url_filters"]:
        return True
    for flt in source_cfg["url_filters"]:
        if flt in url.lower():
            return True
    return False


async def fetch(session, url):
    headers = {"User-Agent": USER_AGENT, "Accept": "text/html,application/xhtml+xml,application/xml"}
    try:
        async with session.get(url, headers=headers, timeout=TIMEOUT, ssl=False) as resp:
            if resp.status == 200:
                return await resp.text(errors="replace")
    except Exception:
        pass
    return None


def parse_sitemap_urls(xml_text):
    urls = []
    sub_sitemaps = []
    try:
        root = ET.fromstring(xml_text)
    except ET.ParseError:
        return urls, sub_sitemaps

    for elem in root.findall("sm:sitemap/sm:loc", NS):
        if elem.text:
            sub_sitemaps.append(elem.text.strip())

    for elem in root.findall("sm:url/sm:loc", NS):
        if elem.text:
            urls.append(elem.text.strip())

    if not urls and not sub_sitemaps:
        for elem in root.iter():
            tag = elem.tag.split("}")[-1] if "}" in elem.tag else elem.tag
            if tag == "loc" and elem.text:
                text = elem.text.strip()
                if "sitemap" in text.lower() and text.endswith(".xml"):
                    sub_sitemaps.append(text)
                else:
                    urls.append(text)

    return urls, sub_sitemaps


def extract_title(html_text):
    m = re.search(r"<h1[^>]*>(.*?)</h1>", html_text, re.DOTALL | re.IGNORECASE)
    if m:
        title = re.sub(r"<[^>]+>", "", m.group(1)).strip()
        if title:
            return title[:500]
    m = re.search(r"<title[^>]*>(.*?)</title>", html_text, re.DOTALL | re.IGNORECASE)
    if m:
        title = re.sub(r"<[^>]+>", "", m.group(1)).strip()
        if title:
            return title[:500]
    return ""


async def collect_urls_from_source(session, source_cfg):
    all_urls = []
    sitemaps_to_process = list(source_cfg["sitemaps"])
    processed_sitemaps = set()

    while sitemaps_to_process:
        batch = sitemaps_to_process[:10]
        sitemaps_to_process = sitemaps_to_process[10:]

        tasks = []
        for sm_url in batch:
            if sm_url in processed_sitemaps:
                continue
            processed_sitemaps.add(sm_url)
            tasks.append(fetch(session, sm_url))

        if not tasks:
            continue

        results = await asyncio.gather(*tasks)

        for xml_text in results:
            if not xml_text:
                continue
            urls, sub_sms = parse_sitemap_urls(xml_text)
            for u in urls:
                if url_matches(u, source_cfg):
                    all_urls.append(u)
            for s in sub_sms:
                if s not in processed_sitemaps:
                    sitemaps_to_process.append(s)

    return all_urls


async def download_worker(name, queue, session, results_queue, stats):
    while True:
        item = await queue.get()
        if item is None:
            queue.task_done()
            break

        url, source = item
        html = await fetch(session, url)
        if html and len(html) > 500:
            title = extract_title(html)
            if title:
                await results_queue.put((url, html, title, source))
                stats["downloaded"] += 1
        else:
            stats["failed"] += 1

        stats["processed"] += 1
        queue.task_done()


def db_writer(conn, results_queue_sync, stats):
    batch = []
    for item in results_queue_sync:
        if item is None:
            break
        url, html, title, source = item
        content_hash = hashlib.md5(html.encode("utf-8", errors="replace")).hexdigest()
        now = int(time.time())
        batch.append((url, html, title, source, now, content_hash))

        if len(batch) >= 100:
            flush_batch(conn, batch, stats)
            batch = []

    if batch:
        flush_batch(conn, batch, stats)


def flush_batch(conn, batch, stats):
    for url, html, title, source, now, content_hash in batch:
        try:
            conn.execute(
                "INSERT OR IGNORE INTO documents (url, html, title, source, crawled_at, content_hash) VALUES (?, ?, ?, ?, ?, ?)",
                (url, html, title, source, now, content_hash)
            )
            stats["saved"] += 1
        except Exception:
            pass
    conn.commit()


async def main():
    conn = init_db()
    existing = get_existing_urls(conn)
    print(f"Existing documents in DB: {len(existing)}")

    connector = aiohttp.TCPConnector(limit=MAX_WORKERS, limit_per_host=15, force_close=True)
    session = aiohttp.ClientSession(connector=connector)

    print(f"\n=== Phase 1: Collecting URLs from sitemaps ===")
    all_article_urls = []
    for src in SITEMAP_SOURCES:
        print(f"  Fetching sitemaps for {src['name']}...", end=" ", flush=True)
        urls = await collect_urls_from_source(session, src)
        new_urls = [(u, src["name"]) for u in urls if u not in existing]
        all_article_urls.extend(new_urls)
        print(f"{len(urls)} total, {len(new_urls)} new")

    import random
    random.shuffle(all_article_urls)

    if len(all_article_urls) > TARGET_DOCS - len(existing):
        all_article_urls = all_article_urls[:TARGET_DOCS - len(existing)]

    print(f"\nTotal URLs to download: {len(all_article_urls)}")

    if not all_article_urls:
        print("Nothing to download!")
        await session.close()
        conn.close()
        return

    print(f"\n=== Phase 2: Downloading articles ({MAX_WORKERS} workers) ===")
    queue = asyncio.Queue(maxsize=MAX_WORKERS * 3)
    results_queue = asyncio.Queue()
    stats = {"processed": 0, "downloaded": 0, "failed": 0, "saved": 0}

    workers = []
    for i in range(MAX_WORKERS):
        w = asyncio.create_task(download_worker(f"w{i}", queue, session, results_queue, stats))
        workers.append(w)

    start_time = time.time()

    async def feed_urls():
        for item in all_article_urls:
            await queue.put(item)
        for _ in range(MAX_WORKERS):
            await queue.put(None)

    async def save_results():
        batch = []
        while True:
            try:
                item = await asyncio.wait_for(results_queue.get(), timeout=1.0)
                url, html, title, source = item
                content_hash = hashlib.md5(html.encode("utf-8", errors="replace")).hexdigest()
                now = int(time.time())
                batch.append((url, html, title, source, now, content_hash))
                if len(batch) >= 200:
                    for b in batch:
                        try:
                            conn.execute(
                                "INSERT OR IGNORE INTO documents (url, html, title, source, crawled_at, content_hash) VALUES (?, ?, ?, ?, ?, ?)", b)
                            stats["saved"] += 1
                        except Exception:
                            pass
                    conn.commit()
                    batch = []
            except asyncio.TimeoutError:
                if all(w.done() for w in workers) and results_queue.empty():
                    break
        if batch:
            for b in batch:
                try:
                    conn.execute(
                        "INSERT OR IGNORE INTO documents (url, html, title, source, crawled_at, content_hash) VALUES (?, ?, ?, ?, ?, ?)", b)
                    stats["saved"] += 1
                except Exception:
                    pass
            conn.commit()

    async def print_progress():
        while not all(w.done() for w in workers):
            await asyncio.sleep(5)
            elapsed = time.time() - start_time
            rate = stats["processed"] / elapsed if elapsed > 0 else 0
            total_in_db = len(existing) + stats["saved"]
            print(f"  [{elapsed:.0f}s] Processed: {stats['processed']}/{len(all_article_urls)} | "
                  f"Downloaded: {stats['downloaded']} | Failed: {stats['failed']} | "
                  f"Saved: {stats['saved']} | DB total: {total_in_db} | "
                  f"Rate: {rate:.1f} pages/s", flush=True)

    feeder = asyncio.create_task(feed_urls())
    saver = asyncio.create_task(save_results())
    progress = asyncio.create_task(print_progress())

    await feeder
    await asyncio.gather(*workers)
    await saver
    progress.cancel()

    elapsed = time.time() - start_time
    total_in_db = conn.execute("SELECT COUNT(*) FROM documents").fetchone()[0]
    sources = conn.execute("SELECT source, COUNT(*) FROM documents GROUP BY source").fetchall()

    await session.close()
    conn.close()

    print(f"\n=== Done in {elapsed:.1f}s ===")
    print(f"Total in DB: {total_in_db}")
    for src, cnt in sources:
        print(f"  {src}: {cnt}")
    print(f"Download rate: {stats['processed'] / elapsed:.1f} pages/s")


if __name__ == "__main__":
    asyncio.run(main())
