#!/bin/bash

SCRIPTPATH=$(dirname "$(realpath -s "$0")")

cd "$SCRIPTPATH"

if [[ ! -d "$SCRIPTPATH/deps" ]]; then
  mkdir deps
fi

if [[ ! -d "$SCRIPTPATH/deps/makeEspArduino" ]]; then
  git clone https://github.com/plerup/makeEspArduino.git "$SCRIPTPATH/deps/makeEspArduino"
fi
if [[ ! -d "$SCRIPTPATH/deps/esp8266" ]]; then
  git clone https://github.com/esp8266/Arduino.git "$SCRIPTPATH/deps/esp8266"
fi
if [[ ! -d "$SCRIPTPATH/deps/esp32" ]]; then
  git clone https://github.com/espressif/arduino-esp32.git "$SCRIPTPATH/deps/esp32"
fi

cd "$SCRIPTPATH/deps/makeEspArduino"
git pull origin --quiet

cd "$SCRIPTPATH/deps/esp8266"
git fetch origin --tags --quiet
#GIT_TAG=$(git tag | grep -e '^[0-9\.]*$' | tail -1)
GIT_TAG=3.0.2
if [[ "$(git describe --tags)" != "$GIT_TAG" ]]; then
  git checkout --quiet tags/$GIT_TAG
  git submodule update --init --quiet
  cd tools
  python get.py
fi

cd "$SCRIPTPATH/deps/esp32"
git fetch origin --tags --quiet
#GIT_TAG=$(git tag | grep -e '^[0-9\.]*$' | tail -1)
GIT_TAG=2.0.2
if [[ "$(git describe --tags)" != "$GIT_TAG" ]]; then
  git checkout --quiet tags/$GIT_TAG
  git submodule update --init --quiet
  cd tools
  python get.py
fi

cd "$SCRIPTPATH"
