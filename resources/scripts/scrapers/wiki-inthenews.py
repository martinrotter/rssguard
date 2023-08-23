# Obtains Wikipedia's "In the news" today's articles.

import urllib.request
import re
import json
from newspaper import Article

url = "https://en.wikipedia.org/wiki/Main_Page"
response = urllib.request.urlopen(url)
text =  response.read().decode("utf-8")

text_li = re.search("In the news[\S\n\t\v ]+?<ul>([\S\n\t\v ]+?)<\/ul>", text).group(1)
articles_li = re.findall("<li>([\S\n\t\v ]+?)<\/li>", text_li)

# Iterate all articles and generate JSON feed entries.
wiki_base_url = "https://en.wikipedia.org"


json_feed = "{{\"title\": \"Wikipedia - In the news\", \"items\": [{items}]}}"
items = list()

for article in articles_li:
  article_url = wiki_base_url + re.search("^.+?href=\"(.+?)\"", article).group(1)

  f = Article(article_url, keep_article_html = True)
  f.download()
  f.parse()

  article_url = json.dumps(article_url)
  article_title = json.dumps(f.title)
  article_html = json.dumps(f.article_html)
  items.append("{{\"title\": {title}, \"content_html\": {html}, \"url\": {url}}}".format(title=article_title,
                                                                                         html=article_html,
                                                                                         url=article_url))

json_feed = json_feed.format(items=", ".join(items))

print(json_feed)