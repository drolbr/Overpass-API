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
  elif [[ $KEY == "pivot" && -n $VALUE ]]; then
    SKETCH_PARAMS="$SKETCH_PARAMS --pivot=$VALUE"
  elif [[ $KEY == "debug" && -n $VALUE ]]; then
    DEBUG=$VALUE
  fi
};
done

BASEDIR=`mktemp -d`

echo -e "\
data=<osm-script> \
 \
<union> \
  <query type=\"relation\"> \
    <has-kv k=\"network\" v=\"$NETWORK_\"/> \
    <has-kv k=\"ref\" v=\"$REF_\"/> \
  </query> \
  <recurse type=\"relation-node\" into=\"__\"/> \
  <recurse type=\"relation-way\"/> \
  <recurse type=\"way-node\"/> \
</union> \
<print mode=\"body\"/> \
 \
</osm-script>
" >$BASEDIR/request.1

if [[ -z $REF ]]; then
{                     
  echo "Content-Type: text/plain; charset=utf-8"
  echo                                          
  echo "An empty value for ref is not allowed."  

  exit 0
};    
fi      

CORRESPONDENCES=`../bin/bbox-brim-query --only-corrs $BRIM_PARAMS`

if [[ $CORRESPONDENCES -gt 0 ]]; then
{
  echo "Content-Type: text/plain; charset=utf-8"
  echo                                          
  echo "Correspondences are for the geographical map currently not supported."  

  exit 0
};
else
{
  REQUEST_METHOD=
  /home/roland/osm-3s/build/bin/osm3s_query --quiet --no-mime <$BASEDIR/request.1 >$BASEDIR/answer.1

  if [[ $DEBUG == "full-query" ]]; then
  {
    echo "Content-Type: text/plain; charset=utf-8"
    echo
    cat <$BASEDIR/answer.1
    echo
  };
  fi;

  echo "Content-Type: image/svg+xml; charset=utf-8"
  echo

  ../bin/draw-route-svg $SKETCH_PARAMS <$BASEDIR/answer.1
};
fi
