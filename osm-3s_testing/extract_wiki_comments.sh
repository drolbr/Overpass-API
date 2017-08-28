#!/usr/bin/env bash

for i in \
  filter.h \
  convert.h \
  make.h \
  evaluator.h \
  tag_value.h \
  aggregators.h \
  binary_operators.h \
  unary_operators.h \
  string_endomorphisms.h \
; do
  cat ../src/overpass_api/statements/$i | awk -f extract_wiki_comments.awk
done

