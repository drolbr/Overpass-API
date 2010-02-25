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
<union> \
  <id-query type=\"relation\" ref=\"$ARG_1\"/> \
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

echo "Content-Type: text/html; charset=utf-8"
echo

if [[ $ARG_2 == "forward" ]]; then
{
  gunzip </tmp/nodes_csv_result.2 | ../bin/nodes-csv-lat-lon-name --forward
};
fi
if [[ $ARG_2 == "backward" ]]; then
{
  gunzip </tmp/nodes_csv_result.2 | ../bin/nodes-csv-lat-lon-name --backward
};
fi
if [[ $ARG_2 == "all" ]]; then
{
  gunzip </tmp/nodes_csv_result.2 | ../bin/nodes-csv-lat-lon-name
};
fi
