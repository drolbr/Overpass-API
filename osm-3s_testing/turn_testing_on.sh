#!/usr/bin/env bash

cat ../src/Makefile.am | awk '{ if ($1 == "#SUBDIRS" && $3 != "") print substr($0,2); else if ($1 == "SUBDIRS" && $3 == "") print "#"$0; else print $0; }' >_
mv _ ../src/Makefile.am

cat ../src/configure.ac | awk '{ if (substr($1,1,16) == "#AC_CONFIG_FILES" && $2 != "") print substr($0,2); else if (substr($1,1,15) == "AC_CONFIG_FILES" && $2 == "") print "#"$0; else print $0; }' >_
mv _ ../src/configure.ac
