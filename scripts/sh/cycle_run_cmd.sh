#!/bin/sh

# This function can be added to the .bashrc
# Example:
# run 10 ./myscript.sh
# run 2 echo "test"
run() {
    number=$1
    shift
    for i in `seq $number`; do
      $@
    done
}
