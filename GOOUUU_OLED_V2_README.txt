GOOUUU ESP32-C5 Wardriver + I2C OLED v2 patch
================================================

Fixes in this revision:
1. Uses ESP32-C5 primary I2C controller (TwoWire(0)) for OLED GPIO24/23.
   TwoWire(1) is LP-I2C on ESP32-C5 and rejected GPIO24/23 in the boot log.
2. Disables HAS_BATTERY for the tested GOOUUU board, leaving primary I2C free.
3. Disables HAS_PSRAM because the tested GOOUUU board has 4 MB flash and no usable PSRAM.
4. Initializes the SSD1306 framebuffer even if the OLED is absent, so the original UI's
   direct display.tft calls do not crash a headless/miswired boot.
5. Tries OLED addresses 0x3C and 0x3D.

OLED wiring:
  GND -> GND
  VCC -> 3V3
  SCL -> GPIO23
  SDA -> GPIO24

microSD wiring (small board labelled 3V3):
  CS   -> GPIO10
  MISO -> GPIO2
  MOSI -> GPIO7
  SCK  -> GPIO6
  VCC  -> 3V3
  GND  -> GND

GPS wiring:
  GPS TX -> GPIO14
  GPS RX -> GPIO13
  VCC -> use the input voltage required by the exact GPS breakout (the GY-GPS6MV2 build used 5V)
  GND -> GND

Known-good Arduino IDE settings for the tested GOOUUU board:
  Board: ESP32C5 Dev Module
  USB CDC On Boot: Disabled
  PSRAM: Disabled
  Flash Size: 4MB
  Partition Scheme: Huge APP (3MB No OTA / 1MB SPIFFS)
  Flash Frequency: 40 MHz
  Flash Mode: QIO
  Upload Speed: 460800
  Serial: 115200

IMPORTANT:
Remove any c5_wardriver_*.bin file from the SD card root before booting this custom build.
Huge APP is a No-OTA partition, so an SD update image will report "Not enough space to begin OTA".
