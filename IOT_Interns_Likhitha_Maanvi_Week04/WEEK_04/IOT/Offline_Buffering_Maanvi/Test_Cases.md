# Week 4 — Offline Buffering Test Report

**Name:** Maanvi Mishra  
**Role:** IoT Intern  
**Module:** Offline Telemetry Buffering  
**Status:** Completed — simulation tests passed

## 1. Implementation

Implemented a bounded FIFO queue in `offline_buffer.py` to simulate offline telemetry buffering and delivery after reconnection.

- Default queue capacity: 50 records
- Test queue capacity: 3 records
- Buffer-full policy: Drop the newest incoming record
- Delivery order: First In, First Out (FIFO)
- Timestamp handling: Preserve each record's original capture timestamp
- Queue removal: Remove records after simulated successful delivery

## 2. Test Cases and Results

| Test | Expected result | Actual result |
|---|---|---|
| Buffer while offline | Records are queued | PASS |
| Remain offline | No records are sent | PASS |
| Reconnect | Queued records are sent in FIFO order | PASS |
| Timestamp preservation | Original timestamps remain unchanged | PASS |
| Buffer full | Newest incoming record is rejected | PASS |
| Successful delivery | Queue becomes empty | PASS |

## 3. Observed Test Output

Three records were buffered and subsequently sent in this order:

1. `evt-001` — `2026-10-27T10:00:00Z`
2. `evt-002` — `2026-10-27T10:00:05Z`
3. `evt-003` — `2026-10-27T10:00:10Z`

When the buffer was full, `evt-004` was rejected. The three previously buffered records were delivered in order, and the queue was emptied.

## 4. Limitations

- All tests used simulated records and simulated delivery.
- The queue currently resides in RAM and does not persist through a power loss or restart.
- Physical flash storage, real LTE connectivity, server acknowledgements, and actual device integration have not been tested.
- The implementation must be reviewed before integration with the shared firmware.

## 5. Conclusion

All implemented simulation checks passed. The offline buffering logic demonstrates bounded storage, FIFO delivery, timestamp preservation, and the selected drop-newest overflow policy. Flash-backed persistence and real network delivery remain to be implemented and verified.
## 6. Transmission Latency Test — Simulation

**Test status:** PASS  
**Test type:** Simulated trigger-to-mock-dashboard latency

### Method
- **Start:** Immediately before the simulated transmission delay, after the accident event is created.
- **Stop:** Immediately after the mock dashboard receives the event.
- **Timer:** Python `time.perf_counter()`.
- **Injected delay:** 50 ms.
- **Observed elapsed time:** 49.58 ms.
- **Event ID:** `c32a40b9-bca2-45fe-ae34-869e6c335f54`

### Result
The mock dashboard received the event with the same event ID. The elapsed time was recorded as 49.58 ms, and the event-ID preservation assertion passed.

### Limitations
This result measures a local simulation with an injected delay. It does not represent real LTE transmission, server acknowledgement, backend processing, or actual dashboard rendering latency. A real end-to-end measurement requires an agreed dashboard receipt point and coordinated timing with the Full Stack team.
## 7. Wednesday Transmission and Latency Summary

### Retry and Acknowledgement
**Result:** PASS — simulation

- The same event ID was retained across retry attempts.
- The event payload remained identical across attempts.
- The event was removed from the queue only after a simulated acknowledgement.

### Latency Measurement
**Result:** PASS — simulation

- Start: Immediately before the mock transmission delay.
- Stop: Immediately after mock dashboard receipt.
- Injected delay: 50 ms.
- Observed elapsed time: 49.58 ms.
- This measures local simulated trigger-to-mock-receipt time, not actual LTE or dashboard latency.

### Offline-to-Online Queue
The earlier bounded-buffer test passed FIFO delivery, timestamp preservation, buffer emptying, and drop-newest overflow handling. The current retry test confirms retry and acknowledgement behavior. A combined test showing records enqueued while offline and subsequently flushed in order after reconnection should be captured separately if the existing output does not demonstrate that sequence.

### Limitations
All results are simulation-based. Physical modem behavior, persistent flash storage, server acknowledgement, and real dashboard receipt remain unverified.
## 8. Thursday — Reconnect and Failure Behaviour

**Module:** `reconnect_simulation.py`  
**Test type:** Simulation only

### Reconnect Policy
- Initial retry delay: 1 second
- Backoff multiplier: 2
- Maximum retry delay: 30 seconds
- Calculated sequence: 1, 2, 4, 8, 16, 30, 30 seconds
- Delay calculation tested without waiting for actual intervals.

### Test Results

| Test | Result |
|---|---|
| Backoff calculation and maximum cap | PASS |
| No-network scenario | Simulated |
| No-server scenario | Simulated |
| Event ID unchanged across retry attempts | PASS |
| Event retained until simulated ACK | PASS |
| Offline-to-online FIFO delivery | PASS |
| Original capture timestamps preserved | PASS |
| Queue emptied after simulated ACK | PASS |

### Offline-to-Online Evidence
Two simulated events, `SIM-001` and `SIM-002`, were delivered in FIFO order after connectivity restoration. Their original capture timestamps were preserved, and the queue was emptied after simulated acknowledgements.

### Limitations
The tests use mocked network states and server responses. They do not validate physical modem reconnection, actual retry timing, flash-backed persistence, or live server acknowledgements. The reconnect policy requires review before integration with shared firmware.
