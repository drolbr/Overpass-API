#!/usr/bin/env bash

for i in \
  if.h \
  for.h \
  complete.h \
  retro.h \
  compare.h \
  print.h \
  timeline.h \
  localize.h \
  filter.h \
  convert.h \
  make.h \
  evaluator.h \
  tag_value.h \
  explicit_geometry.h \
  aggregators.h \
  unary_operators.h \
  binary_operators.h \
  ternary_operator.h \
  string_endomorphisms.h \
  geometry_endomorphisms.h \
  set_list_operators.h \
  runtime_value.h \
; do
  cat ../src/overpass_api/statements/$i | awk -f extract_wiki_comments.awk
done

