#!/bin/bash

perform_test()
{
  EXEC="$1"
  I="$2"
  ARGS="$3"

  mkdir -p "run/${EXEC}_$I"
  pushd "run/${EXEC}_$I/" >/dev/null
  rm -f *
  if [[ -s "../../input/${EXEC}_$I/stdin.log" ]]; then
  {
    #echo "stdin.log found"
    "../../../test-bin/$1" "$I" "$ARGS" <"../../input/${EXEC}_$I/stdin.log" >stdout.log 2>stderr.log
  }; else
  {
    "../../../test-bin/$1" "$I" "$ARGS" >stdout.log 2>stderr.log
  }; fi
  FAILED=
  for FILE in `ls ../../expected/${EXEC}_$I/`; do
  {
    if [[ ! -f $FILE ]]; then
    {
      echo "In Test $EXEC $I: Expected file \"$FILE\" doesn't exist."
      FAILED=YES
    }; fi
  }; done
  for FILE in `ls`; do
  {
    if [[ ! -f "../../expected/${EXEC}_$I/$FILE" ]]; then
    {
      echo "In Test $EXEC $I: Unexpected file \"$FILE\" exists."
      FAILED=YES
    }; else
    {
      RES=`diff -q "../../expected/${EXEC}_$I/$FILE" "$FILE"`
      if [[ -n $RES ]]; then
      {
        echo $RES
        FAILED=YES
      }; fi
    }; fi
  }; done
  if [[ -n $FAILED ]]; then
  {
    echo "Test $EXEC $I FAILED."
  }; else
  {
    echo "Test $EXEC $I succeeded."
    rm -R *
  }; fi
  popd >/dev/null
};

perform_test_loop()
{
  I=1
  while [[ $I -le $2 ]]; do
  {
    perform_test "$1" $I "$3"
    I=$(($I + 1))
  }; done
};

# Test template_db
perform_test_loop file_blocks 12
perform_test_loop block_backend 13
perform_test_loop random_file 4

# Test overpass_api/osm-backend
mkdir -p input/run_and_compare.sh_1/
../test-bin/generate_test_file 200 >input/run_and_compare.sh_1/stdin.log
perform_test run_and_compare.sh 1

mkdir -p input/run_and_compare.sh_2/
mv input/run_and_compare.sh_1/stdin.log input/run_and_compare.sh_2/stdin.log
perform_test run_and_compare.sh 2

mkdir -p input/run_and_compare.sh_3/
mv input/run_and_compare.sh_2/stdin.log input/run_and_compare.sh_3/stdin.log
perform_test run_and_compare.sh 3

# Prepare testing the statements
mkdir -p input/update_database/
mv input/run_and_compare.sh_3/stdin.log input/update_database/stdin.log
../bin/update_database --db-dir=input/update_database/ <input/update_database/stdin.log

perform_test_loop print 5 ../../input/update_database/

rm input/update_database/stdin.log
