#!/bin/bash

SOURCEDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

cd $SOURCEDIR

if [[ $# -ne 0 ]]; then

  EXIT=1

  for i in "$@"
  do

  case $i in
    ota)
    EXIT=0
    ;;
    clean)
    EXIT=0
    ;;
    *)
    ;;
  esac
  done

  if (( $EXIT )); then
    echo "Only ota or build actions allowed for all."
    exit 1
  fi

fi

make SHUTTER_ID=1 $@

make SHUTTER_ID=2 $@

make SHUTTER_ID=3 $@
