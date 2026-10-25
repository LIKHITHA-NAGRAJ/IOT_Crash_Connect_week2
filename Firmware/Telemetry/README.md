# Telemetry packet builder

**Files:** `telemetry.h`, `telemetry.cpp`
**Status:** still the Week 2 NESTED draft packet. It is **NOT aligned to the agreed flat telemetry schema**, because the exact schema was not available. Unavailable values are written as `null`, and recorded data never produces a current timestamp.
**Document:** `Flat_Packet_Compatibility_Notes.md` (what is known, what is missing, the questions for Full Stack and the Technical Lead).
**Tested with:** the legacy suite `Tests/Test_Suites/tests_telemetry_legacy.cpp` (manual review of output).
**Next:** rewrite `telemetry.cpp` to the exact flat schema once confirmed, including integer epoch `timestamp_utc`, `gnss_fix_quality` and `sequence_number`.
