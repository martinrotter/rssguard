# Obtains producthunt's articles.
# Sample input file whose contents must be provided as stdin: "https://www.producthunt.com/topics/XXXX"

import json
import re
import sys
import urllib.request
from html.parser import HTMLParser
from bs4 import BeautifulSoup
from datetime import datetime

input_data = sys.stdin.read()
soup = BeautifulSoup(input_data, 'html.parser')

json_feed = "{{\"title\": {title}, \"items\": [{items}]}}"
items = list()

regex_batch = re.compile('^styles_postContent__.+$')
for post_content in soup.find_all("div", {"class" : regex_batch}):
  regex_single = re.compile('^styles_content__.+$')
  for content in post_content.find_all("div", {"class" : regex_single}):
    article_url = json.dumps("https://www.producthunt.com" + content.p.a["href"])
    article_title = json.dumps(content.h3.text)
    article_contents = json.dumps(content.p.text)
    article_time = json.dumps(datetime.utcnow().isoformat())
    items.append("{{\"title\": {title}, \"content_html\": {html}, \"url\": {url}, \"date_published\": {date}}}".format(
      title=article_title,
      html=article_contents,
      url=article_url,
      date=article_time))

json_feed = json_feed.format(title = json.dumps(soup.title.text), items = ", ".join(items))
print(json_feed)