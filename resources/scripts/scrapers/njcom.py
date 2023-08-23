# Obtains articles from "connect.nj.com"
# Sample input file whose contents must be provided as stdin: "https://connect.nj.com/staff/bklapisch/posts.html"

import json
import re
import sys
import html
import urllib.request
from html.parser import HTMLParser
from bs4 import BeautifulSoup
from datetime import datetime

sys.stdin.reconfigure(encoding='utf-8')
input_data = sys.stdin.read()
soup = BeautifulSoup(input_data, 'html.parser')
json_feed = "{{\"title\": {title}, \"items\": [{items}]}}"
items = list()

for content in soup.find_all("article"):
    article_url = json.dumps(content.find("h2").a["href"])
    article_title = json.dumps(content.find("h2").text.replace("  ", ""))
    article_contents = json.dumps(str(content.find("p")))
    article_time = json.dumps(content.find("time")["datetime"])
    items.append("{{\"title\": {title}, \"content_html\": {html}, \"url\": {url}, \"date_published\": {date}}}".format(
      title=article_title,
      html=article_contents,
      url=article_url,
      date=article_time))

json_feed = json_feed.format(title = json.dumps(soup.title.text), items = ", ".join(items))
print(json_feed)