# Parses output of Nvidia's GDC web service, which provides
# list of articles.
# Sample input file whose contents must be provided as stdin: "https://www.nvidia.com/bin/nvidiaGDC/servlet/article.json?locale=en_US&region=us&type=both&tag=drivers&offset=0"
# This scripts outputs JSON feed 1.1: https://jsonfeed.org/version/1.1

import json
import sys
from datetime import datetime

json_data = json.loads(sys.stdin.read())
json_feed = "{{\"title\": \"{title}\", \"items\": [{items}]}}"
items = list()

json_root = json_data[0]

for ite in json_root["articlePagesList"]:
    article_author = json.dumps(ite["authorName"])
    article_url = json.dumps(ite["articlePath"])
    article_title = json.dumps(ite["articleTitle"])
    article_time = json.dumps(datetime.strptime(ite["articleDate"], "%B %d, %Y").isoformat())
    article_contents = json.dumps(ite["articleShortDescription"])

    items.append("{{\"title\": {title}, \"authors\": [{{\"name\": {author}}}], \"content_text\": {html}, \"url\": {url}, \"date_published\": {date}}}".format(title=article_title, html=article_contents, url=article_url, date=article_time, author=article_author))

json_feed = json_feed.format(title="Nvidia " + json_root["articleLocalizedTag"], items=", ".join(items))
print(json_feed)
