# GOOUUU ESP32-C5 Wardriver OLED Adaptation

## Project overview

This repository is my hardware-specific adaptation of the open-source
[ESP32 Dual Band Wardriver](https://github.com/justcallmekoko/ESP32DualBandWardriver)
created by **Just Call Me Koko (JCMK)**.

The original project supplied the wardriving foundation: ESP32-C5 dual-band Wi-Fi
and BLE collection, GPS-assisted WiGLE logging, microSD storage, uploads, the web
configuration interface, and Solo/Node/Core operation. I used that work as the
starting point and adapted the firmware for the GOOUUU ESP32-C5 hardware I had
available, particularly a 0.96-inch 128x64 SSD1306 I2C OLED instead of the
original 160x80 ST7735 SPI color TFT.

## Thanks and upstream credit

Thank you to **Just Call Me Koko** for creating and releasing the original ESP32
Dual Band Wardriver under the MIT License. This adaptation would not exist
without that project, its radio and logging implementation, and the broader work
that went into making ESP32-C5 wardriving practical.

- Original project: <https://github.com/justcallmekoko/ESP32DualBandWardriver>
- Original creator: Just Call Me Koko
- Upstream license: MIT

The original upstream `README.md` and `LICENSE` are intentionally preserved in
this repository. This document describes my adaptation rather than replacing the
original project's documentation or claiming its underlying work as my own.

## Why I made this adaptation

My GOOUUU ESP32-C5 board and display setup differed from the upstream JCMK host
board configuration:

- I had a 0.96-inch SSD1306 monochrome OLED rather than the ST7735 color TFT.
- The OLED needed I2C rather than SPI.
- The tested board exposed 4 MB of flash and no usable PSRAM.
- The board did not have a supported IP5306 or MAX17048 battery fuel gauge.
- The smaller 128x64 display required a different information layout.
- I wanted the firmware to remain safe when the OLED was missing or miswired.

## Hardware profile

### OLED

| OLED | GOOUUU ESP32-C5 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO24 |
| SCL | GPIO23 |

Display configuration:

- Controller: SSD1306
- Resolution: 128x64
- Interface: I2C
- Primary address: `0x3C`
- Fallback address: `0x3D`
- I2C clock: 400 kHz
- ESP32-C5 controller: `TwoWire(0)`
- Display rotation: 2 (180 degrees)

### microSD

| microSD | GOOUUU ESP32-C5 |
| --- | --- |
| CS | GPIO10 |
| MISO | GPIO2 |
| MOSI | GPIO7 |
| SCK | GPIO6 |
| VCC | 3V3 |
| GND | GND |

### GPS

| GPS | GOOUUU ESP32-C5 |
| --- | --- |
| GPS TX | GPIO14 / C5 RX |
| GPS RX | GPIO13 / C5 TX |
| GND | GND |

Use the input voltage required by the exact GPS breakout. The GY-GPS6MV2 setup
used during this adaptation was powered according to that breakout's
requirements.

## What I changed

### 1. Replaced the ST7735 SPI display with an SSD1306 I2C OLED

Files involved:

- `src/configs.h`
- `src/display.h`
- `src/display.cpp`
- `src/src.ino`

I replaced the upstream 160x80 `Adafruit_ST7735` display implementation with a
128x64 `Adafruit_SSD1306` implementation. The new display object receives a
`TwoWire` bus, SDA/SCL pins, and I2C address instead of an SPI bus and TFT
chip-select/data-command pins.

I retained the existing `display.tft` member name so that the rest of the
inherited UI could continue using its existing drawing calls with fewer invasive
changes.

### 2. Added monochrome compatibility for the inherited color UI

File involved:

- `src/display.h`

The upstream UI uses ST77XX color constants. Because the SSD1306 is monochrome,
I mapped visible ST77XX colors to `SSD1306_WHITE` and black to
`SSD1306_BLACK`. This preserved compatibility while removing the dependency on a
color panel.

### 3. Corrected ESP32-C5 I2C controller selection

Files involved:

- `src/display.cpp`
- `src/src.ino`

My first OLED attempt used `TwoWire(1)`. Hardware testing showed that this maps
to the ESP32-C5 low-power I2C controller, which rejected GPIO23 and GPIO24. I
changed the implementation to `TwoWire(0)`, the primary I2C controller, which
works with those pins.

### 4. Added OLED address detection and fallback

File involved:

- `src/display.cpp`

The display initialization probes `0x3C` first and automatically tries `0x3D`
when necessary. Diagnostic messages are written through the firmware logger so
wiring and address problems are visible over serial and, when enabled, in the SD
debug log.

### 5. Made headless or miswired boots safer

Files involved:

- `src/display.h`
- `src/display.cpp`

The inherited UI accesses the display object directly. I initialize the SSD1306
framebuffer even when the panel does not respond and track whether a physical
display was detected. Drawing flushes and display on/off commands become safe
no-ops when no OLED is present. This allows the wardriver to continue booting
instead of crashing solely because the display is absent or miswired.

### 6. Added explicit SSD1306 framebuffer updates

Files involved:

- `src/display.h`
- `src/display.cpp`
- `src/src.ino`

Unlike the original TFT, the SSD1306 draws into a framebuffer. I added
`Display::show()` and call it from the main firmware loop so UI changes are
transferred to the physical OLED.

### 7. Redesigned the UI for 128x64

File involved:

- `src/ui.cpp`

The upstream 160x80 color dashboard did not fit the smaller monochrome panel. I
reworked the primary statistics view into two compact pages that alternate every
five seconds.

Page 1 shows:

- GPS fix/module state and satellite count
- Scan or wait state
- SD-card status
- Battery level when a supported monitor is present
- Current 2.4 GHz, 5 GHz, and BLE counts
- Running Wi-Fi and BLE totals
- Active log filename or geofence-paused state

Page 2 shows:

- Device information heading
- Local IP address
- GPS latitude and longitude, or GPS waiting/module status
- Active log filename
- SD-card status
- Current Solo, Core, or Node mode

I also changed screen rotation, resized the channel-popularity graph, moved its
labels into the available 64-pixel height, and converted colored graph elements
to monochrome drawing.

### 8. Disabled unsupported battery-monitor support

File involved:

- `src/configs.h`

The tested GOOUUU board did not have a supported IP5306 or MAX17048 fuel gauge.
I left `HAS_BATTERY` undefined. This also prevents the unused battery path from
competing with the OLED for the primary I2C controller.

### 9. Disabled PSRAM for the tested board

File involved:

- `src/configs.h`

The tested board reported 4 MB flash and no usable PSRAM, so I left `HAS_PSRAM`
undefined rather than assuming external memory was available.

### 10. Removed an unnecessary SoftwareSerial include

File involved:

- `src/GpsInterface.h`

The GPS implementation uses `HardwareSerial`, so I removed the unused
`SoftwareSerial.h` include from this hardware-specific build.

## Adaptation revision history

### Initial OLED adaptation

Documented in `GOOUUU_OLED_README.txt`:

- Replaced the SPI TFT with an SSD1306 I2C OLED.
- Assigned SDA to GPIO24 and SCL to GPIO23.
- Added the Adafruit SSD1306 library requirement.
- Rotated the display for the physical installation.
- Initially attempted to isolate the OLED on `TwoWire(1)`.

### V2 hardware correction

Documented in `GOOUUU_OLED_V2_README.txt`:

- Moved the OLED to `TwoWire(0)` after discovering the ESP32-C5 LP-I2C pin
  limitation.
- Disabled the unsupported battery-monitor path.
- Disabled PSRAM for the tested board.
- Added safe behavior for a missing OLED.
- Added `0x3C` to `0x3D` address fallback.
- Recorded known-good Arduino IDE settings.
- Documented the incompatibility between the Huge APP partition layout and SD
  OTA updates.

### V3 UI refinement

Documented in `GOOUUU_OLED_V3_README.txt`:

- Consolidated the final wiring notes.
- Introduced the two-page rotating OLED status layout.
- Recorded the tested GOOUUU board configuration.
- Repeated the warning about SD update binaries when using a no-OTA partition.

The three original revision notes remain in the repository as a record of the
adaptation and troubleshooting process. This document consolidates them into a
single narrative.

## Tested Arduino settings

The GOOUUU configuration recorded during testing was:

```text
Board: ESP32C5 Dev Module
USB CDC On Boot: Disabled
PSRAM: Disabled
Flash Size: 4MB
Partition Scheme: Huge APP (3MB No OTA / 1MB SPIFFS)
Flash Frequency: 40 MHz
Flash Mode: QIO
Upload Speed: 460800
Serial: 115200
```

## Important OTA limitation

`Huge APP (3MB No OTA / 1MB SPIFFS)` has no OTA application partition. The
firmware's SD-card updater requires an OTA-compatible partition layout, so an SD
update under Huge APP reports that there is not enough space to begin OTA.

When using the tested Huge APP configuration:

- Flash firmware over USB.
- Remove old firmware `.bin` files from the SD-card root before booting.
- Do not expect the SD-card OTA updater to work.

If SD-card updates are required, use a compatible OTA partition scheme and
revalidate flash size, application size, and settings storage on the exact board.

## Required display library

In addition to the upstream dependencies, this adaptation uses:

- Adafruit SSD1306
- Adafruit GFX

## What remains from the upstream project

This is an adaptation, not a from-scratch wardriver. Major inherited JCMK
features include:

- ESP32-C5 2.4 GHz and 5 GHz Wi-Fi scanning
- BLE scanning
- GPS-assisted WiGLE logs
- microSD storage
- WiGLE and WDG Wars uploads
- Web-based configuration and file handling
- Geofences and SSID exclusions
- Dock-mode uploads
- Solo, Node, and Core modes
- ESP-NOW communication
- SD update support when an OTA-compatible partition layout is used

For the complete upstream feature documentation and original hardware details,
see the preserved `README.md`.

## License and attribution

The upstream project is distributed under the MIT License. The original
`LICENSE` file and its Just Call Me Koko copyright notice are preserved.

My changes in this repository are modifications to that MIT-licensed project.
Any redistribution should retain the existing copyright and permission notice
and should continue to acknowledge the upstream project appropriately.
