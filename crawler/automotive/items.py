import scrapy


class ArticleItem(scrapy.Item):
    url = scrapy.Field()
    html = scrapy.Field()
    source = scrapy.Field()
    title = scrapy.Field()
