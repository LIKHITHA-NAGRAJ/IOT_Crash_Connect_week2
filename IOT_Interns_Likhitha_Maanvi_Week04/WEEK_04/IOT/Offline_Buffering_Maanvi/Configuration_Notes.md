## Week 4 — Offline Telemetry Buffering

### Implementation
- Module: `Week4_Offline_Buffering/offline_buffer.py`
- Purpose: Simulate bounded local telemetry buffering while the device is offline and ordered delivery after reconnection.
- Default buffer capacity: 50 records.
- Test buffer capacity: 3 records.
- Queue policy: FIFO (First In, First Out).
- Buffer-full policy: Drop the newest incoming record to preserve older unsent records.
- Timestamp handling: Preserve the original capture timestamp for every queued record.

### Simulation Test Results
- Offline records were buffered successfully.
- No records were sent while offline.
- Records were sent in FIFO order after simulated reconnection.
- Original capture timestamps were preserved.
- The buffer-full case rejected the newest incoming record.
- The queue was empty after simulated successful delivery.
- All implemented simulation checks passed.

### Limitations and Pending Work
- Current implementation uses an in-memory queue; data is not persistent across device resets or power loss.
- Delivery is simulated and does not confirm actual LTE or backend communication.
- Flash-backed queue storage, real network acknowledgements, and firmware integration remain pending.
- Coordinate with Likhitha before modifying shared firmware or telemetry files.
- Confirm the agreed telemetry payload and delivery/acknowledgement behavior with the Full Stack team before integration.

**Evidence status:** Simulation results recorded; physical device validation not performed.
## Week 4 — Reconnect and Failure Handling

The reconnect simulation uses exponential backoff with an initial delay of 1 second, multiplier of 2, and maximum delay of 30 seconds. The calculated retry sequence is 1, 2, 4, 8, 16, 30, 30 seconds.

In the simulated no-network case, events should remain queued. In the simulated no-server case, unacknowledged events should remain queued and retain the same event ID across retries. After simulated recovery, queued events are delivered in FIFO order with their original capture timestamps, and removed only after simulated acknowledgement.

**Status:** Simulation checks passed. Real network, modem, and persistent-storage validation remain pending. Confirm the retry policy and shared interfaces with Likhitha before firmware integration.
