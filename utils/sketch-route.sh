#!/bin/bash

ARG_1=`echo $QUERY_STRING | awk -F [=,\&] '{ print $1; }'`
ARG_2=`echo $QUERY_STRING | awk -F [=,\&] '{ print $2; }'`
ARG_3=`echo $QUERY_STRING | awk -F [=,\&] '{ print $3; }'`
ARG_4=`echo $QUERY_STRING | awk -F [=,\&] '{ print $4; }'`
ARG_5=`echo $QUERY_STRING | awk -F [=,\&] '{ print $5; }'`
ARG_6=`echo $QUERY_STRING | awk -F [=,\&] '{ print $6; }'`
ARG_7=`echo $QUERY_STRING | awk -F [=,\&] '{ print $7; }'`
ARG_8=`echo $QUERY_STRING | awk -F [=,\&] '{ print $8; }'`

BASEDIR=`mktemp -d`

if [[ ( $ARG_2 == "backspace" ) || ($ARG_2 == "backtime") || ($ARG_2 == "debug") ]]; then
{
echo -e "\
<osm-script> \
 \
<union> \
  <id-query type=\"relation\" ref=\"$ARG_1\"/> \
  <recurse type=\"relation-node\"/> \
</union> \
<print mode=\"body\"/> \
 \
</osm-script> \
" >$BASEDIR/req
};
elif [[ -n $ARG_2 ]]; then
{
echo -e "\
<osm-script> \
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
" >$BASEDIR/req
};
else
{
echo -e "\
<osm-script> \
 \
<union> \
  <id-query type=\"relation\" ref=\"$ARG_1\"/> \
  <recurse type=\"relation-node\"/> \
</union> \
<print mode=\"body\"/> \
 \
</osm-script> \
" >$BASEDIR/req
};
fi

# REQUEST_METHOD=
# /home/roland/osm-3s/cgi-bin/interpreter </tmp/nodes_csv_req >/tmp/nodes_csv_result.1
# RESPONSE_TYPE=`head -n 1 </tmp/nodes_csv_result.1`
# if [[ $RESPONSE_TYPE != "Content-type: application/osm3s" ]]; then
# {
#   cat </tmp/nodes_csv_result.1
#   exit 0
# };
# fi
# dd if=/tmp/nodes_csv_result.1 of=/tmp/nodes_csv_result.2 bs=1 skip=56

REQUEST_METHOD=
/home/roland/osm-3s/build/bin/osm3s_query --quiet --no-mime <$BASEDIR/req >$BASEDIR/result

if [[ $ARG_2 == "backspace" ]]; then
{
  echo "Content-Type: image/svg+xml; charset=utf-8"
  echo

  #gunzip </tmp/nodes_csv_result.2 | ../bin/sketch-route-svg --backspace
  ../bin/sketch-route-svg --backspace <$BASEDIR/result
};
elif [[ $ARG_2 == "backtime" ]]; then
{
  echo "Content-Type: image/svg+xml; charset=utf-8"
  echo

  #gunzip </tmp/nodes_csv_result.2 | ../bin/sketch-route-svg --backtime
  ../bin/sketch-route-svg --backtime <$BASEDIR/result
};
elif [[ ($ARG_2 == "debug") || ($ARG_3 == "debug") ]]; then
{
  echo "Content-Type: text/plain"
  echo
  cat <$BASEDIR/result
};
else
{
  echo "Content-Type: image/svg+xml; charset=utf-8"
  echo

  #gunzip </tmp/nodes_csv_result.2 | ../bin/sketch-route-svg
  ../bin/sketch-route-svg <$BASEDIR/result
};
fi
