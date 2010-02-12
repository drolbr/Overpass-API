#!/bin/bash

ARG_1=`echo $QUERY_STRING | awk -F [=,\&] '{ print $1; }'`
ARG_2=`echo $QUERY_STRING | awk -F [=,\&] '{ print $2; }'`
ARG_3=`echo $QUERY_STRING | awk -F [=,\&] '{ print $3; }'`
ARG_4=`echo $QUERY_STRING | awk -F [=,\&] '{ print $4; }'`
ARG_5=`echo $QUERY_STRING | awk -F [=,\&] '{ print $5; }'`
ARG_6=`echo $QUERY_STRING | awk -F [=,\&] '{ print $6; }'`
ARG_7=`echo $QUERY_STRING | awk -F [=,\&] '{ print $7; }'`
ARG_8=`echo $QUERY_STRING | awk -F [=,\&] '{ print $8; }'`

if [[ $ARG_2 == "doubleread" ]]; then
{
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
};
elseif [[ -n $ARG_2 ]];
{
echo -e "\
<osm-script timeout=\"180\" element-limit=\"10000000\"> \
 \
<union> \
  <union> \
    <id-query type=\"relation\" ref=\"$ARG_1\"/> \
    <id-query type=\"relation\" ref=\"$ARG_2\"/> \
  </union> \
  <recurse type=\"relation-node\"/> \
</union> \
<print mode=\"body\"/> \
 \
</osm-script> \
" >/tmp/nodes_csv_req
};
else
{
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
};
fi

echo "Content-Type: image/svg+xml; charset=utf-8"
echo

REQUEST_METHOD=
/home/roland/osm-3s/cgi-bin/interpreter </tmp/nodes_csv_req >/tmp/nodes_csv_result.1
dd if=/tmp/nodes_csv_result.1 of=/tmp/nodes_csv_result.2 bs=1 skip=56
if [[ $ARG_2 == "doubleread" ]]; then
{
  gunzip </tmp/nodes_csv_result.2 | ../bin/sketch-route-svg --doubleread-rel
};
else
{
  gunzip </tmp/nodes_csv_result.2 | ../bin/sketch-route-svg
};
fi
