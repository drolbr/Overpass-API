#!/bin/bash

print_test_5()
{
  EXEC="$1"
  I="$2"
  ARGS="$3"

  mkdir -p "run/${EXEC}_$I"
  pushd "run/${EXEC}_$I/" >/dev/null
  rm -f *
  "../../../test-bin/$1" "$I" $ARGS 2>stderr.log | grep "^  <" | sort >stdout.log
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
    echo `date +%X` "Test $EXEC $I FAILED."
  }; else
  {
    echo `date +%X` "Test $EXEC $I succeeded."
    rm -R *
  }; fi
  popd >/dev/null
};

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
    "../../../test-bin/$1" "$I" $ARGS <"../../input/${EXEC}_$I/stdin.log" >stdout.log 2>stderr.log
  }; else
  {
    "../../../test-bin/$1" "$I" $ARGS >stdout.log 2>stderr.log
  }; fi
  FAILED=
  for FILE in `ls ../../expected/${EXEC}_$I/`; do
  {
    if [[ ! -f $FILE ]]; then
    {
      echo "In test $EXEC $I: Expected file \"$FILE\" doesn't exist."
      FAILED=YES
    }; fi
  }; done
  for FILE in `ls`; do
  {
    if [[ ! -f "../../expected/${EXEC}_$I/$FILE" ]]; then
    {
      echo "In test $EXEC $I: Unexpected file \"$FILE\" exists."
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
    echo `date +%X` "Test $EXEC $I FAILED."
  }; else
  {
    echo `date +%X` "Test $EXEC $I succeeded."
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

prepare_test_loop()
{
  I=1
  DATA_SIZE=$3
  while [[ $I -le $2 ]]; do
  {
    mkdir -p expected/$1_$I/
    rm -f expected/$1_$I/*
    ../test-bin/generate_test_file $DATA_SIZE $1_$I >expected/$1_$I/stdout.log
    touch expected/$1_$I/stderr.log
    I=$(($I + 1))
  }; done
};

# The size of the test pattern. Asymptotically, the test pattern consists of
# size^2 elements. The size must be divisible by ten. For a full featured test,
# set the value to 2000.
DATA_SIZE=40

# Test template_db
date +%X
perform_test_loop file_blocks 12
perform_test_loop block_backend 13
perform_test_loop random_file 4

# Test overpass_api/osm-backend
mkdir -p input/run_and_compare.sh_1/
../test-bin/generate_test_file $DATA_SIZE >input/run_and_compare.sh_1/stdin.log
date +%X
perform_test run_and_compare.sh 1

mkdir -p input/run_and_compare.sh_2/
mv input/run_and_compare.sh_1/stdin.log input/run_and_compare.sh_2/stdin.log
date +%X
perform_test run_and_compare.sh 2

mkdir -p input/run_and_compare.sh_3/
mv input/run_and_compare.sh_2/stdin.log input/run_and_compare.sh_3/stdin.log
date +%X
perform_test run_and_compare.sh 3

# Prepare testing the statements
mkdir -p input/update_database/
rm -f input/update_database/*
mv input/run_and_compare.sh_3/stdin.log input/update_database/stdin.log
../bin/update_database --db-dir=input/update_database/ <input/update_database/stdin.log

# Test the print and id_query statements
prepare_test_loop print 4 $DATA_SIZE
mkdir -p expected/print_5/
../test-bin/generate_test_file $DATA_SIZE print_4 | grep "^  <" | sort >expected/print_5/stdout.log
touch expected/print_5/stderr.log

date +%X
perform_test_loop print 4 "$DATA_SIZE ../../input/update_database/"
print_test_5 print 5 "$DATA_SIZE ../../input/update_database/"

# Test the recurse statement
prepare_test_loop recurse 11 $DATA_SIZE
date +%X
perform_test_loop recurse 11 "$DATA_SIZE ../../input/update_database/"

# Test the bbox_query statement
prepare_test_loop bbox_query 8 $DATA_SIZE
date +%X
perform_test_loop bbox_query 8 "$DATA_SIZE ../../input/update_database/"

# Test the query statement
prepare_test_loop query 25 $DATA_SIZE
date +%X
perform_test_loop query 25 "$DATA_SIZE ../../input/update_database/"

# Test the foreach statement
date +%X
perform_test_loop foreach 2 ../../input/update_database/

# Test the union statement
prepare_test_loop union 6 $DATA_SIZE
date +%X
perform_test_loop union 6 ../../input/update_database/

#rm input/update_database/*
