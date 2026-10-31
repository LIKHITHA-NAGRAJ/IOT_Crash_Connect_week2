#!/usr/bin/env bash
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
FW="$HERE/../crashconnect_fw"
TINYGPS_SRC="${TINYGPS_SRC:?set TINYGPS_SRC to the folder that contains TinyGPS++.cpp and TinyGPS++.h}"
OUT="${OUT_DIR:-$HERE/build}"
mkdir -p "$OUT"

SRCS=$(find "$FW/src" -name '*.cpp' ! -path '*/sim/*')
SIM="$FW/src/sim/sim_scenario.cpp"

g++ -std=c++17 -Wall -Wextra -I"$HERE/arduino_stub" -I"$TINYGPS_SRC" -x c++ \
  "$HERE/unit_tests.cpp" $SRCS "$TINYGPS_SRC/TinyGPS++.cpp" -o "$OUT/unit_tests" -lm
g++ -std=c++17 -Wall -Wextra -I"$HERE/arduino_stub" -I"$TINYGPS_SRC" -x c++ \
  "$HERE/scenario_main.cpp" $SRCS "$SIM" "$TINYGPS_SRC/TinyGPS++.cpp" -o "$OUT/scenario" -lm

"$OUT/unit_tests" | tee "$OUT/unit_test_output.txt"
"$OUT/scenario" | tee "$OUT/scenario_output.txt"

if command -v python3 >/dev/null 2>&1; then
  python3 - "$OUT/unit_test_output.txt" "$OUT/scenario_output.txt" <<'EOF'
import json, sys
count = 0
for path in sys.argv[1:]:
    for line in open(path):
        for marker in ("PACKET: ", "[TELEMETRY] "):
            if line.startswith(marker):
                json.loads(line[len(marker):])
                count += 1
print("JSON check: %d packets are valid JSON" % count)
EOF
fi
