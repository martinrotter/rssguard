# Setup parameters.
#RESOURCE = "./localization/rssguard_en.ts"
#CODES = "cs da de en_GB en_US es fi fr gl he id it ja lt nl pl pt_BR pt_PT ru sv uk zh_CN zh_TW"
#TRANSLATION = './localization/rssguard_$CODE.ts'


import sys
import os
import urllib

from transifex.api import transifex_api
from pprint import pprint

# Read API token.
api_token = sys.argv[1]

print("API token: {}".format(api_token))

transifex_api.setup(auth = api_token)

# Constants.
org_slug = "martinrotter"
proj_slug = "rssguard"
translation_file = os.path.normpath("./localization/rssguard_{}.ts")

# Organization/project.
organization = transifex_api.Organization.get(slug = org_slug)
project = organization.fetch('projects').get(slug = proj_slug)
resource = project.fetch('resources').get(slug = proj_slug)
languages = project.fetch('languages')

# Upload resource file.
with open(translation_file.format("en"), "r", encoding = "utf-8") as file:
  print("Uploading resource...")
  
  resource_data = file.read()
  transifex_api.ResourceStringsAsyncUpload.upload(resource_data, resource = resource)

# Download translations.
for lang in languages:
  print("Downloading {} translation...".format(lang.code))

  url = transifex_api.ResourceTranslationsAsyncDownload.download(resource = resource, language = lang)
  target_path = translation_file.format(lang.code)
  urllib.request.urlretrieve(url, target_path)