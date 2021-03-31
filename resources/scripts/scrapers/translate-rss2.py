# Translates entries of RSS 2.0 feed into different locale.
#
# Make sure to have all dependencies installed:
#   pip3 install googletrans==4.0.0-rc1
#   pip3 install asyncio (if using parallel version of the script)
#
# You must provide raw RSS 2.0 UTF-8 feed XML data as input, for example with curl:
#   curl 'https://phys.org/rss-feed/' | python ./translate-rss2.py "en" "pt_BR" "true"
#
# You must provide three command line arguments:
#   translate-rss2.py [FROM-LANGUAGE] [TO-LANGUAGE] [RUN-PARALLEL]

import json
import re
import sys
import time
import html
import requests
import distutils.util
import xml.etree.ElementTree as ET
from googletrans import Translator

lang_from = sys.argv[1]
lang_to = sys.argv[2]
parallel = bool(distutils.util.strtobool(sys.argv[3]))

if parallel:
  import asyncio
  from concurrent.futures import ThreadPoolExecutor

sys.stdin.reconfigure(encoding='utf-8')
rss_data = sys.stdin.read()
rss_document = ET.fromstring(rss_data)
translator = Translator()

def translate_string(to_translate):
  try:
    translated_text = translator.translate(to_translate, src = lang_from, dest = lang_to)

    if not parallel:
      time.sleep(0.2)

    return translated_text.text
  except:
    return to_translate

def process_article(article):
  title = article.find("title")
  title.text = translate_string(title.text)

  contents = article.find("description")
  contents.text = translate_string(" ".join(contents.itertext()))

if parallel:
  with ThreadPoolExecutor(max_workers = 2) as executor:
    futures = []
    for article in rss_document.findall(".//item"):
      futures.append(executor.submit(process_article, article))
    for future in futures:
      future.result()
else:
  for article in rss_document.findall(".//item"):
    process_article(article)

print(ET.tostring(rss_document, encoding = "unicode"))