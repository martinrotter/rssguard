# Embeds GIFs into individual messages from Funnyjunk.com
# Sample input file whose contents must be provided as stdin: "https://funnyjunk.com/rss/most_popular.rss"

import json
import re
import sys
import urllib.request
import xml.etree.ElementTree as ET
from datetime import datetime

input_data = sys.stdin.read()

tree = ET.fromstring(input_data)
pattern = re.compile("href=\"(https://[^<>]+\.(gif|png|jpg))\"")

for ite in tree.find("channel").iter("item"):
  link = ite.find("link").text
  #print(link)
  response = urllib.request.urlopen(link)
  text =  response.read().decode("utf-8")
  for pic_link in re.findall(pattern, text):
    new = ET.SubElement(ite, "enclosure")
    new.set("url", pic_link[0])
    new.set("type", "image/" + pic_link[1])
  #print(ET.tostring(ite, encoding="unicode"))

print(ET.tostring(tree, encoding="unicode"))