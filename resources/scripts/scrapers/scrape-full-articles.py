# Downloads full (HTML) articles for ATOM or RSS 2.0 feed and replaces original articles.
#
# Make sure to have all dependencies installed:
#   pip3 install asyncio (if using parallel version of the script)
#
# You must provide raw ATOM or RSS 2.0 UTF-8 feed XML data as input, for example with curl:
#   curl 'http://rss.cnn.com/rss/edition.rss' | python ./scrape-full-articles.py "4"
#
# You must provide three command line arguments:
#   scrape-full-articles.py [NUMBER-OF-PARALLEL-THREADS]

import json
import sys
import urllib.request
import xml.etree.ElementTree as ET

# Globals.
atom_ns = {"atom": "http://www.w3.org/2005/Atom"}
article_parser_url = "https://extract-article.deta.dev/?url="

# Methods.
def process_article(article, is_rss, is_atom):
  try:
    # Extract link.
    scraped_article = ""

    if is_rss:
      article_link = article.find("link").text
    elif is_atom:
      article_link = article.find("atom:link", atom_ns).attrib['href']

    # Scrape with article-parser.
    link = article_parser_url + article_link

    response = urllib.request.urlopen(link)
    text = response.read().decode("utf-8")
    js = json.loads(text)

    if int(js["error"]) == 0:
      scraped_article = js["data"]["content"]

    # Save scraped data.
    if scraped_article:
      if is_rss:
        article.find("description").text = scraped_article
      elif is_atom:
        at_con = article.find("atom:content", atom_ns)

        if at_con is None:
          article.find("atom:summary", atom_ns).text = scraped_article
        else:
          at_con.text = scraped_article
  except Exception as e:
    pass
    #print(e)


def main():
  no_threads = int(sys.argv[1]) if len(sys.argv) >= 2 else 1

  if no_threads > 1:
    import asyncio
    from concurrent.futures import ThreadPoolExecutor

  sys.stdin.reconfigure(encoding="utf-8")

  #feed_data = urllib.request.urlopen("http://feeds.hanselman.com/ScottHanselman").read()
  feed_data = sys.stdin.read()
  feed_document = ET.fromstring(feed_data)

  # Determine feed type.
  is_rss = feed_document.tag == "rss"
  is_atom = feed_document.tag == "{http://www.w3.org/2005/Atom}feed"

  if not is_rss and not is_atom:
    sys.exit("Passed file is neither ATOM nor RSS 2.0 feed.")

  # Extract articles.
  if is_rss:
    feed_articles = feed_document.findall(".//item")
  elif is_atom:
    feed_articles = feed_document.findall(".//atom:entry", atom_ns)

  # Scrape articles.
  if no_threads > 1:
    with ThreadPoolExecutor(max_workers=no_threads) as executor:
      futures = []
      for article in feed_articles:
        futures.append(
            executor.submit(process_article, article, is_rss, is_atom))
      for future in futures:
        future.result()
  else:
    for article in feed_articles:
      process_article(article, is_rss, is_atom)

  print(ET.tostring(feed_document).decode())


if __name__ == '__main__':
  main()
