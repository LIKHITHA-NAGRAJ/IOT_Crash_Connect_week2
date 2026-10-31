nmea_recorded_gp_sample.txt   Recorded NMEA test sentences, GPS talker (GP) - NOT LIVE GPS
                              lines 1-2 valid fix | lines 3-4 valid fix, second position
                              lines 5-6 no fix (GGA fix quality 0, RMC status V)
                              line 7 deliberately wrong checksum (must be rejected)
nmea_recorded_gn_sample.txt   Same kind of data with the GN talker (multi-constellation receivers such as
                              the MAX-M10S normally report GN); valid fix, then no fix - NOT LIVE GPS
imu_simulated_test_data.csv   SIMULATED / TEST DATA - not real sensor readings.
                              AX/AY/AZ in g, GX/GY/GZ in deg/s, 4 test cases x 5 samples.
                              The IMPACT_TEST case is only a test pattern for the placeholder threshold.
