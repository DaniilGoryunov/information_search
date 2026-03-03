#!/usr/bin/env python3

import http.server
import urllib.parse
import subprocess
import os
import html

PORT = 8080
SEARCH_BIN = os.path.join(os.path.dirname(__file__), "..", "core", "search_engine")
INDEX_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "index.bin")
FORWARD_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "forward.bin")

PAGE_SIZE = 50

SEARCH_HTML = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Automotive Search</title>
    <style>
        body {{ font-family: Arial, sans-serif; max-width: 800px; margin: 50px auto; padding: 0 20px; }}
        h1 {{ color: #333; }}
        .search-box {{ display: flex; gap: 10px; margin: 30px 0; }}
        .search-box input {{ flex: 1; padding: 12px; font-size: 16px; border: 2px solid #ddd; border-radius: 6px; }}
        .search-box button {{ padding: 12px 24px; background: #2563eb; color: white; border: none; border-radius: 6px; font-size: 16px; cursor: pointer; }}
        .search-box button:hover {{ background: #1d4ed8; }}
    </style>
</head>
<body>
    <h1>Automotive News Search Engine</h1>
    <form action="/search" method="GET" class="search-box">
        <input type="text" name="q" placeholder="Enter search query (e.g. electric vehicle || hybrid car)" value="{query}">
        <button type="submit">Search</button>
    </form>
    {results}
</body>
</html>"""

RESULT_ITEM = """<div class="result">
    <div class="result-title"><a href="{url}">{title}</a></div>
    <div class="result-url">{url}</div>
</div>"""


def run_search(query):
    try:
        proc = subprocess.run(
            [SEARCH_BIN, "search", INDEX_PATH, FORWARD_PATH],
            input=query + "\n",
            capture_output=True,
            text=True,
            timeout=10,
        )
        return proc.stdout
    except Exception as e:
        return f"ERROR: {e}"


def parse_results(output):
    results = []
    for line in output.splitlines():
        if line.startswith("  "):
            parts = line.strip().split("\t")
            if len(parts) >= 3:
                results.append({"id": parts[0], "title": parts[1], "url": parts[2]})
    return results


class SearchHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/search":
            params = urllib.parse.parse_qs(parsed.query)
            query = params.get("q", [""])[0]
            page = int(params.get("page", ["0"])[0])

            raw = run_search(query)
            all_results = parse_results(raw)
            start = page * PAGE_SIZE
            end = start + PAGE_SIZE
            page_results = all_results[start:end]

            result_count = 0
            for line in raw.splitlines():
                if line.startswith("RESULTS:"):
                    result_count = int(line.split(":")[1].strip().split()[0])

            results_html = f"<p>Found {result_count} results</p>\n"
            results_html += '<div class="results">\n'
            for r in page_results:
                results_html += RESULT_ITEM.format(
                    url=html.escape(r["url"]),
                    title=html.escape(r["title"])
                )
            results_html += "</div>\n"

            if end < len(all_results):
                results_html += f'<a href="/search?q={urllib.parse.quote(query)}&page={page+1}">Next 50 results</a>'

            body = SEARCH_HTML.format(query=html.escape(query), results=results_html)
        else:
            body = SEARCH_HTML.format(query="", results="")

        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(body.encode("utf-8"))

    def log_message(self, format, *args):
        print(f"[{self.log_date_time_string()}] {format % args}")


def main():
    server = http.server.HTTPServer(("0.0.0.0", PORT), SearchHandler)
    print(f"Search server running at http://localhost:{PORT}")
    server.serve_forever()


if __name__ == "__main__":
    main()
