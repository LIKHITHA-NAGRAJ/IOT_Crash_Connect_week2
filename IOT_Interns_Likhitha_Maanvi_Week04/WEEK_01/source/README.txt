Wokwi project files for the firmware in ../firmware/gps_imu_demo/

  diagram.json   ESP32-S3 board with the Serial Monitor connection
  libraries.txt  library list for Wokwi (TinyGPSPlus)

To run in Wokwi:
  1. Create a new ESP32-S3 project.
  2. Paste ../firmware/gps_imu_demo/gps_imu_demo.ino into sketch.ino.
  3. Put TinyGPSPlus in libraries.txt (or add it in Library Manager).
  4. Start the simulation and open the Serial Monitor (115200 baud).

To run in Arduino IDE: install the "TinyGPSPlus" library by Mikal Hart, select an ESP32-S3 board and upload.
