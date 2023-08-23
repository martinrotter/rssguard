# Provides filtering of entries provided via Metacritic RSS feeds.
#
# Example input feed is: https://www.metacritic.com/rss/tv
#
# This script expects raw RSS 2.0 feed data as input and can be called like
# this on cli:
#   curl 'https://www.metacritic.com/rss/tv' | python ./metacritic.py "<MINIMUM-SCORE>"
#
# Replace <MINIMUM-SCORE> with minimal numerical score your articles must have or pass nothing
# to keep all articles.

import json
import sys
import urllib.request
import xml.etree.ElementTree as ET
import re
import ssl

def get_score_of_url(article_url):
  # Download HTML of article.
  req = urllib.request.Request(article_url)
  req.add_header("Accept", "*/*")
  req.add_header("User-Agent", "curl/7.55.1")
  response = urllib.request.urlopen(req, context=ssl.SSLContext())
  text = response.read().decode("utf-8")
  score = int(re.search(r'(metascore_w larger tvshow positive|ratingValue)">(\d{1,2})', text).group(2))
  return score

def main():
  minimal_score = int(sys.argv[1]) if len(sys.argv) >= 2 else -1

  # Read RSS 2.0 feed data from input.
  sys.stdin.reconfigure(encoding="utf-8")

  #req = urllib.request.Request("https://www.metacritic.com/rss/tv")
  #req.add_header("Accept", "*/*")
  #req.add_header("User-Agent", "curl/7.55.1")
  #feed_data = urllib.request.urlopen(req, context=ssl.SSLContext()).read()

  feed_data = sys.stdin.read()
  feed_document = ET.fromstring(feed_data)

  # Process articles one by one.
  feed_channel = feed_document.find(".//channel")
  feed_articles = feed_channel.findall("item")

  for article in feed_articles:
    try:
      article_score = get_score_of_url(article.find("link").text)
    except:
      article_score = minimal_score - 1
      pass

    if article_score < minimal_score:
      feed_channel.remove(article)
    else:
      article_title = article.find("title")
      article_title.text += " - {}".format(article_score)

  out_xml = ET.tostring(feed_document)
  out_decoded_xml = out_xml.decode()
  
  print(out_decoded_xml)

if __name__ == '__main__':
  main()