# IMU module

**Files:** `imu.h`, `imu.cpp`
**Does:** provides SIMULATED acceleration (g) and gyroscope (deg/s) test values and the acceleration magnitude. The accident detector reads the same kinds of values from simulated scenarios (`../Accident_Detection/sim_scenarios.cpp`).
**Tested with:** SIMULATED values only (legacy telemetry suite and the accident replay).
**Not done / not verified:** no real LSM6DSOX driver, no register configuration, no mounting or axis-mapping calibration.

## To be added by the intern who owns this part (Manvi, per the task sheet)
The following are NOT included here and must not be described as done unless they are added with real files and test evidence:
- LSM6DSOX configuration table (output data rate, measurement range, digital low-pass filter options) with datasheet references
- simulated IMU data source with timestamps
- raw versus moving-average or low-pass filter comparison and traces
- ring buffer for the seconds before an event, and CSV export
- rest-offset estimation, gravity-vector handling, axis mapping
