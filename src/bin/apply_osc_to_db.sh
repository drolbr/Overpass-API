#!/usr/bin/env bash

if [[ -z $3  ]]; then
{
  echo Usage: $0 database_dir replicate_dir start_id
  exit 0
};
fi

DB_DIR="$1"
REPLICATE_DIR="$2"
START=$3
TARGET=$(($START + 1))
META=$4

EXEC_DIR="`dirname $0`/"
if [[ ! ${EXEC_DIR:0:1} == "/" ]]; then
{
  EXEC_DIR="`pwd`/$EXEC_DIR"
};
fi

get_replicate_filename()
{
  printf -v TDIGIT3 %03u $(($TARGET % 1000))
  ARG=$(($TARGET / 1000))
  printf -v TDIGIT2 %03u $(($ARG % 1000))
  ARG=$(($ARG / 1000))
  printf -v TDIGIT1 %03u $ARG
  REPLICATE_FILENAME=$REPLICATE_DIR/$TDIGIT1/$TDIGIT2/$TDIGIT3
};

collect_minute_diffs()
{
  TEMP_DIR=$1

  get_replicate_filename

  while [[ ( -s $REPLICATE_FILENAME.state.txt ) && ( $(($START + 720)) -ge $(($TARGET)) ) ]];
  do
  {
    printf -v TARGET_FILE %09u $TARGET
    gunzip <$REPLICATE_FILENAME.osc.gz >$TEMP_DIR/$TARGET_FILE.osc
    TARGET=$(($TARGET + 1))
    get_replicate_filename
  };
  done
  TARGET=$(($TARGET - 1))
};

apply_minute_diffs()
{
  ./update_from_dir --osc-dir=$1 --version=$DATA_VERSION $META
  EXITCODE=$?
  while [[ $EXITCODE -ne 0 ]];
  do
  {
    sleep 60
    ./update_from_dir --osc-dir=$1 --version=$DATA_VERSION $META
    EXITCODE=$?
  };
  done
};

update_state()
{
  get_replicate_filename
  TIMESTAMP_LINE=`grep "^timestamp" <$REPLICATE_FILENAME.state.txt`
  while [[ -z $TIMESTAMP_LINE ]]; do
  {
    sleep 5
    TIMESTAMP_LINE=`grep "^timestamp" <$REPLICATE_FILENAME.state.txt`
  }; done
  DATA_VERSION=${TIMESTAMP_LINE:10}
};

echo >>$DB_DIR/apply_osc_to_db.log

# update_state

pushd "$EXEC_DIR"

while [[ true ]]; do
{
  echo "`date '+%F %T'`: updating from $START" >>$DB_DIR/apply_osc_to_db.log

  TEMP_DIR=`mktemp -d`
  collect_minute_diffs $TEMP_DIR

  echo "`date '+%F %T'`: updating to $TARGET" >>$DB_DIR/apply_osc_to_db.log

  update_state

  if [[ $TARGET -gt $START ]]; then
  {
    apply_minute_diffs $TEMP_DIR
    echo "$TARGET" >$DB_DIR/replicate_id
  };
  else
  {
    sleep 30
  }; fi

  echo "`date '+%F %T'`: update complete" $TARGET >>$DB_DIR/apply_osc_to_db.log

  rm -f $TEMP_DIR/*
  rmdir $TEMP_DIR

  START=$TARGET
  TARGET=$(($START + 1))
}; done
