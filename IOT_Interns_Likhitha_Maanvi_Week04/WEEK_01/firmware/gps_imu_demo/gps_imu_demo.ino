#include <Arduino.h>
#include <TinyGPS++.h>
#include <math.h>

const char *DEVICE_ID = "CRASH_ESP32_01";

const int RECORD_COUNT = 4;

const char *recordLabels[RECORD_COUNT] = {
    "Valid fix (recorded sample)",
    "Valid fix (second recorded sample)",
    "No fix (GGA fix quality 0, RMC status V)",
    "Corrupted sentence (wrong checksum)"};

const char *nmeaRecords[RECORD_COUNT][2] = {
    {"$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n",
     "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n"},
    {"$GPGGA,123520,4807.045,N,01131.012,E,1,08,0.9,545.6,M,46.9,M,,*46\r\n",
     "$GPRMC,123520,A,4807.045,N,01131.012,E,021.9,084.9,230394,003.1,W*6A\r\n"},
    {"$GPGGA,123521,,,,,0,00,99.99,,,,,,*4E\r\n",
     "$GPRMC,123521,V,,,,,,,230394,,*38\r\n"},
    {"$GPGGA,123522,4807.050,N,01131.020,E,1,08,0.9,545.7,M,46.9,M,,*00\r\n",
     nullptr}};

TinyGPSPlus gps;
TinyGPSCustom gpsFixField(gps, "GPGGA", 6);

struct LocationData
{
    String deviceId;
    double latitude;
    double longitude;
    String timestamp;
    bool gpsStatus;
    int fixQuality;
    int satellites;
    double hdop;
    double altitude;
};

LocationData locationData;

void feedNMEA(const char *sentence)
{
    if (sentence == nullptr)
        return;

    while (*sentence != '\0')
    {
        gps.encode(*sentence);
        sentence++;
    }
}

String getGPSTimestamp()
{
    if (!gps.date.isValid() || !gps.time.isValid())
        return "Unavailable";

    int year = gps.date.year();
    if (year >= 2080)
        year -= 100;

    char buffer[24];
    snprintf(
        buffer,
        sizeof(buffer),
        "%04d-%02d-%02dT%02d:%02d:%02dZ",
        year,
        gps.date.month(),
        gps.date.day(),
        gps.time.hour(),
        gps.time.minute(),
        gps.time.second());

    return String(buffer);
}

void printLocationJson()
{
    char json[200];
    snprintf(
        json,
        sizeof(json),
        "{\"device_id\":\"%s\",\"latitude\":%.6f,\"longitude\":%.6f,"
        "\"timestamp\":\"%s\",\"gps_status\":\"VALID\"}",
        locationData.deviceId.c_str(),
        locationData.latitude,
        locationData.longitude,
        locationData.timestamp.c_str());

    Serial.print("Payload    : ");
    Serial.println(json);
}

void processRecord(int index)
{
    Serial.printf("\n--- Record %d: %s ---\n", index + 1, recordLabels[index]);

    uint32_t failedBefore = gps.failedChecksum();

    feedNMEA(nmeaRecords[index][0]);
    feedNMEA(nmeaRecords[index][1]);

    if (gps.failedChecksum() > failedBefore)
    {
        Serial.println("Result     : REJECTED (checksum mismatch)");
        Serial.print("Checksum failures: ");
        Serial.println(gps.failedChecksum());
        return;
    }

    locationData.deviceId = DEVICE_ID;
    locationData.fixQuality = atoi(gpsFixField.value());
    locationData.gpsStatus = gps.location.isValid() && locationData.fixQuality > 0;
    locationData.latitude = gps.location.lat();
    locationData.longitude = gps.location.lng();
    locationData.timestamp = getGPSTimestamp();
    locationData.satellites = gps.satellites.isValid() ? gps.satellites.value() : -1;
    locationData.hdop = gps.hdop.isValid() ? gps.hdop.hdop() : -1.0;
    locationData.altitude = gps.altitude.isValid() ? gps.altitude.meters() : -1.0;

    Serial.print("Device ID  : ");
    Serial.println(locationData.deviceId);

    if (locationData.gpsStatus)
    {
        Serial.print("Latitude   : ");
        Serial.println(locationData.latitude, 6);

        Serial.print("Longitude  : ");
        Serial.println(locationData.longitude, 6);
    }
    else
    {
        Serial.println("Latitude   : INVALID");
        Serial.println("Longitude  : INVALID");
    }

    Serial.print("Timestamp  : ");
    Serial.println(locationData.timestamp);

    Serial.print("GPS Status : ");
    if (locationData.gpsStatus)
        Serial.printf("VALID (fix quality %d)\n", locationData.fixQuality);
    else
        Serial.println("NO FIX");

    Serial.print("Satellites : ");
    if (locationData.satellites >= 0)
        Serial.println(locationData.satellites);
    else
        Serial.println("Unavailable");

    Serial.print("HDOP       : ");
    if (locationData.hdop >= 0)
        Serial.println(locationData.hdop, 2);
    else
        Serial.println("Unavailable");

    if (locationData.gpsStatus)
    {
        Serial.print("Altitude   : ");
        Serial.print(locationData.altitude, 1);
        Serial.println(" m");
        printLocationJson();
    }
    else
    {
        Serial.println("Payload    : not prepared (no valid fix)");
    }
}

void runGPSDemo()
{
    Serial.println("\n================================");
    Serial.println("TASK 1: GPS NMEA PARSING");
    Serial.println("RECORDED DATA - NOT LIVE GPS");
    Serial.println("================================");

    for (int i = 0; i < RECORD_COUNT; i++)
        processRecord(i);

    Serial.println("\nGPS input is recorded test data.");
}

struct IMUData
{
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
};

const int SAMPLES_PER_CASE = 5;
const char *caseNames[3] = {"STATIONARY", "TILT / ROTATION", "MOVEMENT"};

const IMUData imuTestData[3][SAMPLES_PER_CASE] = {
    {{0.02, 0.01, 0.98, 0.5, 0.2, 1.0},
     {0.01, 0.02, 0.99, 0.4, 0.3, 0.9},
     {0.02, 0.00, 1.00, 0.6, 0.1, 1.1},
     {0.01, 0.01, 0.98, 0.5, 0.2, 1.0},
     {0.02, 0.02, 0.99, 0.4, 0.2, 0.8}},
    {{0.35, 0.15, 0.92, 12.0, 5.0, 3.0},
     {0.45, 0.20, 0.88, 18.0, 7.0, 4.0},
     {0.55, 0.25, 0.80, 25.0, 9.0, 5.0},
     {0.45, 0.20, 0.88, 18.0, 7.0, 4.0},
     {0.35, 0.15, 0.92, 12.0, 5.0, 3.0}},
    {{1.20, 0.40, 0.80, 45.0, 20.0, 10.0},
     {1.80, -0.60, 1.10, 90.0, 55.0, 30.0},
     {-2.10, 1.40, 0.40, 150.0, 95.0, 60.0},
     {2.40, -1.60, 1.90, 210.0, 140.0, 85.0},
     {0.90, 0.30, 1.00, 60.0, 30.0, 15.0}}};

IMUData imu;
int testCase = 0;
int sampleIndex = 0;

float calculateAccelerationMagnitude()
{
    return sqrtf(
        imu.ax * imu.ax +
        imu.ay * imu.ay +
        imu.az * imu.az);
}

void runIMUDemo()
{
    imu = imuTestData[testCase][sampleIndex];

    if (sampleIndex == 0)
    {
        Serial.println("\n================================");
        Serial.println("TASK 2: IMU DATA PROCESSING");
        Serial.println("SIMULATED / TEST DATA");
        Serial.printf("Test case: %s\n", caseNames[testCase]);
        Serial.println("================================");
    }

    Serial.printf("\nSample %d/%d\n", sampleIndex + 1, SAMPLES_PER_CASE);
    Serial.printf("AX: %.2f g\n", imu.ax);
    Serial.printf("AY: %.2f g\n", imu.ay);
    Serial.printf("AZ: %.2f g\n", imu.az);
    Serial.printf("GX: %.2f deg/s\n", imu.gx);
    Serial.printf("GY: %.2f deg/s\n", imu.gy);
    Serial.printf("GZ: %.2f deg/s\n", imu.gz);
    Serial.printf("Acceleration Magnitude: %.3f g\n", calculateAccelerationMagnitude());

    sampleIndex++;
    if (sampleIndex >= SAMPLES_PER_CASE)
    {
        sampleIndex = 0;
        testCase = (testCase + 1) % 3;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\nESP32-S3 GPS + IMU MINI DEMO");
    Serial.println("GPS: RECORDED NMEA DATA - NOT LIVE GPS");
    Serial.println("IMU: SIMULATED / TEST DATA");

    runGPSDemo();
}

void loop()
{
    static unsigned long previousMillis = 0;
    const unsigned long interval = 500;

    if (millis() - previousMillis >= interval)
    {
        previousMillis = millis();
        runIMUDemo();
    }
}
