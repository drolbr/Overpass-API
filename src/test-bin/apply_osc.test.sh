#!/bin/bash

if [[ -z $2  ]]; then
{
  echo Usage: $0 Temp-Dir Database-Dir
  exit 0
};
fi

TEMP_DIR=$1
DATABASE_DIR=$2

# create test OSM XML file
echo "<osm>" >$TEMP_DIR/apply_osc_test.osm
echo "" >>$TEMP_DIR/apply_osc_test.osm
I=1
while [[ $I -le 10 ]]; do
{
  J=45
  while [[ $J -le 54 ]]; do
  {
    echo "<node id=\"$I$J\" lat=\"$J\" lon=\"$(($I - 2))\">" >>$TEMP_DIR/apply_osc_test.osm
    echo "  <tag k=\"lat\" v=\"$J\"/>" >>$TEMP_DIR/apply_osc_test.osm
    echo "  <tag k=\"lon\" v=\"$(($I - 2))\"/>" >>$TEMP_DIR/apply_osc_test.osm
    echo "</node>" >>$TEMP_DIR/apply_osc_test.osm
    J=$(($J + 1))
    echo "<node id=\"$I$J\" lat=\"$J\" lon=\"$(($I - 2))\"/>" >>$TEMP_DIR/apply_osc_test.osm
    J=$(($J + 1))
  };
  done
  I=$(($I + 1))
};
done
I=1
while [[ $I -le 10 ]]; do
{
  echo "<way id=\"$I\">" >>$TEMP_DIR/apply_osc_test.osm
  J=47
  while [[ $J -le 54 ]]; do
  {
    echo "  <nd ref=\"$I$J\"/>" >>$TEMP_DIR/apply_osc_test.osm
    J=$(($J + 1))
  };
  done
  echo "</way>" >>$TEMP_DIR/apply_osc_test.osm
  I=$(($I + 1))
};
done
I=1
while [[ $I -le 10 ]]; do
{
  echo "<relation id=\"$I\">" >>$TEMP_DIR/apply_osc_test.osm
  echo "  <member type=\"node\" ref=\"${I}45\"/>" >>$TEMP_DIR/apply_osc_test.osm
  echo "  <member type=\"node\" ref=\"${I}46\"/>" >>$TEMP_DIR/apply_osc_test.osm
  echo "  <member type=\"way\" ref=\"$I\"/>" >>$TEMP_DIR/apply_osc_test.osm
  echo "</relation>" >>$TEMP_DIR/apply_osc_test.osm
  I=$(($I + 1))
};
done
echo "" >>$TEMP_DIR/apply_osc_test.osm
echo "</osm>" >>$TEMP_DIR/apply_osc_test.osm

# create database and dump
./complete_updater --db-dir=$DATABASE_DIR <$TEMP_DIR/apply_osc_test.osm

mkdir -p $TEMP_DIR/osc/
echo -e "\
<osm-change>

<node id=\"250\" lat=\"50\" lon=\"-0.5\"/>
<delete>
  <node id=\"345\"/>
  <node id=\"445\"/>
</delete>
<node id=\"446\" lat=\"46.01\" lon=\"2.01\"/>
<node id=\"448\" lat=\"48.05\" lon=\"2.05\"/>
<node id=\"947\" lat=\"47\" lon=\"7\">
  <tag k=\"lat\" v=\"fourty-seven\"/>
  <tag k=\"lon_spoken\" v=\"seven\"/>
</node>
<node id=\"948\" lat=\"48.01\" lon=\"7\"/>
<node id=\"2000\" lat=\"55.55\" lon=\"11.11\"/>
<node id=\"2005\" lat=\"55.11\" lon=\"11.55\"/>
<way id=\"5\">
  <nd ref=\"548\"/>
  <nd ref=\"550\"/>
  <nd ref=\"552\"/>
  <nd ref=\"554\"/>
</way>
<way id=\"6\">
  <nd ref=\"647\"/>
  <nd ref=\"649\"/>
  <nd ref=\"651\"/>
  <nd ref=\"653\"/>
</way>
<delete>
  <way id=\"7\"/>
  <way id=\"8\"/>
</delete>
<way id=\"11\">
  <nd ref=\"147\"/>
  <nd ref=\"447\"/>
  <nd ref=\"747\"/>
  <nd ref=\"1047\"/>
</way>

</osm-change>\
" >$TEMP_DIR/osc/apply_osc_test.1.osc

echo -e "\
<osm-change>

<node id=\"445\" lat=\"45.01\" lon=\"2.01\">
  <tag k=\"lat\" v=\"45\"/>
  <tag k=\"lon\" v=\"2\"/>
</node>
<delete>
  <node id=\"446\"/>
</delete>
<node id=\"448\" lat=\"48.01\" lon=\"2.01\"/>
<delete>
  <node id=\"2000\"/>
  <node id=\"2001\"/>
</delete>
<way id=\"5\">
  <nd ref=\"547\"/>
  <nd ref=\"549\"/>
  <nd ref=\"551\"/>
  <nd ref=\"553\"/>
</way>
<delete>
  <way id=\"6\"/>
</delete>
<way id=\"7\">
  <nd ref=\"747\"/>
  <nd ref=\"749\"/>
  <nd ref=\"751\"/>
  <nd ref=\"753\"/>
</way>
<delete>
  <way id=\"21\"/>
</delete>

</osm-change>\
" >$TEMP_DIR/osc/apply_osc_test.2.osc

../bin/apply_osc --db-dir=$DATABASE_DIR --osc-dir=$TEMP_DIR/osc/

./dump_database --db-dir=$DATABASE_DIR
