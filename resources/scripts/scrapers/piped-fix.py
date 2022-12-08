# Fixes Piped ATOM feeds.
#
# You must provide raw ATOM feed XML data as input, for example with curl:
#   curl 'https://pipedapi.kavin.rocks/feed/unauthenticated/rss?channels=UCXuqSBlHAE6Xw-yeJA0Tunw' | python ./piped-fix.py

import json
import sys
import urllib.request
import xml.etree.ElementTree as ET

# Globals.
atom_ns = {"atom": "http://www.w3.org/2005/Atom"}

def main():
  sys.stdin.reconfigure(encoding="utf-8")
  feed_data = sys.stdin.read()
  feed_document = ET.fromstring(feed_data)

  # Prepare elements.
  elem_title = feed_document.find("atom:title", atom_ns)
  elem_description = feed_document.find("atom:subtitle", atom_ns)
  elem_title = feed_document.find("atom:title", atom_ns)
  elem_icon = ET.SubElement(feed_document, ET.QName(atom_ns["atom"], "icon"))

  # Find channel ID in the source XML feed and extract it out.
  channel_id = feed_document.find(".//atom:uri", atom_ns).text
  channel_id = channel_id[channel_id.index("channel/") + 8:]

  # Fetch data with Piped API.
  api_url = "https://pipedapi.kavin.rocks/channel/{}".format(channel_id)
  api_request = urllib.request.Request(api_url)

  api_request.add_header("User-Agent", "curl")
  api_response = urllib.request.urlopen(api_request)
  api_data =  api_response.read().decode("utf-8")

  # Fill data into feed.
  api_json = json.loads(api_data)
  elem_title.text = api_json["name"]
  elem_description.text = api_json["description"]
  elem_icon.text = api_json["avatarUrl"]

  print(ET.tostring(feed_document).decode())

if __name__ == '__main__':
  main()