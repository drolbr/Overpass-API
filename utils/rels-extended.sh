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
  if [[ $KEY == "id" && -n $VALUE ]]; then
  {
    REF=$VALUE
  };
  fi
};
done

BASEDIR=`mktemp -d`

echo -e "\
data=<osm-script timeout=\"180\" element-limit=\"10000000\"> \
 \
<union into=\"full\"> \
  <id-query type=\"relation\" ref=\"$REF\" into=\"rels\"/> \
  <recurse type=\"relation-way\" from=\"rels\"/> \
  <recurse type=\"way-node\"/> \
  <recurse type=\"relation-node\" from=\"rels\"/> \
</union> \
<union> \
  <recurse type=\"node-relation\" from=\"full\"/> \
  <recurse type=\"way-relation\" from=\"full\"/> \
  <recurse type=\"relation-backwards\" from=\"full\"/> \
</union> \
<print mode=\"ids_only\"/> \
 \
</osm-script>
" >$BASEDIR/request.1

if [[ -z $REF ]]; then
{                     
  echo "Content-Type: text/plain; charset=utf-8"
  echo                                          
  echo "Please use an URL like ..?id=[id_of_relation]"  

  exit 0
};    
fi

REQUEST_METHOD=
/home/roland/osm-3s/cgi-bin/interpreter <$BASEDIR/request.1
