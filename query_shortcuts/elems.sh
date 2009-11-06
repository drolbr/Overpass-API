#!/bin/bash

ARG_1=`echo $QUERY_STRING | awk -F [=,\&] '{ print $1; }'`
ARG_2=`echo $QUERY_STRING | awk -F [=,\&] '{ print $2; }'`
ARG_3=`echo $QUERY_STRING | awk -F [=,\&] '{ print $3; }'`
ARG_4=`echo $QUERY_STRING | awk -F [=,\&] '{ print $4; }'`
ARG_5=`echo $QUERY_STRING | awk -F [=,\&] '{ print $5; }'`
ARG_6=`echo $QUERY_STRING | awk -F [=,\&] '{ print $6; }'`
ARG_7=`echo $QUERY_STRING | awk -F [=,\&] '{ print $7; }'`
ARG_8=`echo $QUERY_STRING | awk -F [=,\&] '{ print $8; }'`

REF=`date "+%s"`

echo -e "\
data=<osm-script timeout=\"180\" element-limit=\"10000000\"> \
 \
<query type=\"$ARG_1\"> \
  <has-kv k=\"$ARG_2\" v=\"$ARG_3\"/> \
</query> \
<print mode=\"bodyy\"/> \
 \
</osm-script> \
" >/tmp/req.$REF

REQUEST_METHOD=
/home/roland/osm-3s/cgi-bin/interpreter </tmp/req.$REF

rm /tmp/req.$REF
