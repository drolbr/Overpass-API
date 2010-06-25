#!/bin/bash

if [[ -z $3  ]]; then
{
  echo Usage: $0 database_dir replicate_dir start_id
  exit 0
};
fi

DB_DIR=$1
REPLICATE_DIR=$2
START=$3
TARGET=$(($START + 1))

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

  while [[ ( -s $REPLICATE_FILENAME.osc.gz ) && ( $(($START + 720)) -ge $(($TARGET)) ) ]];
  do
  {
    printf -v TARGET_FILE %09u $TARGET
    gunzip <$REPLICATE_FILENAME.osc.gz >$TEMP_DIR/$TARGET_FILE.osc
    TARGET=$(($TARGET + 1))
    get_replicate_filename
  };
  done
  TARGET=$(($TARGET - 1))
  [[ ! -s $REPLICATE_FILENAME.osc.gz ]]
  COMPLETE=$?
};

apply_minute_diffs()
{
  ./apply_osc --db-dir=$DB_DIR --osc-dir=$1
  EXITCODE=$?
  while [[ $EXITCODE -ne 0 ]];
  do
  {
    sleep 60
    ./apply_osc --db-dir=$DB_DIR --osc-dir=$1
    EXITCODE=$?
  };
  done
};

update_state()
{
  rm $DB_DIR/state
  echo "$TARGET" >$DB_DIR/replicate_id
  get_replicate_filename
  grep "^timestamp" <$REPLICATE_FILENAME.state.txt >$DB_DIR/state
  while [[ ! -f $DB_DIR/state ]]; do
  {
    sleep 5
    grep "^timestamp" <$REPLICATE_FILENAME.state.txt >$DB_DIR/state
  }; done
};

echo >>$DB_DIR/apply_osc_to_db.log
while [[ true ]]; do
{
  while [[ ! -f $DB_DIR/dirty ]]; do
  {
    sleep 5
  }; done

  echo "`date '+%F %T'`: database is dirty" >>$DB_DIR/apply_osc_to_db.log
  echo "`date '+%F %T'`: updating from $START" >>$DB_DIR/apply_osc_to_db.log

  TEMP_DIR=`mktemp -d`
  collect_minute_diffs $TEMP_DIR

  echo "`date '+%F %T'`: updating to $TARGET" >>$DB_DIR/apply_osc_to_db.log

  apply_minute_diffs $TEMP_DIR

  echo "`date '+%F %T'`: update complete" $TARGET >>$DB_DIR/apply_osc_to_db.log

  update_state
  if [[ $COMPLETE -eq 0 ]]; then
  {
    rm $DB_DIR/dirty
  }; fi

  echo "`date '+%F %T'`: database is cleared" $TARGET >>$DB_DIR/apply_osc_to_db.log

  rm -f $TEMP_DIR/*
  rmdir $TEMP_DIR

  START=$TARGET
  TARGET=$(($START + 1))
}; done
