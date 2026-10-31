#!/usr/bin/env bash
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$HERE/.."
TINYGPS_SRC="${TINYGPS_SRC:?set TINYGPS_SRC}"
OUT="${OUT_DIR:-$HERE/build}"
mkdir -p "$OUT"
python3 -I "$ROOT/tools/make_wokwi_project.py" "$OUT/wokwi_flat" >/dev/null
g++ -std=c++17 -Wall -Wextra -I"$HERE/arduino_stub" -I"$TINYGPS_SRC" -I"$OUT/wokwi_flat" -x c++ \
  "$OUT/wokwi_flat/sketch.ino" "$HERE/Wokwi_Host_Check/sketch_host_main.cpp" "$OUT"/wokwi_flat/*.cpp "$TINYGPS_SRC/TinyGPS++.cpp" -o "$OUT/wokwi_sketch_host" -lm
"$OUT/wokwi_sketch_host" > "$OUT/wokwi_sketch_host.txt"
python3 -I "$HERE/validate_contract.py" "$OUT/wokwi_sketch_host.txt"
grep -c "" "$OUT/wokwi_sketch_host.txt"
