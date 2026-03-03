import scrapy
from automotive.items import ArticleItem


class MotortrendSpider(scrapy.Spider):
    name = "motortrend"
    allowed_domains = ["motortrend.com"]

    custom_settings = {
        "DOWNLOAD_DELAY": 0.5,
    }

    def __init__(self, config="../config.yaml", *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.config = config
        self.pages_crawled = 0
        self.max_pages = 18000

    def start_requests(self):
        sitemap_urls = [
            "https://www.motortrend.com/sitemap_index.xml",
        ]
        for url in sitemap_urls:
            yield scrapy.Request(url, callback=self.parse_sitemap_index)

    def parse_sitemap_index(self, response):
        namespaces = {"sm": "http://www.sitemaps.org/schemas/sitemap/0.9"}
        locs = response.xpath("//sm:sitemap/sm:loc/text()", namespaces=namespaces).getall()
        for loc in locs:
            if "news" in loc or "post" in loc or "article" in loc or "review" in loc:
                yield scrapy.Request(loc, callback=self.parse_sitemap)
        if not locs:
            yield from self.parse_sitemap(response)

    def parse_sitemap(self, response):
        namespaces = {"sm": "http://www.sitemaps.org/schemas/sitemap/0.9"}
        urls = response.xpath("//sm:url/sm:loc/text()", namespaces=namespaces).getall()
        for url in urls:
            if self.pages_crawled >= self.max_pages:
                return
            if "/news/" in url or "/reviews/" in url or "/first-look/" in url or "/first-drive/" in url:
                self.pages_crawled += 1
                yield scrapy.Request(url, callback=self.parse_article)

    def parse_article(self, response):
        title = response.css("h1::text").get()
        if not title:
            title = response.xpath("//title/text()").get()
        if not title:
            return

        item = ArticleItem()
        item["url"] = response.url
        item["html"] = response.text
        item["source"] = "motortrend"
        item["title"] = title.strip()
        yield item
