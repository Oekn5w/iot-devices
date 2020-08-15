#!/bin/bash

SCRIPTPATH=$(dirname "$(realpath -s "$0")")

cd "$SCRIPTPATH"

if [[ ! -d "$SCRIPTPATH/deps" ]]; then
  mkdir deps
  cd deps
  git clone https://github.com/plerup/makeEspArduino.git
  git clone https://github.com/esp8266/Arduino.git esp8266
  git clone https://github.com/espressif/arduino-esp32.git esp32
fi

cd "$SCRIPTPATH/deps/makeEspArduino"
git pull origin --quiet

cd "$SCRIPTPATH/deps/esp8266"
git fetch origin --tags --quiet
GIT_TAG=$(git tag | grep -v 'rc' | tail -1)
if [[ "$(git describe --tags)" != "$GIT_TAG" ]]; then
  git checkout --quiet tags/$GIT_TAG
  git submodule update --init --quiet
  cd tools
  python get.py
fi

cd "$SCRIPTPATH/deps/esp32"
git fetch origin --tags --quiet
GIT_TAG=$(git tag | grep -v 'rc' | tail -1)
if [[ "$(git describe --tags)" != "$GIT_TAG" ]]; then
  git checkout --quiet tags/$GIT_TAG
  git submodule update --init --quiet
  cd tools
  python get.py
fi

cd "$SCRIPTPATH"
