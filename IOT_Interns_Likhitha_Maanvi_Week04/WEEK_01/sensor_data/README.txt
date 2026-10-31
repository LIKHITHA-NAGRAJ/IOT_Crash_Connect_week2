nmea_recorded_sample.txt       Recorded NMEA test sentences - NOT LIVE GPS
                               lines 1-2: valid fix (GGA + RMC)
                               lines 3-4: valid fix, second position
                               lines 5-6: no fix (GGA fix quality 0, RMC status V)
                               line 7   : deliberately wrong checksum (must be rejected)

imu_simulated_test_data.csv    SIMULATED / TEST DATA - not real sensor readings
                               AX/AY/AZ in g, GX/GY/GZ in deg/s,
                               acceleration magnitude = sqrt(AX^2 + AY^2 + AZ^2)
