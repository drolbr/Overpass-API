#!/bin/bash

BUF=$QUERY_STRING\&

# echo "Content-Type: text/plain; charset=utf-8"
# echo

SKETCH_PARAMS=

while [[ -n $BUF ]]; do
{
  KEY=`echo $BUF | awk '{ print substr($0,0,match($0,"=")); }'`
  BUF=`echo $BUF | awk '{ print substr($0,match($0,"=")+1); }'`
  VALUE=`echo $BUF | awk '{ print substr($0,0,match($0,"\&")); }'`
  BUF=`echo $BUF | awk '{ print substr($0,match($0,"\&")+1); }'`
  if [[ $KEY == "network" && -n $VALUE ]]; then
    NETWORK=$VALUE
  elif [[ $KEY == "ref" && -n $VALUE ]]; then
    REF=$VALUE
  elif [[ $KEY == "width" && -n $VALUE ]]; then
    SKETCH_PARAMS="$SKETCH_PARAMS --width=$VALUE"
  elif [[ $KEY == "height" && -n $VALUE ]]; then
    SKETCH_PARAMS="$SKETCH_PARAMS --height=$VALUE"
  elif [[ $KEY == "font-size" && -n $VALUE ]]; then
    SKETCH_PARAMS="$SKETCH_PARAMS --stop-font-size=$VALUE"
  elif [[ $KEY == "force-rows" && -n $VALUE ]]; then
    SKETCH_PARAMS="$SKETCH_PARAMS --rows=$VALUE"
  elif [[ $KEY == "style" && -n $VALUE ]]; then
    SKETCH_PARAMS="$SKETCH_PARAMS --options=/opt/osm_why_api/options/sketch-line.$VALUE"
  fi
};
done

echo -e "
data=<osm-script timeout=\"180\" element-limit=\"10000000\"> \
 \
<union> \
  <query type=\"relation\"> \
    <has-kv k=\"network\" v=\"$NETWORK\"/> \
    <has-kv k=\"ref\" v=\"$REF\"/> \
  </query> \
  <recurse type=\"relation-node\"/> \
</union> \
<print mode=\"body\"/> \
 \
</osm-script> \
" >/tmp/sketch_line_req

echo "Content-Type: image/svg+xml; charset=utf-8"
echo

REQUEST_METHOD=
/home/roland/osm-3s/cgi-bin/interpreter </tmp/sketch_line_req >/tmp/sketch_line_req.1
dd if=/tmp/sketch_line_req.1 of=/tmp/sketch_line_req.2 bs=1 skip=56
gunzip </tmp/sketch_line_req.2 | ../bin/sketch-route-svg $SKETCH_PARAMS
