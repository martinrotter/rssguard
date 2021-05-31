#!/bin/bash

# Transka executable.
TRANSKA=./transka

# Get credentials.
read -p "Username: " USERNAME
read -p "Password: " PASSWORD

# Setup parameters.
RESOURCE=../../../localization/rssguard_en.ts
CODES="cs da de en_GB es fi fr gl he id it ja lt nl pl pt_BR pt_PT ru sv uk zh_CN zh_TW"
TRANSLATION='../../../localization/rssguard_$CODE.ts'

declare PARAMS

PARAMS+="-u "$USERNAME" -p "$PASSWORD" -ps "rssguard" -rs "rssguard" -ur "$RESOURCE" "

for CODE in $CODES; do
  PARAMS+="-dt "$CODE" "$(eval echo $TRANSLATION)" "
done

cd ./transka

$TRANSKA $PARAMS
