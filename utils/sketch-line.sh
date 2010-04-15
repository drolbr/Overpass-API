#!/bin/bash

BUF=$QUERY_STRING\&

SKETCH_PARAMS=
BRIM_PARAMS=

while [[ -n $BUF ]]; do
{
  KEY=`echo $BUF | awk '{ print substr($0,0,match($0,"=")); }'`
  BUF=`echo $BUF | awk '{ print substr($0,match($0,"=")+1); }'`
  VALUE=`echo $BUF | awk '{ print substr($0,0,match($0,"\&")); }'`
  BUF=`echo $BUF | awk '{ print substr($0,match($0,"\&")+1); }'`
  if [[ $KEY == "network" && -n $VALUE ]]; then
  {
    NETWORK=$VALUE
    NETWORK_=`echo $VALUE | ../bin/uncgi`
  };
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
  {
    SKETCH_PARAMS="$SKETCH_PARAMS --options=/opt/osm_why_api/options/sketch-line.$VALUE"
    BRIM_PARAMS="$BRIM_PARAMS --options=/opt/osm_why_api/options/sketch-line.$VALUE"
  };
  elif [[ $KEY == "correspondences" && -n $VALUE ]]; then
  {
    SKETCH_PARAMS="$SKETCH_PARAMS --walk-limit=$VALUE"
    BRIM_PARAMS="$BRIM_PARAMS --size=$VALUE"
  };
  elif [[ $KEY == "max-cors-per-line" && -n $VALUE ]]; then
    SKETCH_PARAMS="$SKETCH_PARAMS --max-correspondences-per-line=$VALUE"
  elif [[ $KEY == "max-cors-below" && -n $VALUE ]]; then
    SKETCH_PARAMS="$SKETCH_PARAMS --max-correspondences-below=$VALUE"
  elif [[ $KEY == "debug" && -n $VALUE ]]; then
    DEBUG=$VALUE
  fi
};
done

BASEDIR=`mktemp -d`

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
" >$BASEDIR/request.1

if [[ -z $REF ]]; then
{                     
  echo "Content-Type: text/plain; charset=utf-8"
  echo                                          
  echo "An empty value for ref is not allowed"  

  exit 0
};    
fi      

CORRESPONDENCES=`../bin/bbox-brim-query --only-corrs $BRIM_PARAMS`

if [[ $CORRESPONDENCES -gt 0 ]]; then
{
  REQUEST_METHOD=
  /home/roland/osm-3s/cgi-bin/interpreter <$BASEDIR/request.1 >$BASEDIR/answer.1
  RESPONSE_TYPE=`head -n 1 <$BASEDIR/answer.1`
  if [[ $RESPONSE_TYPE != "Content-type: application/osm3s" ]]; then
  {
    cat <$BASEDIR/answer.1
    exit 0
  };
  fi
  dd if=$BASEDIR/answer.1 of=$BASEDIR/answer.2 bs=1 skip=56
  gunzip <$BASEDIR/answer.2 | ../bin/bbox-brim-query $BRIM_PARAMS >$BASEDIR/request.2

  if [[ $DEBUG == "full-query" ]]; then
  {
    echo "Content-Type: text/plain; charset=utf-8"
    echo

    cat <$BASEDIR/request.2

    echo
  };
  fi

  REQUEST_METHOD=
  /home/roland/osm-3s/cgi-bin/interpreter <$BASEDIR/request.2 >$BASEDIR/answer.3
  RESPONSE_TYPE=`head -n 1 <$BASEDIR/answer.3`
  if [[ $RESPONSE_TYPE != "Content-type: application/osm3s" ]]; then
  {
    cat <$BASEDIR/answer.3
    exit 0
  };
  fi
  dd if=$BASEDIR/answer.3 of=$BASEDIR/answer.4 bs=1 skip=56

  echo "Content-Type: image/svg+xml; charset=utf-8"
  echo

  if [[ $DEBUG == "full-query" ]]; then
  {
    gunzip <$BASEDIR/answer.4
    echo
    echo "../bin/sketch-route-svg --ref=\"$REF_\" --network=\"$NETWORK_\" $SKETCH_PARAMS"
    exit 0;
  };
  fi;

  gunzip <$BASEDIR/answer.4 | ../bin/sketch-route-svg --ref="$REF_" --network="$NETWORK_" $SKETCH_PARAMS
};
else
{
  REQUEST_METHOD=
  /home/roland/osm-3s/cgi-bin/interpreter <$BASEDIR/request.1 >$BASEDIR/answer.1
  RESPONSE_TYPE=`head -n 1 <$BASEDIR/answer.1`
  if [[ $RESPONSE_TYPE != "Content-type: application/osm3s" ]]; then
  {
    cat <$BASEDIR/answer.1
    exit 0
  };
  fi
  dd if=$BASEDIR/answer.1 of=$BASEDIR/answer.2 bs=1 skip=56

  if [[ $DEBUG == "full-query" ]]; then
  {
    echo "Content-Type: text/plain; charset=utf-8"
    echo

    gunzip <$BASEDIR/answer.2

    echo
  };
  fi;

  echo "Content-Type: image/svg+xml; charset=utf-8"
  echo

  gunzip <$BASEDIR/answer.2 | ../bin/sketch-route-svg $SKETCH_PARAMS
};
fi
