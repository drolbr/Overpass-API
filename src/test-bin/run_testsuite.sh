#!/bin/bash

perform_test()
{
  EXEC="$1"
  I="$2"
  ARGS="$3"

  mkdir -p "run/${EXEC}_$I"
  pushd "run/${EXEC}_$I/" >/dev/null
  if [[ -s "../../input/${EXEC}_$I/stdin.log" ]]; then
  {
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

perform_test_loop file_blocks 12
perform_test_loop block_backend 13
perform_test_loop random_file 4

mkdir -p input/node_updater_1/
../test-bin/generate_test_file >input/node_updater_1/stdin.log
perform_test node_updater 1

mkdir -p input/way_updater_1/
mv input/node_updater_1/stdin.log input/way_updater_1/stdin.log
perform_test way_updater 1

mkdir -p input/relation_updater_1/
mv input/way_updater_1/stdin.log input/relation_updater_1/stdin.log
perform_test relation_updater 1

rm input/relation_updater_1/stdin.log
