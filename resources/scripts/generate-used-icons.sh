#!/bin/bash

# This script will generate .qrc file from used icons.
#
# PWD must be the root of repository.
# Output qrc file has paths relative to "resources" folder.

append_icon_theme_files() {
  local -n ICON_NAMES_TO_FIND=$1
  local -n ALREADY_FOUND_ICONS=$2
  local ICONS_BASE_FOLDER="$3"
  local ICON_THEME_NAME="$4"

  local ICON_THEME_FOLDER="$ICONS_BASE_FOLDER/$ICON_THEME_NAME"

  # Find relevant icons.
  for ICON_NAME in "${ICON_NAMES_TO_FIND[@]}"; do
    local ICON=$(find "$ICON_THEME_FOLDER" -name "${ICON_NAME}.*" -printf "%P\n" | tail -1)
    
    if [ -n "$ICON" ]; then
      ALREADY_FOUND_ICONS+=("./$(basename "$ICONS_BASE_FOLDER")/$ICON_THEME_NAME/$ICON")
    fi
  done

  # Append index file.
  ALREADY_FOUND_ICONS+=("./$(basename "$ICONS_BASE_FOLDER")/$ICON_THEME_NAME/index.theme")
}

main() {
  local ROOT_FOLDER="$(pwd)"
  local ROOT_SRC_FOLDER="$(pwd)/src/librssguard"
  local RESOURCES_FOLDER="$ROOT_FOLDER/resources"
  local THEMES_FOLDER="$RESOURCES_FOLDER/graphics"

  local FOUND_ICON_FILES
  local ICON_THEMES=("Breeze" "Breeze Dark")
  local ICON_NAMES=($(pcregrep.exe -r -h --om-separator=$'\n' -o1 -o2 '.+fromTheme\(QSL\(\"([\_\-\+a-z]+)\"\)(?:, QSL\(\"([\_\-\+a-z]+)\"\))?' "$ROOT_SRC_FOLDER" | sort -u))

  #cd $ROOT_SRC_FOLDER
  #echo "${ICON_NAMES[@]}"

  # Build list of relative paths to individual icons.
  for THEME_NAME in "${ICON_THEMES[@]}"; do
    append_icon_theme_files ICON_NAMES FOUND_ICON_FILES "$THEMES_FOLDER" "$THEME_NAME"
  done

  # Generate final XML and print it.
  printf "<RCC>\n  <qresource prefix=\"/\">\n"

  for ICON_FILE in "${FOUND_ICON_FILES[@]}"; do
    echo "    <file>$ICON_FILE</file>"
  done

  printf "  </qresource>\n</RCC>"

  #declare -p FOUND_ICON_FILES
}

main