# Week 2 Test Results (IoT)

> All inputs are SIMULATED: GPS = recorded sample NMEA (not live), IMU = test values, network = simulated condition.
> These tests check the software structure only. They do NOT validate real GPS, IMU or modem hardware.

**Where run:** Wokwi, ESP32-S3 (esp32-s3-devkitc-1), Arduino framework, Serial Monitor output.
**Build result:** compiled and ran in Wokwi. Full output: `wokwi_serial_output.txt`. Screenshots: `../Screenshots/`.
A PlatformIO build for the ESP32-S3 target was not performed for this firmware.

| # | Test | Expected | Result |
|---|---|---|---|
| 1 | Normal start-up with recorded NMEA | BOOT, SELF_TEST, ACQUIRING_GPS, NETWORK_CONNECTING, OPERATIONAL; packet has parsed lat/lon and time | PASS (simulated) |
| 2 | No GPS fix | location.valid false; latitude, longitude, timestamp_utc null | PASS (simulated) |
| 3 | IMU unavailable | imu.available false; all IMU values null | PASS (simulated) |
| 4 | Movement burst (2.474 g vs placeholder 2.0 g) | OPERATIONAL to EVENT_ACTIVE, event_active true, then back to OPERATIONAL | PASS (simulated) |
| 5 | GPS fix lost, then recovered | OPERATIONAL to DEGRADED to OPERATIONAL | PASS (simulated) |
| 6 | Self-test fails; illegal jump | SELF_TEST to FAULT; FAULT to OPERATIONAL rejected | PASS (simulated) |

## Notes
- Test 2 shows `"device_state":"OPERATIONAL"` while there is no GPS fix. The state was left over from Test 1, and Test 2 only checks null handling. In a real run, losing the fix would move the device to DEGRADED.
- "Packet build: SUCCESS" means the JSON was created. It was NOT sent anywhere.
- The 1994 timestamp comes from the public NMEA sample, not from live GPS time.

## Not tested
- Packet builder with a too-small output buffer (the code returns failure and an empty string, but no Wokwi test was run)
- Anything involving real GPS, IMU, A7670C modem, or a real network
- Timing, timeouts, retries and continuous operation (there is no periodic loop yet)
