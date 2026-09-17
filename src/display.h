#ifndef display_h
#define display_h

#include <FS.h>
#include <LinkedList.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <functional>

#include "configs.h"
#include "BatteryInterface.h"

extern BatteryInterface battery;

// Compatibility aliases so the rest of the wardriver UI can keep using
// the ST77XX-style color names. The OLED is monochrome, so every visible
// color maps to WHITE and BLACK maps to BLACK.
#ifndef ST77XX_BLACK
  #define ST77XX_BLACK SSD1306_BLACK
  #define ST77XX_WHITE SSD1306_WHITE
  #define ST77XX_RED SSD1306_WHITE
  #define ST77XX_GREEN SSD1306_WHITE
  #define ST77XX_BLUE SSD1306_WHITE
  #define ST77XX_CYAN SSD1306_WHITE
  #define ST77XX_MAGENTA SSD1306_WHITE
  #define ST77XX_YELLOW SSD1306_WHITE
#endif

#ifndef CYAN
  #define CYAN SSD1306_WHITE
#endif

class Display {
  public:
    Display(TwoWire* wire, int sda, int scl, uint8_t addr);
    Adafruit_SSD1306* tft;  // kept as "tft" for compatibility with existing UI code

    void begin();
    void main(uint32_t currentTime);
    void clearScreen();
    void ctrlBacklight(bool on = true);
    void drawCenteredText(String text, bool centerVertically = false);
    void show();
    bool detected() const { return _detected; }

  private:
    TwoWire* _wire;
    int _sda;
    int _scl;
    uint8_t _addr;
    bool _detected = false;
};

#endif
