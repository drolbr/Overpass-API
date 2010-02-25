#!/bin/bash

BUF=$QUERY_STRING\&

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
  {
    REF=$VALUE
    REF_=`echo $VALUE | ../bin/uncgi`
  };
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
  elif [[ $KEY == "correspondences" && -n $VALUE ]]; then
    CORRESPONDENCES=$VALUE
  elif [[ $KEY == "max-cors-per-line" && -n $VALUE ]]; then
    SKETCH_PARAMS="$SKETCH_PARAMS --max-correspondences-per-line=$VALUE"
  elif [[ $KEY == "max-cors-below" && -n $VALUE ]]; then
    SKETCH_PARAMS="$SKETCH_PARAMS --max-correspondences-below=$VALUE"
  elif [[ $KEY == "debug" && -n $VALUE ]]; then
    DEBUG=$VALUE
  fi
};
done

echo -e "\
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
</osm-script>
" >/tmp/sketch_line_req

if [[ -n $CORRESPONDENCES ]]; then
{
  REQUEST_METHOD=
  /home/roland/osm-3s/cgi-bin/interpreter </tmp/sketch_line_req >/tmp/sketch_line_req.1
  RESPONSE_TYPE=`head -n 1 </tmp/sketch_line_req.1`
  if [[ $RESPONSE_TYPE != "Content-type: application/osm3s" ]]; then
  {
    cat </tmp/sketch_line_req.1
    exit 0
  };
  fi
  dd if=/tmp/sketch_line_req.1 of=/tmp/sketch_line_req.2 bs=1 skip=56
  gunzip </tmp/sketch_line_req.2 | ../bin/bbox-brim-query --size=$CORRESPONDENCES >/tmp/sketch_line_req.3

  if [[ $DEBUG == "full-query" ]]; then
  {
    echo "Content-Type: text/plain; charset=utf-8"
    echo

    cat </tmp/sketch_line_req.3

    echo
  };
  fi;

  REQUEST_METHOD=
  /home/roland/osm-3s/cgi-bin/interpreter </tmp/sketch_line_req.3 >/tmp/sketch_line_req.4
  RESPONSE_TYPE=`head -n 1 </tmp/sketch_line_req.4`
  if [[ $RESPONSE_TYPE != "Content-type: application/osm3s" ]]; then
  {
    cat </tmp/sketch_line_req.4
    exit 0
  };
  fi
  dd if=/tmp/sketch_line_req.4 of=/tmp/sketch_line_req.5 bs=1 skip=56

  echo "Content-Type: image/svg+xml; charset=utf-8"
  echo

  if [[ $DEBUG == "full-query" ]]; then
  {
    gunzip </tmp/sketch_line_req.5
    exit 0;
  };
  fi;

  gunzip </tmp/sketch_line_req.5 | ../bin/sketch-route-svg --walk-limit=$CORRESPONDENCES --ref="$REF_" $SKETCH_PARAMS
};
else
{
  REQUEST_METHOD=
  /home/roland/osm-3s/cgi-bin/interpreter </tmp/sketch_line_req >/tmp/sketch_line_req.1
  RESPONSE_TYPE=`head -n 1 </tmp/sketch_line_req.1`
  if [[ $RESPONSE_TYPE != "Content-type: application/osm3s" ]]; then
  {
    cat </tmp/sketch_line_req.1
    exit 0
  };
  fi
  dd if=/tmp/sketch_line_req.1 of=/tmp/sketch_line_req.2 bs=1 skip=56

  echo "Content-Type: image/svg+xml; charset=utf-8"
  echo

  gunzip </tmp/sketch_line_req.2 | ../bin/sketch-route-svg $SKETCH_PARAMS
};
fi
