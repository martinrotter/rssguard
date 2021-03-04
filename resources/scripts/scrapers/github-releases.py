# Use this script as "post-process" script, let RSS Guard download
# the "releases" file itself.
# 
# This script expects one command line argument:
#   true -> ignore prereleases
#   false -> include prereleases 
#
# Sample input file whose contents must be provided as stdin: https://api.github.com/repos/<user>/<repo>/releases
#
# This script produces JSON feed.

import re
import json
import sys
import distutils.util
from datetime import datetime

leave_out_prereleases = distutils.util.strtobool(sys.argv[1])
input_data = sys.stdin.read()
json_data = json.loads(input_data)

json_feed = "{{\"title\": \"{title}\", \"items\": [{items}]}}"
items = list()

for rel in json_data:
  if rel['prerelease'] and leave_out_prereleases:
    continue 

  article_author = json.dumps(rel["author"]["login"])
  article_url = json.dumps(rel["html_url"])
  article_title = json.dumps(rel["tag_name"])
  article_time = json.dumps(datetime.strptime(rel["published_at"], "%Y-%m-%dT%H:%M:%SZ").isoformat())
  items.append("{{\"title\": {title}, \"authors\": [{{\"name\": {author}}}], \"content_html\": {html}, \"url\": {url}, \"date_published\": {date}}}".format(title=article_title,
                                                                                                                                                            html=article_title,
                                                                                                                                                            url=article_url,
                                                                                                                                                            date=article_time,
                                                                                                                                                            author=article_author))

json_feed = json_feed.format(title = "Releases", items = ", ".join(items))
print(json_feed)