#!/bin/bash

# Transka executable.
TRANSKA=./transka

# Get credentials.
read -p "Username: " USERNAME
read -p "Password: " PASSWORD

# Setup parameters.
RESOURCE=../../../localization/rssguard-en_GB.ts
CODES="cs_CZ da_DK de_DE en_US fr_FR he_IL id_ID it_IT ja_JP lt_LT nl_NL pt_BR sv_SE"
TRANSLATION='../../../localization/rssguard-$CODE.ts'

declare PARAMS

PARAMS+="-u "$USERNAME" -p "$PASSWORD" -ps "rssguard" -rs "rssguard" -ur "$RESOURCE" "

for CODE in $CODES; do
  PARAMS+="-dt "$CODE" "$(eval echo $TRANSLATION)" "
done

cd ./transka

$TRANSKA $PARAMS