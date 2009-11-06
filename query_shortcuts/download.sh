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
  <bbox-query w=\"$ARG_2\" s=\"$ARG_3\" e=\"$ARG_4\" n=\"$ARG_5\"/> \
  <recurse type=\"node-relation\" into=\"rels\"/> \
  <recurse type=\"node-way\"/> \
  <recurse type=\"way-relation\"/> \
</union> \
<union> \
  <item/> \
  <recurse type=\"way-node\"/> \
</union> \
<print mode=\"body\"/> \
 \
</osm-script> \
" >/tmp/bbox_req

REQUEST_METHOD=
/home/roland/osm-3s/cgi-bin/interpreter </tmp/bbox_req
