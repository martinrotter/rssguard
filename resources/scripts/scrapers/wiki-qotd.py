# Downloads all quotes of the day.

import urllib.request
import json
from bs4 import BeautifulSoup

url = "https://en.wikiquote.org/wiki/Wikiquote:Quote_of_the_day/Complete_list"
response = urllib.request.urlopen(url)
text =  response.read().decode("utf-8")

soup = BeautifulSoup(text, 'html.parser')
lists = soup.find_all("ul")
items = list()
json_feed = "{{\"title\": {title}, \"items\": [{items}]}}"

for lst in lists:
  try:
    last_link = lst.find_all("a")[-1]
    quote_link = last_link.get("href")

    if quote_link.startswith("/"):
      quote_link = "https://en.wikiquote.org" + quote_link

    quote_author = last_link.get_text()
    quote_text = lst.find("li").decode_contents()
    quote_heading = (quote_text[:75] + '...') if len(quote_text) > 75 else quote_text
    quote_text = "<span>" + quote_text + "</span>"

    items.append("{{\"title\": {title}, \"authors\": [{{\"name\": {author}}}], \"content_html\": {html}, \"url\": {url}, \"date_published\": {date}}}".format(
      title = json.dumps(quote_heading),
      html = json.dumps(quote_text),
      url = json.dumps(quote_link),
      author = json.dumps(quote_author),
      date = json.dumps("2020-12-31T08:00:00")))
  except:
    continue

json_feed = json_feed.format(title = json.dumps(soup.title.text), items = ", ".join(items))
print(json_feed)