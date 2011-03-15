#!/bin/bash

perform_test()
{
  EXEC="$1"
  I="$2"
  ARGS="$3"

  mkdir -p "run/${EXEC}_$I"
  pushd "run/${EXEC}_$I/" >/dev/null
  "../../../test-bin/$1" "$I" "$ARGS" >stdout.log 2>stderr.log
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

I=1
while [[ $I -le 12 ]]; do
{
  perform_test file_blocks $I
  I=$(($I + 1))
}; done

I=1
while [[ $I -le 13 ]]; do
{
  perform_test block_backend $I
  I=$(($I + 1))
}; done
