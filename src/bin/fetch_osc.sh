#!/bin/bash

if [[ -z $1  ]]; then
{
  echo Usage: $0 Hourly_Timestamp
  exit 0
};
fi

REPLICATE_ID=$1
SOURCE_DIR=$2
LOCAL_DIR=$3

if [[ ! -d $LOCAL_DIR ]];
  then {
    mkdir $LOCAL_DIR
};
fi

fetch_minute_diff()
{
  printf -v TDIGIT3 %03u $(($1 % 1000))
  ARG=$(($1 / 1000))
  printf -v TDIGIT2 %03u $(($ARG % 1000))
  ARG=$(($ARG / 1000))
  printf -v TDIGIT1 %03u $ARG
  
  if [[ ! -d $LOCAL_DIR/$TDIGIT1 ]];
  then {
    mkdir $LOCAL_DIR/$TDIGIT1
  };
  fi
  if [[ ! -d $LOCAL_DIR/$TDIGIT1/$TDIGIT2 ]];
  then {
    mkdir $LOCAL_DIR/$TDIGIT1/$TDIGIT2
  };
  fi
  if [[ ! -s $LOCAL_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.osc.gz ]];
  then {
    wget -nv -O $LOCAL_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.osc.gz $SOURCE_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.osc.gz
  };
  fi
  until [[ -s $LOCAL_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.osc.gz ]];
  do {
    sleep 60
    wget -nv -O $LOCAL_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.osc.gz $SOURCE_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.osc.gz
  };
  done
  if [[ ! -s $LOCAL_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.state.txt ]];
  then {
    wget -nv -O $LOCAL_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.state.txt $SOURCE_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.state.txt
  };
  fi
  until [[ -s $LOCAL_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.state.txt ]];
  do {
    sleep 60
    wget -nv -O $LOCAL_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.state.txt $SOURCE_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.state.txt
  };
  done
  TIMESTAMP_LINE=`grep timestamp $LOCAL_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3.state.txt`
  TIMESTAMP=${TIMESTAMP_LINE:10}
};

while [[ 0 -eq 0 ]];
do
{
  REPLICATE_ID=$(($REPLICATE_ID + 1))
  fetch_minute_diff $REPLICATE_ID
  sleep 1
  echo "fetch_osc()@"`date "+%s"`": new_replicate_diff $REPLICATE_ID $TIMESTAMP" >>$LOCAL_DIR/fetch_osc.log
  echo " new_changefile $REPLICATE_ID $TIMESTAMP " >>/tmp/dispatcher.pipe
};
done
