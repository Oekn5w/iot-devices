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

echo
echo "Processing shutter-1"
$SOURCEDIR/src/shutter-1/make-all.sh $@
RES=$?
if [[ "$RES" != "0" ]]; then
  exit $RES
fi

echo
echo "Processing shutter-3"
cd $SOURCEDIR/src/shutter-3
make $@
RES=$?
if [[ "$RES" != "0" ]]; then
  echo "Shutter-3 make failed, aborting!"
  exit $RES
fi

echo
echo "Processing boiler"
cd $SOURCEDIR/src/boiler
make $@
RES=$?
if [[ "$RES" != "0" ]]; then
  echo "Boiler make failed, aborting!"
  exit $RES
fi

exit 0
