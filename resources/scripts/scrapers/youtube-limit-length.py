# Use this as RSS Guard post-processing script.
#
# Example command line usage:
#   curl 'PATH_TO_SOME_YOUTUBE_CHANNEL_RSS_FEED' | python 'youtube-limit-length.py'

import sys
import xml.etree.ElementTree as ET
import requests
import json
import isodate

sys.stdin.reconfigure(encoding="utf-8")
input_data = sys.stdin.read()

api_key = "YOUR_API_KEY_HERE"
min_required_length = 90  # In seconds. 

youtube_query = (
    "https://www.googleapis.com/youtube/v3/videos?part=contentDetails&id=VIDEO-ID&key="
    + api_key
)
ns = {
    "atom": "http://www.w3.org/2005/Atom",
    "yt": "http://www.youtube.com/xml/schemas/2015",
}

tree = ET.fromstring(input_data)
entries = tree.findall("atom:entry", ns)

for entry in entries:
    video_id = entry.find("yt:videoId", ns).text
    youtube_query_specific = youtube_query.replace("VIDEO-ID", video_id)
    api_response = requests.get(youtube_query_specific).text
    response_json = json.loads(api_response)
    
    try:
        length_encoded = str(response_json["items"][0]["contentDetails"]["duration"])
    except:
        continue
           
    length = isodate.parse_duration(length_encoded)

    if length.seconds < min_required_length:
        tree.remove(entry)

sys.stdout.buffer.write(ET.tostring(tree, encoding='utf-8', method='xml'))