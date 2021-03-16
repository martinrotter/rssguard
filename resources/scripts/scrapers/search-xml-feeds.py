# Produces the list of links to XML feeds as extracted from input list of generic URLs.
# This script expects to have the file path passed as the only input parameter

import re
import sys
import urllib.request
from urllib.parse import urljoin

urls_file = sys.argv[1]

with open(urls_file) as f:
  urls_lines = [line.rstrip() for line in f]

regexp_link = re.compile("<link[^>]+type=\"application\/(?:atom\+xml|rss\+xml|feed\+json|json)\"[^>]*>")
regexp_href = re.compile("href=\"([^\"]+)\"")

for url in urls_lines:
  # Download HTML data.
  try:
    url_response = urllib.request.urlopen(url)
    html =  url_response.read().decode("utf-8")
  except:
    continue

  # Search for XML feeds with regexps.
  for link_tag in re.findall(regexp_link, html):
    for link_xml_feed in re.findall(regexp_href, link_tag):
      if link_xml_feed.startswith("/"):
        print(urljoin(url, "/") + link_xml_feed[1:])
      else:
        print(link_xml_feed)