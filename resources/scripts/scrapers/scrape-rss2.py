# Downloads full articles for RSS 2.0 feed and replaces original articles.
#
# Make sure to have all dependencies installed:
#   pip3 install newspaper3k
#   pip3 install asyncio (if using parallel version of the script)
#
# You must provide raw RSS 2.0 UTF-8 feed XML data as input, for example with curl:
#   curl 'http://rss.cnn.com/rss/edition.rss' | python ./scrape-rss2.py "4"
#
# You must provide three command line arguments:
#   scrape-rss2.py  [NUMBER-OF-PARALLEL-THREADS]

import json
import re
import sys
import time
import html
import requests
import distutils.util
import xml.etree.ElementTree as ET
from newspaper import Article

no_threads = int(sys.argv[1])

if no_threads > 1:
  import asyncio
  from concurrent.futures import ThreadPoolExecutor

sys.stdin.reconfigure(encoding='utf-8')
rss_data = sys.stdin.read()
rss_document = ET.fromstring(rss_data)

def process_article(article):
  try:
    link = article.find("link").text

    f = Article(link, keep_article_html = True)
    f.download()
    f.parse()
    article.find("description").text = f.article_html
  except:
    pass

# Scrape articles.
if no_threads > 1:
  with ThreadPoolExecutor(max_workers = no_threads) as executor:
    futures = []
    for article in rss_document.findall(".//item"):
      futures.append(executor.submit(process_article, article))
    for future in futures:
      future.result()
else:
  for article in rss_document.findall(".//item"):
    process_article(article)

print(ET.tostring(rss_document, encoding = "unicode"))