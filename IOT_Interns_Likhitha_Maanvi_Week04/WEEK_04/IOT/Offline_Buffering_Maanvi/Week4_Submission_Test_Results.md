# Week 4 — IoT Test Results

**Name:** Maanvi Mishra  
**Role:** IoT Intern  
**Project:** Crash Connect — Crash Alert Technology System  
**Testing status:** Simulation and mock-based validation

## 1. Offline Buffering Tests

| Test | Input type | Result | Known issue |
|---|---|---|---|
| Buffer telemetry while offline | Simulated | PASS — records stored in bounded queue | In-memory storage only |
| Buffer-full handling | Simulated | PASS — newest incoming record dropped | Overflow policy requires team approval |
| FIFO delivery | Simulated | PASS — records sent in original order | No physical modem tested |
| Timestamp preservation | Simulated | PASS — original timestamps retained | Device clock accuracy not verified |
| Queue emptying | Mock | PASS — records removed after simulated ACK | No real server ACK |

## 2. Reconnect and Failure Tests

### Exponential Backoff

| Parameter | Value |
|---|---:|
| Initial delay | 1 second |
| Multiplier | 2 |
| Maximum delay | 30 seconds |
| Calculated sequence | 1, 2, 4, 8, 16, 30, 30 seconds |

**Input type:** Simulated

**Result:** PASS — calculated retry delays follow exponential backoff and respect the 30-second maximum.

**Known issue:** Actual waiting intervals and physical network reconnection have not been tested.

### No-Network Scenario

**Input type:** Simulated

**Result:** PASS — the simulation checks the disconnected-network condition and the expected retry policy.

**Known issue:** Real modem connectivity and reconnection have not been verified.

### No-Server Scenario

**Input type:** Mock

**Result:** PASS — mock server failures retain the event until a simulated ACK is received.

**Known issue:** Actual backend availability and server response handling remain unverified.

## 3. Retry and Acknowledgement Tests

| Test | Input type | Result | Known issue |
|---|---|---|---|
| Same event ID across retries | Mock | PASS | No physical modem tested |
| Identical payload across retries | Mock | PASS | Backend payload acceptance not verified |
| Queue removal after ACK | Mock | PASS | Real server acknowledgement not tested |

## 4. Offline-to-Online Demonstration

**Input type:** Simulated records and mock acknowledgements.

Two records were buffered while offline and delivered after simulated connectivity restoration.

| Delivery order | Event ID | Original capture timestamp |
|---|---|---|
| 1 | SIM-001 | 2026-10-29T10:00:00Z |
| 2 | SIM-002 | 2026-10-29T10:00:05Z |

**Observed results:**
- FIFO delivery preserved.
- Original capture timestamps preserved.
- Simulated ACK received for both events.
- Queue emptied after successful simulated delivery.

**Overall result:** PASS.

## 5. Latency Measurement

**Input type:** Simulated event and mock dashboard receipt.

- Start: Immediately before the simulated transmission delay.
- Stop: Immediately after mock dashboard receipt.
- Injected delay: 50 ms.
- Observed elapsed time: 49.58 ms.
- Event ID preserved: Yes.

**Result:** PASS — simulated trigger-to-mock-dashboard measurement.

**Known issue:** This is not actual LTE, backend, or dashboard latency.

## 6. Overall Limitations and Pending Validation

1. No physical LTE modem transmission was performed.
2. Queue storage is currently in memory and is not verified across power loss.
3. Network and server responses were simulated or mocked.
4. Real server acknowledgement and dashboard delivery remain unverified.
5. Firmware integration and shared-interface review remain subject to team approval.

## 7. Conclusion

The implemented offline buffering, retry, exponential-backoff, acknowledgement, and reconnect simulation checks passed their documented test cases. The offline-to-online demonstration verified ordered delivery, original timestamp preservation, and queue removal after simulated acknowledgement.

All results are identified as simulated or mocked. Physical hardware and live backend validation remain pending.