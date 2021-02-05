# Obtains Wikipedia's "In the news" today's articles.

import urllib.request
import re
import json
from html.parser import HTMLParser

url = "https://en.wikipedia.org/wiki/Main_Page"
response = urllib.request.urlopen(url)
text =  response.read().decode("utf-8")

text_li = re.search("In the news[\S\n\t\v ]+?<ul>([\S\n\t\v ]+?)<\/ul>", text).group(1)
articles_li = re.findall("<li>([\S\n\t\v ]+?)<\/li>", text_li)

# Iterate all articles and generate JSON feed entries.
wiki_base_url = "https://en.wikipedia.org"

class HTMLFilter(HTMLParser):
  text = ""
  def handle_data(self, data):
      self.text += data

json_feed = "{{\"title\": \"Wikipedia - In the news\", \"items\": [{items}]}}"
items = list()

for article in articles_li:
  article_url = json.dumps(wiki_base_url + re.search("^.+?href=\"(.+?)\"", article).group(1))
  f = HTMLFilter()
  f.feed(article)
  f.text
  article_title = json.dumps(f.text)
  article_html = json.dumps("<div>{}</div>".format(article))
  items.append("{{\"title\": {title}, \"content_html\": {html}, \"url\": {url}}}".format(title=article_title,
                                                                                         html=article_html,
                                                                                         url=article_url))

json_feed = json_feed.format(items=", ".join(items))

print(json_feed)