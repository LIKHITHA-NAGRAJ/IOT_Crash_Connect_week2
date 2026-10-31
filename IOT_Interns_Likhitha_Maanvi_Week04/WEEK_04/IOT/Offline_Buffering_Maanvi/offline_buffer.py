
"""
Week 4 - Crash Connect IoT
Offline telemetry buffering simulation.

This is an in-memory simulation of the intended flash-backed queue.
It does not demonstrate persistent storage across power loss.
"""

from collections import deque
from dataclasses import dataclass
from typing import Any, Dict, List


BUFFER_CAPACITY = 50


@dataclass
class TelemetryRecord:
    record_id: str
    captured_at: str
    payload: Dict[str, Any]


class OfflineBuffer:
    def __init__(self, capacity: int = BUFFER_CAPACITY):
        if capacity <= 0:
            raise ValueError("Capacity must be greater than zero")

        self.capacity = capacity
        self._queue = deque()
        self.dropped_newest = 0

    def add(self, record: TelemetryRecord) -> bool:
        """Queue a record. Drop the newest incoming record if full."""
        if len(self._queue) >= self.capacity:
            self.dropped_newest += 1
            print(f"BUFFER FULL: dropped incoming record {record.record_id}")
            return False

        self._queue.append(record)
        print(f"BUFFERED: {record.record_id} captured_at={record.captured_at}")
        return True

    def flush(self, online: bool) -> List[TelemetryRecord]:
        """
        Simulate sending queued records when online.
        Records are removed only after simulated successful delivery.
        """
        sent = []

        if not online:
            print("OFFLINE: records remain buffered")
            return sent

        while self._queue:
            record = self._queue[0]

            # Simulated successful transmission.
            print(
                f"SENT: {record.record_id} "
                f"captured_at={record.captured_at}"
            )
            sent.append(record)
            self._queue.popleft()

        return sent

    @property
    def size(self) -> int:
        return len(self._queue)


def run_demo():
    print("=== OFFLINE BUFFERING SIMULATION ===")

    buffer = OfflineBuffer(capacity=3)

    # Simulate three records captured while offline.
    records = [
        TelemetryRecord("evt-001", "2026-10-27T10:00:00Z",
                        {"event_type": "telemetry", "value": 10}),
        TelemetryRecord("evt-002", "2026-10-27T10:00:05Z",
                        {"event_type": "telemetry", "value": 11}),
        TelemetryRecord("evt-003", "2026-10-27T10:00:10Z",
                        {"event_type": "accident", "value": 12}),
    ]

    print("\n--- Device offline ---")
    for record in records:
        buffer.add(record)

    print(f"Buffered count: {buffer.size}")

    print("\n--- Buffer-full test ---")
    extra = TelemetryRecord(
        "evt-004",
        "2026-10-27T10:00:15Z",
        {"event_type": "telemetry", "value": 13},
    )
    accepted = buffer.add(extra)
    print(f"Incoming record accepted: {accepted}")
    print(f"Buffered count: {buffer.size}")
    print(f"Dropped newest count: {buffer.dropped_newest}")

    print("\n--- Still offline ---")
    buffer.flush(online=False)
    print(f"Buffered count: {buffer.size}")

    print("\n--- Reconnected ---")
    sent = buffer.flush(online=True)

    print("\n--- Verification ---")
    sent_ids = [record.record_id for record in sent]
    expected_ids = ["evt-001", "evt-002", "evt-003"]
    assert sent_ids == expected_ids, "Records were not sent in FIFO order"

    assert [r.captured_at for r in sent] == [
        r.captured_at for r in records
    ], "Capture timestamps changed"

    assert buffer.size == 0, "Buffer was not emptied after successful send"
    assert buffer.dropped_newest == 1, "Buffer-full behavior was incorrect"

    print("PASS: buffered records were sent in FIFO order")
    print("PASS: original capture timestamps were preserved")
    print("PASS: buffer emptied after simulated delivery")
    print("PASS: newest incoming record was dropped when full")
    print("PASS: all checks passed")
    print("NOTE: Simulation only; no physical flash or network was used.")


if __name__ == "__main__":
    run_demo()
