#!/bin/sh

RUBY=ruby
VALIDATE_RB=../validate.schema.json.rb

for i in *.table.json
do
  echo "validating ... $i"
  $RUBY $VALIDATE_RB table.schema.json $i
  if [ $? != 0 ]; then
    echo "error, abort"
    exit 1
  fi
done