#!/usr/bin/env python3

import asyncio
import aiohttp
import sqlite3
import hashlib
import time
import os
import re
import xml.etree.ElementTree as ET

DB_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "corpus.db")
MAX_WORKERS = 80
TIMEOUT = aiohttp.ClientTimeout(total=25, connect=10)
UA = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36"
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


async def fetch(session, url):
    headers = {"User-Agent": UA, "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"}
    try:
        async with session.get(url, headers=headers, timeout=TIMEOUT, ssl=False) as resp:
            if resp.status == 200:
                return await resp.text(errors="replace")
    except Exception:
        pass
    return None


def parse_sitemap(xml_text):
    urls, subs = [], []
    try:
        root = ET.fromstring(xml_text)
    except ET.ParseError:
        return urls, subs
    for elem in root.iter():
        tag = elem.tag.split("}")[-1] if "}" in elem.tag else elem.tag
        if tag == "loc" and elem.text:
            t = elem.text.strip()
            if t.endswith(".xml") or "sitemap" in t.lower():
                subs.append(t)
            else:
                urls.append(t)
    return urls, subs


async def get_sitemap_urls(session, sitemap_url):
    all_urls = []
    to_process = [sitemap_url]
    processed = set()
    while to_process:
        batch = to_process[:20]
        to_process = to_process[20:]
        tasks = []
        for u in batch:
            if u in processed:
                continue
            processed.add(u)
            tasks.append(fetch(session, u))
        if not tasks:
            continue
        results = await asyncio.gather(*tasks)
        for text in results:
            if not text:
                continue
            urls, subs = parse_sitemap(text)
            all_urls.extend(urls)
            for s in subs:
                if s not in processed:
                    to_process.append(s)
    return all_urls


def extract_title(html):
    m = re.search(r"<h1[^>]*>(.*?)</h1>", html, re.DOTALL | re.IGNORECASE)
    if m:
        t = re.sub(r"<[^>]+>", "", m.group(1)).strip()
        if t and len(t) > 3:
            return t[:500]
    m = re.search(r"<title[^>]*>(.*?)</title>", html, re.DOTALL | re.IGNORECASE)
    if m:
        t = re.sub(r"<[^>]+>", "", m.group(1)).strip()
        if t and len(t) > 3:
            return t[:500]
    return ""


async def download_worker(queue, session, results_queue, stats):
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
                stats["ok"] += 1
            else:
                stats["fail"] += 1
        else:
            stats["fail"] += 1
        stats["done"] += 1
        queue.task_done()


async def main():
    conn = init_db()
    existing = set(r[0] for r in conn.execute("SELECT url FROM documents").fetchall())
    current = len(existing)
    target = 35000
    need = target - current

    print(f"Current: {current}, need: {need}")
    if need <= 0:
        print("Done!")
        conn.close()
        return

    connector = aiohttp.TCPConnector(limit=100, limit_per_host=20, force_close=True)
    session = aiohttp.ClientSession(connector=connector)

    all_urls = []

    print("\n=== Collecting URLs ===")

    print("  thedrive.com (all sections)...", end=" ", flush=True)
    td_urls = await get_sitemap_urls(session, "https://www.thedrive.com/sitemap.xml")
    td_new = [(u, "thedrive") for u in td_urls
              if u not in existing
              and not any(x in u for x in [".jpg", ".png", ".gif", "/author/", "/tag/"])]
    all_urls.extend(td_new)
    print(f"{len(td_urls)} total, {len(td_new)} new")

    print("  topspeed.com...", end=" ", flush=True)
    ts_urls = await get_sitemap_urls(session, "https://www.topspeed.com/sitemap_index.xml")
    ts_new = [(u, "topspeed") for u in ts_urls
              if u not in existing
              and not any(x in u for x in [".jpg", ".png", "/video/", "/tag/", "/category/", "/author/"])
              and len(u) > 40]
    all_urls.extend(ts_new)
    print(f"{len(ts_urls)} total, {len(ts_new)} new")

    print("  carbuzz.com...", end=" ", flush=True)
    cb_urls = await get_sitemap_urls(session, "https://carbuzz.com/sitemap_index.xml")
    cb_new = [(u, "carbuzz") for u in cb_urls
              if u not in existing
              and not any(x in u for x in [".jpg", ".png", "/video/", "/tag/"])
              and len(u) > 30]
    all_urls.extend(cb_new)
    print(f"{len(cb_urls)} total, {len(cb_new)} new")

    EXTRA_SITEMAPS = [
        ("roadandtrack", "https://www.roadandtrack.com/sitemap.xml"),
        ("caranddriver", "https://www.caranddriver.com/sitemap.xml"),
        ("autoblog", "https://www.autoblog.com/sitemap.xml"),
        ("edmunds", "https://www.edmunds.com/sitemap.xml"),
        ("kbb", "https://www.kbb.com/sitemap.xml"),
    ]
    for name, sm in EXTRA_SITEMAPS:
        print(f"  {name}...", end=" ", flush=True)
        urls = await get_sitemap_urls(session, sm)
        new = [(u, name) for u in urls
               if u not in existing
               and not any(x in u for x in [".jpg", ".png", "/video/", "/tag/", "/author/"])
               and len(u) > 30]
        all_urls.extend(new)
        print(f"{len(urls)} total, {len(new)} new")

    import random
    random.shuffle(all_urls)
    seen = set()
    deduped = []
    for url, src in all_urls:
        if url not in seen:
            seen.add(url)
            deduped.append((url, src))
    all_urls = deduped

    margin = int(need * 2.5)
    if len(all_urls) > margin:
        all_urls = all_urls[:margin]

    print(f"\nTotal URLs to download: {len(all_urls)}")
    if not all_urls:
        await session.close()
        conn.close()
        return

    print(f"\n=== Downloading ({MAX_WORKERS} workers) ===")
    queue = asyncio.Queue(maxsize=MAX_WORKERS * 3)
    results_queue = asyncio.Queue()
    stats = {"done": 0, "ok": 0, "fail": 0, "saved": 0}

    workers = [asyncio.create_task(download_worker(queue, session, results_queue, stats))
               for _ in range(MAX_WORKERS)]
    start = time.time()

    async def feed():
        for item in all_urls:
            await queue.put(item)
        for _ in range(MAX_WORKERS):
            await queue.put(None)

    async def save():
        batch = []
        while True:
            try:
                item = await asyncio.wait_for(results_queue.get(), timeout=2.0)
                url, html, title, source = item
                h = hashlib.md5(html.encode("utf-8", errors="replace")).hexdigest()
                batch.append((url, html, title, source, int(time.time()), h))
                if len(batch) >= 200:
                    for b in batch:
                        try:
                            conn.execute("INSERT OR IGNORE INTO documents (url,html,title,source,crawled_at,content_hash) VALUES (?,?,?,?,?,?)", b)
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
                    conn.execute("INSERT OR IGNORE INTO documents (url,html,title,source,crawled_at,content_hash) VALUES (?,?,?,?,?,?)", b)
                    stats["saved"] += 1
                except Exception:
                    pass
            conn.commit()

    async def progress():
        while not all(w.done() for w in workers):
            await asyncio.sleep(10)
            el = time.time() - start
            rate = stats["done"] / el if el > 0 else 0
            total = current + stats["saved"]
            print(f"  [{el:.0f}s] {stats['done']}/{len(all_urls)} | "
                  f"OK: {stats['ok']} | Fail: {stats['fail']} | "
                  f"Saved: {stats['saved']} | DB: {total} | {rate:.1f}/s", flush=True)

    await asyncio.gather(feed(), save(), progress(), *workers, return_exceptions=True)

    total = conn.execute("SELECT COUNT(*) FROM documents").fetchone()[0]
    sources = conn.execute("SELECT source, COUNT(*) FROM documents GROUP BY source ORDER BY COUNT(*) DESC").fetchall()

    await session.close()
    conn.close()

    el = time.time() - start
    print(f"\n=== Done in {el:.1f}s ===")
    print(f"Total in DB: {total}")
    for s, c in sources:
        print(f"  {s}: {c}")


if __name__ == "__main__":
    asyncio.run(main())
