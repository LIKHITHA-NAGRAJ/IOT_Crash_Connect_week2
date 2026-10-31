#!/usr/bin/env bash
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
FW="$HERE/../Firmware"
TINYGPS_SRC="${TINYGPS_SRC:?set TINYGPS_SRC to the folder containing TinyGPS++.cpp}"
OUT="${OUT_DIR:-$HERE/build}"
mkdir -p "$OUT"
CXX="g++ -std=c++17 -Wall -Wextra -I$HERE/arduino_stub -I$TINYGPS_SRC -I$FW"
SRCS=$(find "$FW" -name '*.cpp')
for prog in "$@"; do
  $CXX "$prog" $SRCS "$TINYGPS_SRC/TinyGPS++.cpp" -o "$OUT/$(basename "$prog" .cpp)" -lm
done
