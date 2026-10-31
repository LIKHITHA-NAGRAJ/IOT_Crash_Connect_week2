
"""
Crash Connect Week 4 - Reconnect Simulation
Tests exponential backoff, no-network, no-server,
and stable event IDs across recovery.

SIMULATION ONLY: no physical modem or server is used.
"""

from dataclasses import dataclass
from typing import List


INITIAL_DELAY_S = 1
BACKOFF_MULTIPLIER = 2
MAX_DELAY_S = 30


def backoff_delays(attempts: int) -> List[int]:
    """Return capped exponential-backoff delays."""
    delays = []
    delay = INITIAL_DELAY_S

    for _ in range(attempts):
        delays.append(delay)
        delay = min(delay * BACKOFF_MULTIPLIER, MAX_DELAY_S)

    return delays


@dataclass
class Event:
    event_id: str
    captured_at: str
    payload: dict


def simulate_no_network():
    print("\n--- NO-NETWORK TEST ---")
    delays = backoff_delays(7)

    print("Network state: DISCONNECTED")
    print("Retry delays (seconds):", delays)
    print("Expected: events remain queued; no send attempted without network.")
    assert delays == [1, 2, 4, 8, 16, 30, 30]
    print("PASS: exponential backoff is capped at 30 seconds.")


def simulate_no_server():
    print("\n--- NO-SERVER TEST ---")
    event = Event(
        event_id="SIM-EVENT-001",
        captured_at="2026-10-29T10:00:00Z",
        payload={"event_type": "ACCIDENT", "simulated": True},
    )

    queued = True
    attempted_event_ids = []

    # Mock server fails twice, then succeeds.
    mock_responses = ["NO_SERVER", "NO_SERVER", "ACK"]

    for attempt, response in enumerate(mock_responses, start=1):
        attempted_event_ids.append(event.event_id)
        print(
            f"Attempt {attempt}: response={response}, "
            f"event_id={event.event_id}"
        )

        if response == "ACK":
            queued = False
            print("Simulated ACK received; event removed from queue.")
            break
        else:
            print("No ACK; event remains queued.")

    assert attempted_event_ids == [
        event.event_id, event.event_id, event.event_id
    ]
    assert queued is False

    print("PASS: event ID stayed unchanged across retries.")
    print("PASS: event remained queued until ACK.")


def simulate_offline_then_online():
    print("\n--- OFFLINE-THEN-ONLINE TEST ---")
    queue = [
        Event("SIM-001", "2026-10-29T10:00:00Z",
              {"value": 10, "simulated": True}),
        Event("SIM-002", "2026-10-29T10:00:05Z",
              {"value": 20, "simulated": True}),
    ]

    original_ids = [event.event_id for event in queue]
    original_timestamps = [event.captured_at for event in queue]

    print("Offline: queued events:", original_ids)
    print("Connectivity restored.")

    delivered_ids = []
    delivered_timestamps = []

    # Mock successful delivery in FIFO order.
    while queue:
        event = queue[0]
        print(
            f"SIMULATED SEND: {event.event_id} "
            f"captured_at={event.captured_at}"
        )
        print(f"SIMULATED ACK: {event.event_id}")

        delivered_ids.append(event.event_id)
        delivered_timestamps.append(event.captured_at)
        queue.pop(0)

    assert delivered_ids == original_ids
    assert delivered_timestamps == original_timestamps
    assert not queue

    print("PASS: FIFO order preserved.")
    print("PASS: original timestamps preserved.")
    print("PASS: queue emptied after simulated ACK.")


if __name__ == "__main__":
    print("=== WEEK 4 RECONNECT TESTS ===")
    print("LABEL: SIMULATION ONLY")

    simulate_no_network()
    simulate_no_server()
    simulate_offline_then_online()

    print("\n=== ALL RECONNECT SIMULATION TESTS COMPLETED ===")
    print("No physical modem, network, or server was tested.")
