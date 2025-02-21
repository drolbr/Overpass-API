#!/usr/bin/env bash

rm -f node.$1.txt

for i in $(ls sample.$1*.txt); do
  echo "$i" >>node.$1.txt
  while read -r LINE; do
    echo "$LINE" | ./de_escape_json | ../bin/osm3s_query --concise | grep -vE 'generator' | md5sum >>node.$1.txt
  done <$i
done

echo "done" >>node.$1.txt
