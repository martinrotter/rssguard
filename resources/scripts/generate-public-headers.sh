#!/bin/bash

# This script will generate list of public headers to be included in packages.
#
# First argument is the root folder of library of RSS Guard.

root_folder="$1"
headers_regex=".+(services/abstract).+\.h$"

find "$root_folder" -type f -regextype posix-egrep -regex "$headers_regex" | sed -e "s|^src|..|"