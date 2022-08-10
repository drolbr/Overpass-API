#!/usr/bin/env bash

{ git log | head -n 1 | awk '{ print $2; }'; cat ../src/overpass_api/core/settings.cc; } | awk -f patch_settings.awk >_
mv _ ../src/overpass_api/core/settings.cc

VERSION=$(cat ../src/overpass_api/core/settings.cc | grep -E '^ *version' | awk '{ print substr($1,10,length($1)-12); }')

{ echo "$VERSION"; cat ../src/configure.ac; } | awk -f patch_configure_ac.awk >_
mv ../src/configure.ac ../src/configure.test.ac
mv _ ../src/configure.ac

cat ../src/Makefile.am | awk '{ if ($1 == "distdir") print $1" = osm-3s_v'$VERSION'"; else print $0; }' >_
mv ../src/Makefile.am ../src/Makefile.test.am
mv _ ../src/Makefile.am

git commit -a -m "Automated commit for release $VERSION"

cat ../src/Makefile.am | awk '{ if ($1 == "#SUBDIRS" && $3 == "") print substr($0,2); else if ($1 == "SUBDIRS" && $3 != "") print "#"$0; else print $0; }' >_
mv _ ../src/Makefile.am

cat ../src/configure.ac | awk '{ if (substr($1,1,16) == "#AC_CONFIG_FILES" && $2 == "") print substr($0,2); else if (substr($1,1,15) == "AC_CONFIG_FILES" && $2 != "") print "#"$0; else print $0; }' >_
mv _ ../src/configure.ac

pushd ../build
make dist
popd
