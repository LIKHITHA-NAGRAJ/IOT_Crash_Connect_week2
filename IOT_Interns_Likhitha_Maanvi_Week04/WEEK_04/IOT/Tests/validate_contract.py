import json, sys, re

COMMON = ["device_id", "schema_version", "message_type", "sequence_number", "timestamp_utc", "time_source"]
REQUIRED = {
    "telemetry": COMMON + ["data_source", "device_state", "gnss_status", "gnss_fix_quality", "latitude", "longitude", "speed_kmh",
                           "accel_x_g", "accel_y_g", "accel_z_g", "gyro_x_dps", "gyro_y_dps", "gyro_z_dps", "accel_magnitude_g",
                           "battery_mv", "fault_flags"],
    "status": COMMON + ["firmware_version", "uptime_s", "reset_reason", "boot_count", "network_registered", "signal_csq",
                        "signal_dbm", "power_source", "battery_mv"],
    "event": COMMON + ["event_id", "event_type", "description", "latitude", "longitude", "gnss_fix_quality", "location_age_s",
                       "location_source", "accident_type", "confirmation_state", "snapshot"],
}
payloads = []
for line in open(sys.argv[1]):
    m = re.search(r"\[(TELEMETRY|STATUS|EVENT)\] (?:payload )?(\{.*\})\s*$", line)
    if m:
        payloads.append(json.loads(m.group(2)))
problems = []
counts = {}
event_ids = []
for p in payloads:
    t = p.get("message_type")
    counts[t] = counts.get(t, 0) + 1
    for k in REQUIRED.get(t, []):
        if k not in p:
            problems.append("%s missing %s" % (t, k))
    for k, v in p.items():
        if isinstance(v, (dict, list)):
            problems.append("%s field %s is nested" % (t, k))
    if p.get("timestamp_utc") is not None and not isinstance(p["timestamp_utc"], int):
        problems.append("timestamp_utc is not an integer")
    if not isinstance(p.get("gnss_fix_quality", 0), int):
        problems.append("gnss_fix_quality is not an integer")
    if t == "telemetry" and p["gnss_status"] != "FIX":
        if p["latitude"] is not None or p["longitude"] is not None or p["speed_kmh"] is not None:
            problems.append("telemetry without fix carries coordinates or speed")
    if t == "event":
        event_ids.append(p["event_id"])
        if p["event_type"] not in ("accident", "sos"):
            problems.append("bad event_type")
        if p["location_source"] == "NONE" and (p["latitude"] is not None or p["longitude"] is not None):
            problems.append("event with no location carries coordinates")
        if p["location_source"] != "NONE" and p["location_age_s"] is None:
            problems.append("event location without age")
if len(event_ids) != len(set(event_ids)):
    problems.append("duplicate event ids in one run")
print("payloads checked:", counts)
print("unique event ids:", len(set(event_ids)))
print("problems:", len(problems))
for x in problems[:20]:
    print("  -", x)
sys.exit(1 if problems else 0)
