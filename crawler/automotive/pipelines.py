import sqlite3
import hashlib
import time
import os
import yaml


class SQLitePipeline:
    def open_spider(self, spider):
        config_path = getattr(spider, "config", "../config.yaml")
        with open(config_path, "r") as f:
            cfg = yaml.safe_load(f)

        db_path = cfg["db"]["path"]
        db_dir = os.path.dirname(os.path.abspath(db_path))
        os.makedirs(db_dir, exist_ok=True)

        self.conn = sqlite3.connect(db_path)
        self.conn.execute("""
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
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_url ON documents(url)")
        self.conn.commit()

    def close_spider(self, spider):
        self.conn.close()

    def process_item(self, item, spider):
        html = item.get("html", "")
        content_hash = hashlib.md5(html.encode("utf-8", errors="replace")).hexdigest()
        now = int(time.time())

        row = self.conn.execute(
            "SELECT content_hash FROM documents WHERE url = ?",
            (item["url"],)
        ).fetchone()

        if row is None:
            self.conn.execute(
                "INSERT INTO documents (url, html, title, source, crawled_at, content_hash) VALUES (?, ?, ?, ?, ?, ?)",
                (item["url"], html, item.get("title", ""), item["source"], now, content_hash)
            )
        elif row[0] != content_hash:
            self.conn.execute(
                "UPDATE documents SET html=?, title=?, crawled_at=?, content_hash=? WHERE url=?",
                (html, item.get("title", ""), now, content_hash, item["url"])
            )

        self.conn.commit()
        return item
