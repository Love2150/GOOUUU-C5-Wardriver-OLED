# GOOUUU ESP32-C5 Wardriver OLED

[![Firmware build](https://github.com/Love2150/GOOUUU-C5-Wardriver-OLED/actions/workflows/wardriver_build_parallel.yml/badge.svg)](https://github.com/Love2150/GOOUUU-C5-Wardriver-OLED/actions/workflows/wardriver_build_parallel.yml)
[![Installer manifest](https://github.com/Love2150/GOOUUU-C5-Wardriver-OLED/actions/workflows/build_installer_manifest.yml/badge.svg)](https://github.com/Love2150/GOOUUU-C5-Wardriver-OLED/actions/workflows/build_installer_manifest.yml)

A hardware-specific adaptation of the ESP32-C5 Dual Band Wardriver for a
**GOOUUU ESP32-C5 board with a 0.96-inch 128x64 SSD1306 I2C OLED**.

This repository documents the changes I made while adapting the original
wardriver to the hardware I had available: replacing the color SPI TFT with a
small monochrome OLED, correcting ESP32-C5 I2C behavior, redesigning the display
layout, and accounting for the tested board's flash, PSRAM, and battery-monitor
limitations.

## Credit and thanks

This project is based on the open-source
[ESP32 Dual Band Wardriver](https://github.com/justcallmekoko/ESP32DualBandWardriver)
created by **Just Call Me Koko (JCMK)**.

Thank you to Just Call Me Koko for creating and releasing the original project
under the MIT License. The upstream project provided the wardriving foundation,
including ESP32-C5 dual-band Wi-Fi and BLE collection, GPS-assisted WiGLE logs,
microSD storage, uploads, the web interface, and Solo/Node/Core operation. My
work builds on that foundation and adapts it to my GOOUUU OLED configuration.

I am not claiming the underlying wardriver as an original project of my own.
This repository is a documented hardware adaptation of JCMK's work.

- **Original project:** <https://github.com/justcallmekoko/ESP32DualBandWardriver>
- **Original creator:** Just Call Me Koko
- **Original documentation, preserved unchanged:** [UPSTREAM_README.md](UPSTREAM_README.md)
- **Full account of my modifications:** [GOOUUU_C5_WARDRIVER_ADAPTATION.md](GOOUUU_C5_WARDRIVER_ADAPTATION.md)
- **License:** [MIT](LICENSE)

## What I changed

### SSD1306 OLED conversion

I replaced the upstream 160x80 ST7735 SPI color TFT implementation with an
Adafruit SSD1306 128x64 monochrome I2C OLED implementation.

The display now uses:

```text
Controller: SSD1306
Resolution: 128x64
Interface: I2C
SDA: GPIO24
SCL: GPIO23
Primary address: 0x3C
Fallback address: 0x3D
I2C controller: TwoWire(0)
Rotation: 2 (180 degrees)
```

### ESP32-C5 I2C correction

My initial version attempted to use `TwoWire(1)`. Testing showed that it maps to
the ESP32-C5 low-power I2C controller, which rejected GPIO23 and GPIO24. I moved
the OLED to the primary controller, `TwoWire(0)`.

### Safe headless operation

The OLED driver initializes its framebuffer and tracks whether a physical panel
was detected. Display flush and power commands become safe no-ops when the OLED
is missing or miswired, allowing the rest of the wardriver to continue booting.

### Monochrome compatibility

The inherited UI uses ST77XX color constants. I mapped visible colors to
SSD1306 white and retained the existing `display.tft` interface to minimize
invasive changes to the upstream drawing code.

### 128x64 interface redesign

The primary status display was redesigned as two compact pages that alternate
every five seconds.

**Page 1 — collection status**

- GPS fix/module state and satellite count
- Scan or wait state
- SD-card status
- Battery level when supported
- Current 2.4 GHz, 5 GHz, and BLE counts
- Running Wi-Fi and BLE totals
- Active log or geofence-paused status

**Page 2 — device details**

- Local IP address
- GPS latitude and longitude
- GPS waiting/module status when no fix is available
- Active log filename
- SD-card status
- Solo, Core, or Node mode

The channel-popularity graph was also resized and repositioned for the smaller
screen.

### GOOUUU board configuration

For the tested board, I disabled firmware assumptions that did not match the
hardware:

- `HAS_BATTERY` is left undefined because no supported IP5306 or MAX17048 fuel
  gauge was available.
- `HAS_PSRAM` is left undefined because the board reported 4 MB flash and no
  usable PSRAM.
- An unused `SoftwareSerial.h` include was removed because the GPS implementation
  uses `HardwareSerial`.

### CI updates for the adaptation

The GitHub Actions builds were updated to:

- Use `$GITHUB_WORKSPACE` rather than an upstream-repository-specific runner
  path.
- Install the pinned Adafruit SSD1306 dependency required by the OLED port.
- Compile the ESP32-C5 firmware and build installer metadata successfully in
  this repository.

## Hardware wiring

### OLED

| OLED | GOOUUU ESP32-C5 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO24 |
| SCL | GPIO23 |

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

Use the supply voltage required by the exact GPS breakout rather than assuming
all GPS modules accept the same voltage.

## Tested Arduino settings

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

The tested `Huge APP (3MB No OTA / 1MB SPIFFS)` partition layout has no OTA
application partition. The firmware's SD-card updater requires an OTA-capable
partition layout and will fail with a not-enough-space error under Huge APP.

When using the tested Huge APP layout:

- Flash firmware over USB.
- Remove old firmware `.bin` files from the SD-card root before booting.
- Do not expect SD-card OTA updates to work.

If SD updates are required, choose an OTA-compatible partition layout and
revalidate application size and settings storage on the exact board.

## Documentation

- [Complete adaptation history and technical changes](GOOUUU_C5_WARDRIVER_ADAPTATION.md)
- [Original JCMK README, preserved unchanged](UPSTREAM_README.md)
- [Initial OLED adaptation notes](GOOUUU_OLED_README.txt)
- [V2 hardware correction notes](GOOUUU_OLED_V2_README.txt)
- [V3 OLED interface notes](GOOUUU_OLED_V3_README.txt)

The original README contains the complete upstream feature and usage
documentation, including WiGLE logging, the web interface, uploads, geofences,
dock mode, and Solo/Node/Core behavior.

## Build verification

GitHub Actions verifies:

- Native Solo channel-dwell scheduler tests
- ESP32-C5 firmware compilation
- Firmware artifact generation
- Installer target validation
- Installer manifest unit tests
- Authoritative installer bundle generation

## License

The upstream project is distributed under the MIT License. The original
[LICENSE](LICENSE), including Just Call Me Koko's copyright notice, is preserved.

Redistributions should retain that copyright and permission notice and continue
to acknowledge the upstream project. See
[GOOUUU_C5_WARDRIVER_ADAPTATION.md](GOOUUU_C5_WARDRIVER_ADAPTATION.md) for a
more detailed distinction between inherited functionality and my hardware-specific
changes.
