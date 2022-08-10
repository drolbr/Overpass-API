#!/usr/bin/env bash

cat ../src/Makefile.am | awk '{ if ($1 == "#SUBDIRS" && $3 != "") print substr($0,2); else if ($1 == "SUBDIRS" && $3 == "") print "#"$0; else print $0; }' >_
if [[ -z $(diff -q _ ../src/Makefile.test.am) ]]; then
  mv ../src/Makefile.test.am ../src/Makefile.am
else
  mv _ ../src/Makefile.am
fi

cat ../src/configure.ac | awk '{ if (substr($1,1,16) == "#AC_CONFIG_FILES" && $2 != "") print substr($0,2); else if (substr($1,1,15) == "AC_CONFIG_FILES" && $2 == "") print "#"$0; else print $0; }' >_
if [[ -z $(diff -q _ ../src/configure.test.ac) ]]; then 
  mv ../src/configure.test.ac ../src/configure.ac
else
  mv _ ../src/configure.ac
fi
