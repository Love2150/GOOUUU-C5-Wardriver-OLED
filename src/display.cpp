#include "display.h"
#include "logger.h"

Display::Display(TwoWire* wire, int sda, int scl, uint8_t addr)
  : _wire(wire), _sda(sda), _scl(scl), _addr(addr) {
  tft = new Adafruit_SSD1306(TFT_WIDTH, TFT_HEIGHT, _wire, OLED_RST);
}

void Display::begin() {
  // GPIO23/24 must use the ESP32-C5 primary I2C controller (TwoWire(0)).
  // TwoWire(1) maps to LP-I2C on C5 and rejects these pins.
  if (!_wire->begin(_sda, _scl, OLED_I2C_FREQ)) {
    Logger::log(WARN_MSG, "OLED I2C bus init failed on SDA=" + String(_sda) + " SCL=" + String(_scl));
    _detected = false;
    return;
  }
  delay(20);

  // Probe 0x3C first and 0x3D as a fallback.
  _wire->beginTransmission(_addr);
  uint8_t err = _wire->endTransmission();
  if (err != 0 && _addr != 0x3D) {
    _wire->beginTransmission(0x3D);
    if (_wire->endTransmission() == 0) {
      _addr = 0x3D;
      err = 0;
    }
  }

  // Allocate the SSD1306 framebuffer even if the panel is absent. The rest of the
  // original UI writes directly through display.tft, so this keeps headless boots
  // safe instead of crashing when a display is unplugged or miswired.
  if (!tft->begin(SSD1306_SWITCHCAPVCC, _addr, false, false)) {
    Logger::log(WARN_MSG, "SSD1306 framebuffer/init failed");
    _detected = false;
    return;
  }

  if (err != 0) {
    Logger::log(WARN_MSG, "OLED not detected at 0x3C or 0x3D; continuing headless");
    _detected = false;
    tft->clearDisplay();
    tft->setTextWrap(false);
    tft->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    tft->setTextSize(1);
    tft->setRotation(2);
    return;
  }

  Logger::log(GUD_MSG, "OLED detected at 0x" + String(_addr, HEX));
  _detected = true;
  tft->clearDisplay();
  tft->setTextWrap(false);
  tft->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  tft->setTextSize(1);
  tft->setRotation(2);
  tft->setCursor(0, 0);
  tft->println("C5 WARDRIVER");
  tft->println("GOOUUU OLED");
  tft->println("OLED: OK");
  tft->display();
}

void Display::drawCenteredText(String text, bool centerVertically) {
  if (!_detected) return;
  tft->setRotation(2);
  tft->setTextSize(1);
  tft->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  tft->setTextWrap(false);

  const uint8_t charWidth = 6;
  const uint8_t charHeight = 8;
  uint16_t textWidth = text.length() * charWidth;
  int16_t x = (TFT_WIDTH > textWidth) ? (TFT_WIDTH - textWidth) / 2 : 0;
  int16_t y = centerVertically ? (TFT_HEIGHT - charHeight) / 2 : tft->getCursorY();
  tft->setCursor(x, y);
  tft->print(text);
  tft->display();
}

void Display::ctrlBacklight(bool on) {
  // SSD1306 modules do not have a separate backlight. Use display on/off.
  if (!_detected) return;
  tft->ssd1306_command(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
}

void Display::clearScreen() {
  if (!_detected) return;
  tft->clearDisplay();
  tft->setCursor(0, 0);
}

void Display::show() {
  if (_detected) tft->display();
}

void Display::main(uint32_t currentTime) {
  (void)currentTime;
}
