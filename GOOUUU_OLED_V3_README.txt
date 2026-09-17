GOOUUU ESP32-C5 + I2C OLED v3

OLED wiring:
  VCC -> 3V3
  GND -> GND
  SDA -> GPIO24
  SCL -> GPIO23

Display behavior:
  Page 1 (5 seconds): GPS fix/satellites, SD state, scan state, 2.4/5 GHz/BLE counts, totals, log/geofence status.
  Page 2 (5 seconds): local IP, GPS latitude/longitude, active log file, SD state, run mode.
  Pages rotate automatically every 5 seconds.

Known-good Arduino settings for the tested GOOUUU C5:
  Board: ESP32C5 Dev Module
  USB CDC On Boot: Disabled
  PSRAM: Disabled
  Flash Size: 4MB
  Partition Scheme: Huge APP (3MB No OTA / 1MB SPIFFS)
  Flash Frequency: 40 MHz
  Flash Mode: QIO
  Upload Speed: 460800
  Serial: 115200

Important:
  Remove any old firmware .bin file from the SD-card root because Huge APP has no OTA partition.
  This profile disables the unused battery-gauge I2C path so the main I2C controller can drive the OLED on GPIO23/24.
