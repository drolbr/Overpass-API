#!/bin/bash

ARG_1=`echo $QUERY_STRING | awk -F [=,\&] '{ print $1; }'`
ARG_2=`echo $QUERY_STRING | awk -F [=,\&] '{ print $2; }'`
ARG_3=`echo $QUERY_STRING | awk -F [=,\&] '{ print $3; }'`
ARG_4=`echo $QUERY_STRING | awk -F [=,\&] '{ print $4; }'`
ARG_5=`echo $QUERY_STRING | awk -F [=,\&] '{ print $5; }'`
ARG_6=`echo $QUERY_STRING | awk -F [=,\&] '{ print $6; }'`
ARG_7=`echo $QUERY_STRING | awk -F [=,\&] '{ print $7; }'`
ARG_8=`echo $QUERY_STRING | awk -F [=,\&] '{ print $8; }'`

echo -e "\
<osm-script timeout=\"180\" element-limit=\"10000000\"> \
 \
<id-query type=\"relation\" ref=\"$ARG_1\"/> \
<print mode=\"body\"/> \
 \
</osm-script> \
" >/tmp/nodes_csv_rel_req

REQUEST_METHOD=
/home/roland/osm-3s/cgi-bin/interpreter </tmp/nodes_csv_rel_req >/tmp/nodes_csv_rel_result.1
RESPONSE_TYPE=`head -n 1 </tmp/nodes_csv_rel_result.1`
if [[ $RESPONSE_TYPE != "Content-type: application/osm3s" ]]; then
{
  cat </tmp/nodes_csv_rel_result.1
  exit 0
};
fi
dd if=/tmp/nodes_csv_rel_result.1 of=/tmp/nodes_csv_rel_result.2 bs=1 skip=56
gunzip </tmp/nodes_csv_rel_result.2 | ../bin/memberlist >/tmp/nodes_rel_csv_result.3

mkdir /tmp/nodes_csv_rels
while read -u 3 REL; do
{
  echo -e "\
  <osm-script timeout=\"180\" element-limit=\"10000000\"> \
   \
  <union> \
    <id-query type=\"relation\" ref=\"$REL\"/> \
    <recurse type=\"relation-node\"/> \
  </union> \
  <print mode=\"body\"/> \
   \
  </osm-script> \
  " >/tmp/nodes_csv_req

  REQUEST_METHOD=
  /home/roland/osm-3s/cgi-bin/interpreter </tmp/nodes_csv_req >/tmp/nodes_csv_result.1
  RESPONSE_TYPE=`head -n 1 </tmp/nodes_csv_result.1`
  if [[ $RESPONSE_TYPE != "Content-type: application/osm3s" ]]; then
  {
    cat </tmp/nodes_csv_result.1
    exit 0
  };
  fi
  dd if=/tmp/nodes_csv_result.1 of=/tmp/nodes_csv_result.2 bs=1 skip=56
  if [[ $ARG_2 == "forward" ]]; then
  {
    gunzip </tmp/nodes_csv_result.2 | ../bin/nodes-csv-lat-lon-name --forward >/tmp/nodes_csv_rels/$REL
  };
  fi
  if [[ $ARG_2 == "backward" ]]; then
  {
    gunzip </tmp/nodes_csv_result.2 | ../bin/nodes-csv-lat-lon-name --backward >/tmp/nodes_csv_rels/$REL
  };
  fi
  if [[ $ARG_2 == "all" ]]; then
  {
    gunzip </tmp/nodes_csv_result.2 | ../bin/nodes-csv-lat-lon-name >/tmp/nodes_csv_rels/$REL
  };
  fi
};
done 3</tmp/nodes_rel_csv_result.3

echo "Content-Type: application/gzip"
echo

cd /tmp/
tar cf - nodes_csv_rels/ | gzip

rm /tmp/nodes_csv_req/*
rmdir /tmp/nodes_csv_req
