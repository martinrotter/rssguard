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

    if not quote_author:
      continue

    quote_text = "<span>" + lst.find("li").decode_contents() + "</span>"
    quote_heading = lst.find("li")
    quote_heading = (quote_heading.get_text()[:75] + '...') if len(quote_heading) > 75 else quote_heading.get_text()
    quote_heading = quote_heading.split(" ~")[0]

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