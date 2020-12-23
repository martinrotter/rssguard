#!/bin/bash

# This script will generate .qrc file from used icons.
#
# PWD is used as used src root folder.
# "resources/graphics/Faenza" is used as folder for icons.
# Output qrc file has paths relative to "resources" folder.

echo_formatted_qrc() {
  printf "<RCC>\n  <qresource prefix=\"/\">\n"
  
  # Sort icons first.
  readarray -t sorted < <(for a in "$@"; do echo "$a"; done | sort)
  
  for ICON_FILE in "${sorted[@]}"; do
    # We find icon.
    echo "    <file>$ICON_FILE</file>"
  done
  
  printf "  </qresource>\n</RCC>"
}

discover_used_icons() {
  local ROOT_SRC_FOLDER="$(pwd)"
  local RESOURCES_FOLDER="$ROOT_SRC_FOLDER/../resources"
  
  local INDEX_FILE_1="./graphics/Faenza/index.theme"
  local INDEX_FILE_2="./graphics/Numix/index.theme"
  
  declare -a ICON_FILES
  #echo "Root src folder: \"$ROOT_SRC_FOLDER\"."
  
  # Now we discover all usages of icons.
  local ICON_NAMES=$(grep -Prioh '(?<=fromTheme\(QSL\(\")[-\+a-z]+' "$ROOT_SRC_FOLDER" | sort -u)
 
  cd "$RESOURCES_FOLDER"
  
  for ICON_NAME in $ICON_NAMES; do
    # We find icon.
    local ICON_FILE="$(find . -name "${ICON_NAME}.*")"
    ICON_FILES+=("$ICON_FILE")
  done
  
  ICON_FILES+=("$INDEX_FILE_1")
  ICON_FILES+=("$INDEX_FILE_2")
  cd "$ROOT_SRC_FOLDER"
  echo_formatted_qrc ${ICON_FILES[@]}
}

discover_used_icons