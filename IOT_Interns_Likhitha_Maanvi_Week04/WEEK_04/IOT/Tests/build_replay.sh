#!/usr/bin/env bash
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
FW="$HERE/../Firmware"
OUT="${OUT_DIR:-$HERE/build}"
mkdir -p "$OUT"
g++ -std=c++17 -Wall -Wextra -I"$HERE/arduino_stub" -I"$FW" "$HERE/Replay_Tool/replay.cpp" \
  "$FW/Accident_Detection/detector.cpp" "$FW/IMU/imu.cpp" -o "$OUT/replay" -lm
"$OUT/replay" "$HERE/../Test_Data/scenarios" "$HERE/Scenario_Results.csv"
