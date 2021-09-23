# Translates entries of RSS 2.0 feed into different locale.
#
# Make sure to have all dependencies installed:
#   pip3 install googletrans
#   pip3 install asyncio (if using parallel version of the script)
#   pip3 install hyper (for HTTP/2 support, much faster than default)
#
# You must provide raw RSS 2.0 UTF-8 feed XML data as input, for example with curl:
#   curl 'https://phys.org/rss-feed/' | python ./translate-feed.py "en" "pt_BR" "true"
#
# You must provide three command line arguments:
#   translate-feed.py [FROM-LANGUAGE] [TO-LANGUAGE] [RUN-PARALLEL] [FEED-ENCODING (optional)]

import json
import re
import io
import sys
import time
import html
import requests
import distutils.util
import xml.etree.ElementTree as ET
import itertools as IT
from googletrans import Translator
from bs4 import BeautifulSoup

lang_from = sys.argv[1]
lang_to = sys.argv[2]
parallel = bool(distutils.util.strtobool(sys.argv[3]))

if (len(sys.argv) >= 5):
  src_enc = sys.argv[4]
else:
  src_enc = "utf-8"

if parallel:
  import asyncio
  from concurrent.futures import ThreadPoolExecutor

sys.stdin.reconfigure(encoding = src_enc)
rss_data = sys.stdin.read()

#print(rss_data)

try:
  rss_document = ET.fromstring(rss_data)
except ET.ParseError as err:
  lineno, column = err.position
  line = next(IT.islice(io.StringIO(rss_data), lineno))
  caret = '{:=>{}}'.format('^', column)
  err.msg = '{}\n{}\n{}'.format(err, line, caret)
  raise 

translator = Translator()

atom_ns = {"ns": "http://www.w3.org/2005/Atom"}

def translate_string(to_translate):
  try:
    if to_translate is None:
      return to_translate

    translated_text = translator.translate(to_translate, src = lang_from, dest = lang_to)

    if not parallel:
      time.sleep(0.2)

    return translated_text.text
  except:
    return to_translate

def process_article(article):
  title = article.find("title")

  if title is None:
    title = article.find("ns:title", atom_ns)

  if title is not None:
    title.text = translate_string(title.text)

  # RSS.
  contents = article.find("description")

  if contents is None:
    # ATOM.
    contents = article.find("ns:content", atom_ns)

  if contents is not None:
    htmll = "<div>{}</div>".format(contents.text)

    soup = BeautifulSoup(htmll, features = "lxml")
    contents.text = translate_string(soup.get_text())
    contents.text = contents.text.replace("\n", "<br/>")

# Translate title.
# RSS.
channel = rss_document.find("channel")

if channel is not None:
  title = channel.find("title")

if (channel is None) or (title is None):
  # ATOM.
  title = rss_document.find("ns:title", atom_ns)

if title is not None:
  title.text = translate_string(title.text)

# Translate articles.
if parallel:
  with ThreadPoolExecutor(max_workers = 2) as executor:
    futures = []
    for article in rss_document.findall(".//item"):
      futures.append(executor.submit(process_article, article))
    for article in rss_document.findall(".//ns:entry", atom_ns):
      futures.append(executor.submit(process_article, article))
    for future in futures:
      future.result()
else:
  for article in rss_document.findall(".//item"):
    process_article(article)
  for article in rss_document.findall(".//ns:entry", atom_ns):
    process_article(article)

out_xml = ET.tostring(rss_document)
out_decoded_xml = out_xml.decode()

print(out_decoded_xml)
