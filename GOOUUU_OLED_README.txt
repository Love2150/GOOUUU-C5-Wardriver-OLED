GOOUUU ESP32-C5 + 0.96-inch I2C OLED custom variant
====================================================

OLED wiring:
  OLED GND -> GOOUUU GND
  OLED VCC -> GOOUUU 3V3
  OLED SCL -> GPIO23
  OLED SDA -> GPIO24

Display assumptions:
  Controller: SSD1306
  Resolution: 128x64
  I2C address: 0x3C, with automatic fallback to 0x3D
  Uses ESP32-C5 secondary I2C controller (TwoWire(1)) so it does not conflict
  with the firmware's existing battery-monitor I2C bus on GPIO4/GPIO5.

Existing peripheral wiring is unchanged:
  SD SCK  -> GPIO6
  SD MISO -> GPIO2
  SD MOSI -> GPIO7
  SD CS   -> GPIO10
  GPS RX  <- GPIO14 (GPS TX -> C5 RX)
  GPS TX  -> GPIO13 (C5 TX -> GPS RX)

Library change:
  Install the Adafruit SSD1306 library in addition to the existing dependencies.

Important:
  This source tree was adapted from the uploaded v2.3.2 repository source.
  It has not been compiled in this environment because the repository does not
  include a reproducible Arduino/PlatformIO build manifest and the toolchain is
  not installed here. Compile/flash-test before relying on it in the field.

V4 change: OLED display rotation changed from 0 to 2 (180 degrees).
