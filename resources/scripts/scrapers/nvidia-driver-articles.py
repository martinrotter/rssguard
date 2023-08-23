# Parses output of Nvidia's GDC web service, which provides
# list of articles.
# Sample input file whose contents must be provided as stdin: "https://www.nvidia.com/bin/nvidiaGDC/servlet/article.json?locale=en_US&region=us&type=both&tag=drivers&offset=0"
# This scripts outputs JSON feed 1.1: https://jsonfeed.org/version/1.1

import json
import sys
from datetime import datetime

json_data = json.loads(sys.stdin.read())
json_root = json_data[0]
json_feed = {"title": "Nvidia " + json_root["articleLocalizedTag"], "items": []}

for article in json_root["articlePagesList"]:
    new_item = {
        "title": article["articleTitle"],
        "authors": [{"name": article["authorName"]}],
        "content_text": article["articleShortDescription"],
        "url": article["articlePath"],
        "date_published": datetime.strptime(article["articleDate"], "%B %d, %Y").isoformat()
    }
    json_feed["items"].append(new_item)

print(json.dumps(json_feed))
