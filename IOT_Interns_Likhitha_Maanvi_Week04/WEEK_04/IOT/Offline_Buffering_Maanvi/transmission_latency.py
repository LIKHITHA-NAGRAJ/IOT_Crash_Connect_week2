
"""
Week 4 - Simulated accident-event transmission latency.
Measures from event trigger to mock dashboard receipt.
No real modem or dashboard is used.
"""

import time
import uuid


def run_latency_test():
    event = {
        "event_id": str(uuid.uuid4()),
        "event_type": "ACCIDENT",
        "timestamp": "2026-10-28T11:00:00Z",
        "confirmation_state": "SIMULATED",
        "simulation": True,
    }

    # Start: accident event is triggered.
    start = time.perf_counter()
    print(f"[SIMULATED TRIGGER] event_id={event['event_id']}")

    # Mock transmission/network delay, for test purposes only.
    simulated_network_delay_seconds = 0.05
    time.sleep(simulated_network_delay_seconds)

    # Stop: mock dashboard receives the event.
    dashboard_received_event = event.copy()
    end = time.perf_counter()

    latency_ms = (end - start) * 1000

    assert dashboard_received_event["event_id"] == event["event_id"]

    print(
        f"[MOCK DASHBOARD RECEIVED] "
        f"event_id={dashboard_received_event['event_id']}"
    )
    print(f"Injected mock delay: {simulated_network_delay_seconds * 1000:.0f} ms")
    print(f"Measured trigger-to-mock-dashboard latency: {latency_ms:.2f} ms")
    print("PASS: Event ID preserved.")
    print("LABEL: SIMULATION ONLY; NOT REAL LTE OR DASHBOARD LATENCY.")


if __name__ == "__main__":
    run_latency_test()
