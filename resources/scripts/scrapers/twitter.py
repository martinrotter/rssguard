# Generates JSON feed from Twitter timeline URL.
#
# This script expects two input parameters:
#   twitter.py [twitter-user-name] [twitter-user-id]
#
# For example:
#   twitter.py 'NASA' '11348282'

import json
import re
import sys
import time
import html
import urllib.request
import requests
import distutils.util
from datetime import datetime

twitter_url = "https://twitter.com/" + sys.argv[1]
twitter_id = sys.argv[2]
twitter_username = twitter_url[twitter_url.rfind("/") + 1:]
twitter_bearer = "Bearer AAAAAAAAAAAAAAAAAAAAANRILgAAAAAAnNwIzUejRCOuH5E6I8xnZz4puTs%3D1Zv7ttfk8LF81IUq16cHjhLTvJu4FA33AGWWjCpTnA"

# Download RAW Twitter HTML data and extract token.
url_response = urllib.request.urlopen(twitter_url)
twitter_html =  url_response.read().decode("utf-8")
twitter_token = re.search("gt=(\d+);", twitter_html).group(1)

# Obtain JSON Twitter data with token.
twitter_json_url = "https://twitter.com/i/api/2/timeline/profile/{user_id}.json?include_profile_interstitial_type=1&include_blocking=1&include_blocked_by=1&include_followed_by=1&include_want_retweets=1&include_mute_edge=1&include_can_dm=1&include_can_media_tag=1&skip_status=1&cards_platform=Web-12&include_cards=1&include_ext_alt_text=true&include_quote_count=true&include_reply_count=1&tweet_mode=extended&include_entities=true&include_user_entities=true&include_ext_media_color=true&include_ext_media_availability=true&send_error_codes=true&simple_quoted_tweet=true&include_tweet_replies=false&count=50&userId={user_id}&ext=mediaStats&2ChighlightedLabel".format(user_id = twitter_id)

url_response = requests.get(twitter_json_url, headers = {
  "x-guest-token": twitter_token,
  "Authorization": twitter_bearer
})

# Convert to JSON feed.
json_data = json.loads(url_response.text)
json_root = json_data["globalObjects"]["tweets"]
json_feed = "{{\"title\": \"{title}\", \"home_page_url\": \"{url}\", \"items\": [{items}]}}"
items = list()

for ite in json_root:
  article = json_root[ite]

  if "urls" in article["entities"] and len(article["entities"]["urls"]) > 0:
    article_url = json.dumps(article["entities"]["urls"][0]["expanded_url"])
  else:
    article_url = json.dumps("")

  article_title = json.dumps(article["full_text"][:75] + (article["full_text"][75:] and '...'))
  article_time = json.dumps(datetime.strptime(article["created_at"], "%a %b %d %H:%M:%S %z %Y").isoformat())
  article_contents = json.dumps(article["full_text"])

  items.append("{{\"title\": {title}, \"authors\": [{{\"name\": {author}}}], \"content_text\": {text}, \"url\": {url}, \"date_published\": {date}}}".format(
    title = article_title,
    text = article_contents,
    author = json.dumps(twitter_username),
    url = article_url,
    date = article_time))

json_feed = json_feed.format(
  title = "twitter.com/" + twitter_username,
  url = twitter_url,
  items = ", ".join(items))
print(json_feed)