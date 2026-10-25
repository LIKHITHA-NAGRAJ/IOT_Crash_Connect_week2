# IoT / Firmware: Interface Table, Proposed Pin Map and Requirements

**From:** IoT team (firmware) | **To:** PCB team | **Status: PROPOSAL for PCB review. Nothing here is final or agreed.**
Firmware pins are configurable (`config`), so PCB can change any pin below. Please send back the final map and I will update the firmware configuration.
Everything is software and datasheet-level planning. No hardware has been tested.

Approved components: ESP32-S3-WROOM-1-N8R2 (MCU module), SIMCom A7670C (cellular), u-blox MAX-M10S-00B (GNSS), ST LSM6DSOX (motion sensor), TI LMR33630-Q1 (buck), TI BQ24075 (battery management), 1200 mAh rechargeable lithium (backup).

## 1. Interface summary
| Block | Interface | Direction (from ESP32) | Firmware assumption | Status |
|---|---|---|---|---|
| GNSS (MAX-M10S) | UART, NMEA text | RX from GNSS, TX to GNSS | 3.3 V, 9600 baud default assumed, 1 Hz | To confirm in datasheet |
| Modem (A7670C) | UART + control lines | TX/RX, plus PWRKEY, RESET, STATUS outputs/inputs | AT commands over UART; baud to confirm | **Voltage level to confirm** |
| IMU (LSM6DSOX) | I2C preferred (SPI possible) | Master | I2C 400 kHz, INT1 optional | **I2C vs SPI pending PCB** |
| Charger (BQ24075) | Status signals | Inputs | Charging / power-good read as inputs | Optional, pending design |
| Battery / supply sense | ADC input, supply-present input | Inputs | Report power source and battery level | **Pending: backup switching method** |
| Manual SOS button | GPIO input | Input | Debounced in firmware | **Pending: does it exist?** |
| Programming / debug | USB or UART0, EN, BOOT | n/a | Must be able to flash and reset | PCB to confirm |

## 2. Proposed pin map (ESP32-S3-WROOM-1-N8R2)
| Function | Signal | Proposed GPIO | Notes |
|---|---|---|---|
| GNSS UART | GNSS_TX to ESP32 RX | GPIO18 | 3.3 V |
| GNSS UART | ESP32 TX to GNSS_RX | GPIO17 | |
| Modem UART | MODEM_TXD to ESP32 RX | GPIO16 | Level shifting if the modem I/O is not 3.3 V |
| Modem UART | ESP32 TX to MODEM_RXD | GPIO15 | |
| Modem control | PWRKEY | GPIO4 | Active level and timing from the A7670C datasheet |
| Modem control | RESET | GPIO5 | Optional |
| Modem status | STATUS | GPIO6 | Input; optional |
| Modem status | RI (ring / URC) | GPIO7 | Optional |
| IMU I2C | SDA | GPIO8 | Pull-ups on PCB |
| IMU I2C | SCL | GPIO9 | |
| IMU | INT1 | GPIO10 | Optional (motion interrupt) |
| Charger | CHG status | GPIO11 | Optional, pending |
| Charger | PGOOD | GPIO12 | Optional, pending |
| Supply | Battery sense (divider to ADC) | GPIO1 | Pending design |
| User | SOS button | GPIO13 | Pending: confirm it exists |
| User | Status LED | GPIO14 | Optional |

**Pins to avoid on this module** (public pinout information; please confirm against the Espressif datasheet):
- GPIO0, GPIO3, GPIO45, GPIO46: strapping pins that affect boot.
- GPIO26 to GPIO32: used by the internal flash/PSRAM bus, not available.
- GPIO35 to GPIO37: used by octal PSRAM on other variants. Free on the N8R2, but I suggest avoiding them in case the module variant changes.
- GPIO19 and GPIO20: native USB, if USB is used for programming/debug.
- GPIO43 and GPIO44: default UART0 console.
- GPIO39 to GPIO42: default JTAG, only if debugging is wanted.
- ESP32-S3 GPIO is 3.3 V logic. Do not connect 5 V signals directly.

## 3. Firmware interface requirements (what firmware needs from hardware)
1. **GNSS:** a UART link with NMEA output including GGA (fix quality, satellites, HDOP) and RMC (speed, course, date and time). Update rate about 1 Hz (configurable). GNSS backup supply for faster restarts is a PCB decision.
2. **Modem:** a UART for AT commands; PWRKEY and RESET control from firmware (or a power-cycle path); a STATUS signal so firmware knows whether the modem is on. UART voltage level and any need for flow control to be confirmed. SIM presence handling and the peak-current supply path are PCB items.
3. **IMU:** I2C (or SPI) access to the LSM6DSOX, with the device address defined (SA0 wiring) and, if possible, INT1 routed for motion events. Sampling rate and ranges are firmware decisions to be agreed.
4. **Power information:** a way for firmware to know the vehicle supply is present versus running on the backup battery, and an approximate battery level. How backup switching works is still pending.
5. **Programming and debug:** USB or UART0 access, EN and BOOT control, test points on the programming lines.
6. **Logic levels:** list any signal that is not 3.3 V so firmware and PCB agree on level shifting.
7. **Antennas:** firmware has no direct requirement, but please record the LTE and GNSS antenna types, connectors and placement, and any LTE/GNSS isolation measures.

## 4. Questions back to PCB
1. Final pin map, or edits to the proposal above?
2. A7670C UART voltage level, default baud rate, and whether flow control is needed.
3. IMU bus: I2C or SPI, and the I2C address.
4. Is there a manual SOS button, a status LED, and charger status or battery-sense wiring?
5. How is backup switching done, and can firmware sense which source is active?
6. How will the board be programmed and debugged (USB or UART0)?
7. Antenna details for LTE and GNSS.

## 5. Source notes
- Component choices: team's approved list.
- ESP32-S3 module memory and pin restrictions: public ESP32-S3-WROOM-1 pinout information (verify in the Espressif datasheet).
- GNSS, modem and IMU electrical details: NOT verified here, listed as "to confirm".
