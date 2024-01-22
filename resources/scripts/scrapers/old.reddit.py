import json
import sys
import time
import urllib.request
import http.cookies
import bs4

number_of_pages = 6
subreddit = sys.argv[1]
url_base = "https://old.reddit.com/r/{}".format(subreddit)
json_feed = '{{"title": "Old.Reddit - {subr}", "items": [{items}]}}'
items = list()

last_item_id = None

for page_number in range(1, number_of_pages + 1):
    if page_number == 1:
        page_url = url_base
    else:
        page_url = url_base + "?after={}".format(last_item_id)
    
    page_request = urllib.request.Request(page_url)
    page_request.add_header("User-Agent", "curl/8.4.0")
    page_response = urllib.request.urlopen(page_request)
    page_html = page_response.read().decode("utf-8")
    
    soup = bs4.BeautifulSoup(page_html, "html.parser")
    entries = soup.css.select("[class=thing],[data-fullname^=t3_]")
   
    for entry in entries:
        title = entry.css.select_one(".title").text
        html = str(entry)
        url = entry.css.select_one("a[href]")["href"]
        author = entry.css.select_one("a[class^=author]").text
        publ = entry.css.select_one("time")["datetime"]
        last_item_id = entry["data-fullname"]
        
        item = '{{"title": {title}, "id": {url}, "authors": [{{"name": {author}}}], "date_published": {publ}, "content_html": {html}, "url": {url}}}'.format(
            title=json.dumps(title),
            html=json.dumps(html),
            url=json.dumps(url),
            author=json.dumps(author),
            publ=json.dumps(publ)
        )
        items.append(item)
        
    time.sleep(10)

json_feed = json_feed.format(subr=subreddit, items=", ".join(items))
print(json_feed)
