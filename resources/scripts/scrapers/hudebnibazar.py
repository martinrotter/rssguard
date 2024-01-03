# Obtains listings from hudebnibazar.cz and outputs them as JSON feed.
#
# How to call: python3 hudebnibazar.cz <category> <number-of-pages>
# For example: python3 hudebnibazar.cz "elektricke-kytary/110100" 4

import urllib.request
import requests
import re
import json
import sys
import ssl
import http.client
import http.cookies
import dateparser
import bs4
import datetime
import pytz

# ssl._DEFAULT_CIPHERS = "TLS_RSA_WITH_AES_256_GCM_SHA384"
category = sys.argv[1]
number_of_pages = int(sys.argv[2])

url_base = "https://hudebnibazar.cz"
url = "{}/{}/?is=1&f=&n=vse&r=&i=50&o=datum&ign=on".format(url_base, category)
json_feed = '{{"title": "HudebniBazar - {cat}", "items": [{items}]}}'
items = list()

# To avoid TLSv1.2 errors.
ct = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
ct.set_ciphers("DEFAULT:@SECLEVEL=0")


def processListingDate(string_date: str):
    # mins = re.search("^před (\\d+) min\\.", string_date)

    # if mins is not None:
    #     return datetime.datetime.now() - datetime.timedelta(minutes=int(mins.group(1)))

    # tday = re.search("^dnes v (\\d{1,2}):(\\d{2})$", string_date)

    # if tday is not None:
    #     return datetime.datetime.today().replace(
    #         hour=int(tday.group(1)), minute=int(tday.group(2))
    #     )

    # yday = re.search("^včera v (\\d{1,2}):(\\d{2})$", string_date)

    # if yday is not None:
    #     return (datetime.datetime.today() - datetime.timedelta(days=1)).replace(
    #         hour=int(yday.group(1)), minute=int(yday.group(2))
    #     )

    dy = dateparser.parse(string_date, languages=["cs"]).replace(second=0, microsecond=0)
    local = pytz.timezone("Europe/Prague")
    return local.localize(dy).astimezone(pytz.utc)



def processListingImgs(listing: bs4.Tag):
    pics_elems = listing.find("div", class_="InzeratObr")
    pics = list()
    
    if pics_elems is None:
        return pics
    
    for pic in pics_elems.find_all("a"):
        pics.append(url_base + pic.get("href"))

    return pics


def generateListingJson(listing: bs4.Tag):
    article_price = listing.find(class_="InzeratCena").contents[0].get_text(strip=True)
    article_title = listing.find(class_="InzeratNadpis").b.get_text(strip=True)
    article_date = listing.find(class_="InzeratZarazeno").get_text(strip=True)

    article_parsed_date = processListingDate(article_date)
    article_imgs = processListingImgs(listing)

    if article_imgs.count == 0:
        article_attachments = ""
    else:
        article_attachments = ", ".join(
            ['{{"url": {}, "mime_type": "image/jpeg"}}'.format(json.dumps(i)) for i in article_imgs]
        )

    article_url = json.dumps(url_base + listing.find("a", recursive=False)["href"])
    article_fulltitle = json.dumps("[{}] {}".format(article_price, article_title))
    article_html = json.dumps(listing.find(class_="InzeratText").get_text(strip=True))
    article_author = json.dumps(
        listing.find(lambda tag: tag.name == "div" and not tag.attrs).get_text(
            strip=True
        )
    )
    article_fulldate = json.dumps(article_parsed_date.isoformat())

    return '{{"title": {title}, "id": {url}, "attachments": [{att}], "authors": [{{"name": {author}}}], "date_published": {publ}, "content_html": {html}, "url": {url}}}'.format(
        title=article_fulltitle,
        html=article_html,
        url=article_url,
        author=article_author,
        publ=article_fulldate,
        att=article_attachments,
    )


php_ssid = None

for page_number in range(1, number_of_pages + 1):
    page_url = url + "&p={}".format(page_number)
    page_request = urllib.request.Request(page_url)
    page_request.add_header("User-Agent", "curl/8.4.0")

    if php_ssid is not None:
        page_request.add_header("Cookie", "PHPSESSID={};".format(php_ssid))

    page_response = urllib.request.urlopen(page_request, context=ct)
    page_html = page_response.read().decode("utf-8")

    if php_ssid is None:
        cook = http.cookies.SimpleCookie()
        cook.load(page_response.getheader("Set-Cookie"))
        php_ssid = cook.get("PHPSESSID").value

    soup = bs4.BeautifulSoup(page_html, "html.parser")
    listings = soup.find_all("div", class_="InzeratBody")

    for listing in listings:
        items.append(generateListingJson(listing))

json_feed = json_feed.format(cat=category, items=", ".join(items))
print(json_feed)
