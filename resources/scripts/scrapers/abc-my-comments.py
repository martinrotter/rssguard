# Obtains Abclinuxu's "mé komentáře" as JSON feed.

import urllib.request
import re
import json
import sys
from datetime import datetime
from html.parser import HTMLParser

base_url = "https://www.abclinuxu.cz"
url = "https://www.abclinuxu.cz/History?type=comments&uid={uid}".format(uid=sys.argv[1])
response = urllib.request.urlopen(url)
text =  response.read().decode("utf-8")

abc_title = re.search("<h1>(.+?)</h1>", text).group(1)
abc_table = re.search("<table class=\"ds\">(.+?)<\/table>", text, re.S).group(1)

articles_iter = re.finditer("<tr>.+?href=\"(.+?)\">(.+?)</a>.+?\"td-datum\">.+?(\d{1,2}\.\d{1,2}\.\d{4} \d{2}:\d{2}).+?</tr>", abc_table, re.S)

# Iterate all articles and generate JSON feed entries.

class HTMLFilter(HTMLParser):
  text = ""
  def handle_data(self, data):
      self.text += data

json_feed = "{{\"title\": \"{title}\", \"items\": [{items}]}}"
items = list()

for article in articles_iter:
  article_url = json.dumps(base_url + article.group(1))

  f = HTMLFilter()
  f.feed(article.group(2))

  article_title = json.dumps(f.text)
  article_time = json.dumps(datetime.strptime(article.group(3), "%d.%m.%Y %H:%M").isoformat())
  items.append("{{\"title\": {title}, \"content_html\": {html}, \"url\": {url}, \"date_published\": {date}}}".format(title=article_title,
                                                                                                                     html=article_title,
                                                                                                                     url=article_url,
                                                                                                                     date=article_time))

json_feed = json_feed.format(title=abc_title, items=", ".join(items))
print(json_feed)