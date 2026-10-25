# Replay tools (host, run on a PC, not in Wokwi)

All tools use the same code as the firmware. All scenario data is SIMULATED. Placeholder thresholds. Run the commands from this folder.

```
# CSV replay of one scenario (overrides: --jerk, --impact-g X, --window-ms N, --require-orientation)
g++ -std=c++17 -I ../../Firmware -I ../../Firmware/Accident_Detection replay_csv.cpp ../../Firmware/Accident_Detection/accident_detector.cpp ../../Firmware/config.cpp -o replay_csv
./replay_csv ../../Test_Data/Accident_Scenarios/10_rollover_like.csv

# Summary of all scenarios as CSV (this produced ../Scenario_Results.csv)
g++ -std=c++17 -I ../../Firmware -I ../../Firmware/Accident_Detection scenario_summary.cpp ../../Firmware/Accident_Detection/accident_detector.cpp ../../Firmware/Accident_Detection/sim_scenarios.cpp ../../Firmware/config.cpp -o scenario_summary
./scenario_summary > ../Scenario_Results.csv

# Regenerate the scenario CSV files
g++ -std=c++17 -I ../../Firmware/Accident_Detection export_scenarios.cpp ../../Firmware/Accident_Detection/sim_scenarios.cpp -o export_scenarios
./export_scenarios ../../Test_Data/Accident_Scenarios

# Regenerate the mock event payloads
g++ -std=c++17 -I ../../Firmware/SOS_Events gen_sample_events.cpp ../../Firmware/SOS_Events/event_builder.cpp -o gen_sample_events
./gen_sample_events ../../Test_Data/Sample_Events
```
CSV columns: `t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,speed_kmh` (speed: number = valid GPS speed, `NA` = unavailable, empty = no GPS update at this row; lines starting with `#` are comments). Real logged data in the same format can be replayed the same way. Results are not validated accident detection.
