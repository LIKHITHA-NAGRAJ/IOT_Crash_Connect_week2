import random, math, os, csv

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "scenarios")
os.makedirs(OUT, exist_ok=True)
DT = 20
N = 1000


def base(rng, t, vib=0.03):
    return [rng.uniform(-vib, vib), rng.uniform(-vib, vib), 1.0 + rng.uniform(-vib, vib),
            rng.uniform(-2, 2), rng.uniform(-2, 2), rng.uniform(-2, 2)]


def write(name, rows):
    with open(os.path.join(OUT, name + ".csv"), "w", newline="") as f:
        f.write("t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,gps_speed_kmh,gps_fix\n")
        for r in rows:
            f.write(r + "\n")


def row(t, v, speed, fix=True):
    s = "" if (speed is None or not fix) else "%.1f" % speed
    return "%d,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%s,%d" % (t, v[0], v[1], v[2], v[3], v[4], v[5], s, 1 if fix else 0)


def build(name, seed, fn, fix=True):
    rng = random.Random(seed)
    rows = []
    for i in range(N):
        t = i * DT
        v, speed = fn(rng, t)
        rows.append(row(t, v, speed, fix))
    write(name, rows)


def normal(rng, t):
    return base(rng, t, 0.05), 50.0


def braking(rng, t):
    v = base(rng, t, 0.03)
    sp = 60.0
    if 6000 <= t < 8000:
        v[0] = -0.5
        sp = 60.0 - 40.0 * (t - 6000) / 2000.0
    elif t >= 8000:
        sp = 20.0
    return v, sp


def cornering(rng, t):
    v = base(rng, t, 0.03)
    if 5000 <= t < 8000:
        v[1] = 0.6
        v[5] = 25.0
    return v, 40.0


def pothole(rng, t):
    v = base(rng, t, 0.05)
    if t == 8000:
        v[2] = 4.5
    return v, 40.0


def bump(rng, t):
    v = base(rng, t, 0.05)
    if 8000 <= t < 8060:
        v[2] = 2.5
    return v, 20.0


def knock(rng, t):
    v = base(rng, t, 0.05)
    if 8000 <= t < 8060:
        v[0] = 6.0
    return v, 40.0


def frontal(rng, t):
    v = base(rng, t, 0.03)
    sp = 50.0
    if 8000 <= t < 8080:
        v = [8.0, -1.0, 1.0, 40.0, 20.0, 10.0]
    elif 8080 <= t < 8500:
        v = [0.8, 0.5, 1.1, 150.0, 90.0, 60.0]
        sp = 50.0
    if t >= 9000:
        sp = 20.0
    if t >= 10000:
        sp = 0.0
    return v, sp


def rollover(rng, t):
    v = base(rng, t, 0.03)
    sp = 60.0
    if 8000 <= t < 8060:
        v = [6.0, 3.0, 1.0, 120.0, 200.0, 50.0]
    elif 8060 <= t < 9000:
        v = [0.5, 0.5, 0.5, 200.0, 120.0, 80.0]
    elif t >= 9000:
        v = [rng.uniform(-0.03, 0.03), 1.0, rng.uniform(-0.03, 0.03), rng.uniform(-2, 2), rng.uniform(-2, 2), rng.uniform(-2, 2)]
    if t >= 9500:
        sp = 0.0
    return v, sp


def dropout(rng, t):
    v = base(rng, t, 0.03)
    return v, 30.0


build("normal_driving", 1, normal)
build("hard_braking", 2, braking)
build("sharp_cornering", 3, cornering)
build("pothole_spike", 4, pothole)
build("speed_bump", 5, bump)
build("device_knock_gps_ok", 6, knock)
build("device_knock_no_gps", 6, knock, fix=False)
build("frontal_impact", 7, frontal)
build("frontal_impact_no_gps", 7, frontal, fix=False)
build("rollover", 8, rollover)

rng = random.Random(9)
rows = []
for i in range(N):
    t = i * DT
    v = base(rng, t, 0.03)
    if 8000 <= t < 9000:
        rows.append("%d,nan,nan,nan,nan,nan,nan,30.0,1" % t)
    else:
        rows.append(row(t, v, 30.0))
write("sensor_dropout", rows)

manifest = [
    ("normal_driving", "NONE", "", "Steady driving at 50 km/h, small noise"),
    ("hard_braking", "NONE", "", "0.5 g braking for 2 s, 60 to 20 km/h"),
    ("sharp_cornering", "NONE", "", "0.6 g lateral for 3 s, 25 dps yaw"),
    ("pothole_spike", "NONE", "", "Single 4.5 g sample at 8 s, driving continues"),
    ("speed_bump", "NONE", "", "2.5 g for 60 ms at 20 km/h"),
    ("device_knock_gps_ok", "REJECTED", "", "6 g for 60 ms, vehicle keeps moving at 40 km/h, GPS available"),
    ("device_knock_no_gps", "REJECTED", "", "Same knock without GPS: known false-positive risk, expected to fail"),
    ("frontal_impact", "CONFIRMED", "IMPACT", "8 g for 80 ms at 50 km/h, tumble, stillness, GPS speed to 0"),
    ("frontal_impact_no_gps", "CONFIRMED", "IMPACT", "Same impact with no GPS fix: confirmed from IMU only"),
    ("rollover", "CONFIRMED", "ROLLOVER_SUSPECTED", "6 g, rotation of 90 degrees, stillness on its side"),
    ("sensor_dropout", "NONE", "", "1 s of invalid IMU rows, no event"),
]
with open(os.path.join(OUT, "manifest.csv"), "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["scenario", "expected_decision", "expected_type", "description"])
    w.writerows(manifest)
print("wrote", len(manifest), "scenarios to", OUT)
