#!/usr/bin/env bash

mkdir -p "/tmp/translate_xapi/"

while [[ true ]]; do
{
  find /tmp/translate_xapi/ -mmin +240 -exec rm {} \;
  sleep 7200
};
done
