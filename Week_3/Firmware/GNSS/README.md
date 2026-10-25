# GNSS module

**Files:** `gnss.h`, `gnss.cpp` (recorded NMEA samples: `../../Test_Data/NMEA_Sample_Log.txt`)
**Does:** parses NMEA GGA and RMC, checks the checksum, derives fix quality from the GGA sentence, gates the fix on satellite count and HDOP, reports speed, course and UTC epoch seconds, tracks the age of the last valid fix, keeps the last valid fix as history only, and reports NO_FIX (quality 0, no position or speed) when the criteria are not met or the fix is stale.
**Tested with:** RECORDED sample NMEA lines (parser tests only), 31 checks in `Tests/Test_Suites/tests_gnss.cpp`.
**Not done:** real UART reading from the u-blox MAX-M10S, live time (recorded time is flagged not live), real receiver behaviour.
**Placeholders:** minimum satellites (4), maximum HDOP (5.0), stale-fix limit (3 s). See `Configuration_Notes.md`.
