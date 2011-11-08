#!/usr/bin/env bash

if [[ -z $1  ]]; then
{
  echo "Usage: $0 test_size"
  echo
  echo "An appropriate value for a fast test is 40, a comprehensive value is 2000."
  exit 0
};
fi

# The size of the test pattern. Asymptotically, the test pattern consists of
# size^2 elements. The size must be divisible by ten. For a full featured test,
# set the value to 2000.
DATA_SIZE="$1"
BASEDIR="$(cd `dirname $0` && pwd)/.."
NOTIMES="$2"

evaluate_test()
{
  DIRNAME="$1"

  FAILED=
  for FILE in `ls ../../expected/$DIRNAME/`; do
  {
    if [[ ! -f $FILE ]]; then
    {
      echo "In Test $EXEC $I: Expected file \"$FILE\" doesn't exist."
      FAILED=YES
    }; fi
  }; done
  for FILE in `ls`; do
  {
    if [[ ! -f "../../expected/$DIRNAME/$FILE" ]]; then
    {
      echo "In Test $EXEC $I: Unexpected file \"$FILE\" exists."
      FAILED=YES
    }; else
    {
      RES=`diff -q "../../expected/$DIRNAME/$FILE" "$FILE"`
      if [[ -n $RES ]]; then
      {
        echo $RES
        FAILED=YES
      }; fi
    }; fi
  }; done
};

perform_serial_test()
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
    "$BASEDIR/test-bin/$1" "$I" $ARGS <"../../input/${EXEC}_$I/stdin.log" >stdout.log 2>stderr.log
  }; else
  {
    "$BASEDIR/test-bin/$1" "$I" $ARGS >stdout.log 2>stderr.log
  }; fi
  evaluate_test "${EXEC}_$I"
  if [[ -n $FAILED ]]; then
  {
    echo `date +%T` "Test $EXEC $I FAILED."
  }; else
  {
    echo `date +%T` "Test $EXEC $I succeeded."
    rm -R *
  }; fi
  popd >/dev/null
};

# Test overpass_api/osm-backend
mkdir -p input/run_and_compare.sh_1/
rm -f input/run_and_compare.sh_1/*
$BASEDIR/test-bin/generate_test_file $DATA_SIZE >input/run_and_compare.sh_1/stdin.log
date +%T
perform_serial_test run_and_compare.sh 1

mkdir -p input/run_and_compare.sh_2/
rm -f input/run_and_compare.sh_2/*
mv input/run_and_compare.sh_1/stdin.log input/run_and_compare.sh_2/stdin.log
date +%T
perform_serial_test run_and_compare.sh 2

mkdir -p input/run_and_compare.sh_3/
rm -f input/run_and_compare.sh_3/*
mv input/run_and_compare.sh_2/stdin.log input/run_and_compare.sh_3/stdin.log
date +%T
perform_serial_test run_and_compare.sh 3

rm -R input/run_and_compare.sh_3
