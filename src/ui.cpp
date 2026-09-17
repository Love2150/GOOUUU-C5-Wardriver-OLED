#include "ui.h"
#include "ChannelDwell.h"

namespace {
constexpr uint8_t CHANNELS_PER_PAGE = 8;
constexpr uint32_t POPULARITY_PAGE_TIME_MS = 4000;
constexpr uint8_t POPULARITY_GRAPH_BOTTOM = 52;
constexpr uint8_t POPULARITY_MAX_BAR_HEIGHT = 34;
}

void UI::begin() {
  sd_file_menu.list = new LinkedList<MenuNode>();
  action_menu.list  = new LinkedList<MenuNode>();
  mode_menu.list    = new LinkedList<MenuNode>();
  upload_menu.list  = new LinkedList<MenuNode>();
  delete_all_menu.list = new LinkedList<MenuNode>();
  upload_all_menu.list = new LinkedList<MenuNode>();
  mark_geofence_menu.list = new LinkedList<MenuNode>();

  mode_menu.name   = "Mode";
  action_menu.name = "Action";
  upload_menu.name = "Upload";
  delete_all_menu.name = "Delete All?";
  upload_all_menu.name = "Upload All?";
  mark_geofence_menu.name = "Mark Geofence Center?";

  this->buildSDFileMenu();

  action_menu.parentMenu = &sd_file_menu;
  mode_menu.parentMenu   = &sd_file_menu;
  upload_menu.parentMenu = &action_menu;  // Upload is a submenu of Action
  delete_all_menu.parentMenu = &sd_file_menu;
  upload_all_menu.parentMenu = &sd_file_menu;
  mark_geofence_menu.parentMenu = &sd_file_menu;

  // Geofence menu
  this->addNodes(&mark_geofence_menu, "No", ST77XX_WHITE, NULL, 0, [this]() {
    this->current_menu = mark_geofence_menu.parentMenu;
  });
  this->addNodes(&mark_geofence_menu, "Yes", ST77XX_WHITE, NULL, 0, [this]() {
    display.clearScreen();

    // Check to make sure we have GPS
    if (gps.getFixStatus() && gps.getGpsModuleStatus()) {
      bool save_available = false;

      // Look for next available geofence
      for (int i = 0; i < MAX_GEOFENCES; i++) {
        String geoStr = settings.loadSetting<String>("geo_" + String(i));

        // Parse stored JSON geo string
        DynamicJsonDocument geoDoc(256);
        if (!geoStr.isEmpty() && deserializeJson(geoDoc, geoStr) == DeserializationError::Ok) {
          float gLat = geoDoc["lat"];
          float gLon = geoDoc["lon"];
          int gRad = geoDoc["rad"];
          String old_label = geoDoc["label"];
          if (gLat != 0.0 && gLon != 0.0 && old_label != "") {
            Logger::log(STD_MSG, "geo_" + String(i) + " lat: " + String(gLat) + ", label: " + old_label + " exists. Skipping...");
            continue;
          }
          else {
            save_available = true;
            Logger::log(STD_MSG, "geo_" + String(i) + " available. Creating...");
            int rad = (int)(0.10 * 1609.34);     // convert to meters for storage
            String label = "Live Geofence " + String(i);

            DynamicJsonDocument geoDoc(256);
            geoDoc["lat"]   = gps.getLat().toFloat();
            geoDoc["lon"]   = gps.getLon().toFloat();
            geoDoc["rad"]   = rad;
            geoDoc["label"] = label;
            String geoStr;
            serializeJson(geoDoc, geoStr);

            settings.saveSetting<bool>("geo_" + String(i), geoStr);

            wifi_ops.reloadGeofenceCache();

            Logger::log(GUD_MSG, "Saved geofence center \"" + label + "\" as geo_" + String(i));

            display.drawCenteredText("New Geofence Saved", true);

            break;

          }
        }
      }
      if (!save_available) {
        display.drawCenteredText("No available save slots", true);
      }
    }
    else {
      display.drawCenteredText("Need GPS Fix", true);
    }

    delay(2000);

    this->current_menu = &sd_file_menu;
  });

  // Delete all Menu
  this->addNodes(&delete_all_menu, "No", ST77XX_WHITE, NULL, 0, [this]() {
    this->current_menu = delete_all_menu.parentMenu;
  });
  this->addNodes(&delete_all_menu, "Yes", ST77XX_WHITE, NULL, 0, [this]() {
    display.clearScreen();

    display.drawCenteredText("Deleting Logs...");

    buffer.setFileName("");
    this->setupSDFileList();

    for (int i = 0; i < sd_obj.sd_files->size(); i++) {
      if (sd_obj.sd_files->get(i).startsWith("wardrive_") || sd_obj.sd_files->get(i).startsWith("wigle-")) {
        if (sd_obj.removeFile("/" + sd_obj.sd_files->get(i))) {
          Logger::log(STD_MSG, "Removed file: " + sd_obj.sd_files->get(i));
          sd_obj.removeFile("/" + sd_obj.sd_files->get(i) + ".wdg");
          sd_obj.removeFile("/" + sd_obj.sd_files->get(i) + ".wigle");
        }
        else {
          Logger::log(WARN_MSG, "Could not remove file: " + sd_obj.sd_files->get(i));
        }
      }
    }
    display.clearScreen();

    display.drawCenteredText("Logs removed");

    delay(2000);

    this->buildSDFileMenu();

    this->current_menu = &sd_file_menu;
  });

  // Upload all Menu
  this->addNodes(&upload_all_menu, "Back", ST77XX_WHITE, NULL, 0, [this]() {
    this->current_menu = upload_all_menu.parentMenu;
  });
  this->addNodes(&upload_all_menu, "WiGLE", ST77XX_WHITE, NULL, 0, [this]() {
    this->setupSDFileList();
    if (wifi_ops.tryConnectToWiFi()) {
      delay(1000);
      for (int i = 0; i < sd_obj.sd_files->size(); i++) {
        if (sd_obj.sd_files->get(i).startsWith("wardrive_") || sd_obj.sd_files->get(i).startsWith("wigle-")) {
          Logger::log(STD_MSG, "Uploading " + sd_obj.sd_files->get(i) + "...");
          if (wifi_ops.uploadFile("/" + sd_obj.sd_files->get(i), true, WIGLE_UPLOAD)) {
            display.clearScreen();
            display.drawCenteredText("WiGLE OK", true);
          } else {
            display.clearScreen();
            display.drawCenteredText("WiGLE failed", true);
          }
        }
      }
    }
    wifi_ops.deinitWiFi();
    delay(10);
    wifi_ops.initWiFi();
    delay(2000);
    this->current_menu = upload_all_menu.parentMenu;
  });
  this->addNodes(&upload_all_menu, "WDGWars", ST77XX_WHITE, NULL, 0, [this]() {
    this->setupSDFileList();
    if (wifi_ops.tryConnectToWiFi()) {
      delay(1000);
      for (int i = 0; i < sd_obj.sd_files->size(); i++) {
        if (sd_obj.sd_files->get(i).startsWith("wardrive_") || sd_obj.sd_files->get(i).startsWith("wigle-")) {
          Logger::log(STD_MSG, "Uploading " + sd_obj.sd_files->get(i) + "...");
          if (wifi_ops.uploadFile("/" + sd_obj.sd_files->get(i), true, WDG_UPLOAD)) {
            display.clearScreen();
            display.drawCenteredText("WDG OK", true);
          } else {
            display.clearScreen();
            display.drawCenteredText("WDG failed", true);
          }
        }
      }
    }
    wifi_ops.deinitWiFi();
    delay(10);
    wifi_ops.initWiFi();
    delay(2000);
    this->current_menu = upload_all_menu.parentMenu;
  });
  this->addNodes(&upload_all_menu, "Both", ST77XX_WHITE, NULL, 0, [this]() {
    this->setupSDFileList();
    if (wifi_ops.tryConnectToWiFi()) {
      delay(1000);
      for (int i = 0; i < sd_obj.sd_files->size(); i++) {
        if (sd_obj.sd_files->get(i).startsWith("wardrive_") || sd_obj.sd_files->get(i).startsWith("wigle-")) {
          Logger::log(STD_MSG, "Uploading " + sd_obj.sd_files->get(i) + "...");
          if (wifi_ops.uploadFile("/" + sd_obj.sd_files->get(i), true, BOTH_UPLOAD)) {
            display.clearScreen();
            display.drawCenteredText("Upload OK", true);
          } else {
            display.clearScreen();
            display.drawCenteredText("Upload failed", true);
          }
        }
      }
    }
    wifi_ops.deinitWiFi();
    delay(10);
    wifi_ops.initWiFi();
    delay(2000);
    this->current_menu = upload_all_menu.parentMenu;
  });

  this->addNodes(&action_menu, "Back", ST77XX_WHITE, NULL, 0, [this]() {
    this->current_menu = action_menu.parentMenu;
  });

  // Upload opens submenu
  this->addNodes(&action_menu, "Upload", ST77XX_WHITE, NULL, 0, [this]() {
    this->current_menu = &upload_menu;
  });

  this->addNodes(&action_menu, "Delete", ST77XX_WHITE, NULL, 0, [this]() {
    if ("/" + sd_obj.selected_file_name == buffer.getFileName())
      buffer.setFileName("");

    if (sd_obj.removeFile("/" + sd_obj.selected_file_name)) {
      Logger::log(STD_MSG, "Removed file: " + sd_obj.selected_file_name);
      display.clearScreen();
      display.drawCenteredText("File removed", true);
    } else {
      Logger::log(STD_MSG, "Could not remove file");
      display.clearScreen();
      display.drawCenteredText("Could not remove file", true);
    }
    delay(2000);
    this->buildSDFileMenu();
    this->current_menu = &sd_file_menu;
  });

  // Upload Menu
  this->addNodes(&upload_menu, "Back", ST77XX_WHITE, NULL, 0, [this]() {
    this->current_menu = upload_menu.parentMenu;
  });
  this->addNodes(&upload_menu, "WiGLE", ST77XX_WHITE, NULL, 0, [this]() {
    if (wifi_ops.tryConnectToWiFi()) {
      delay(1000);
      if (wifi_ops.uploadFile("/" + sd_obj.selected_file_name, true, WIGLE_UPLOAD)) {
        display.clearScreen();
        display.drawCenteredText("WiGLE OK", true);
      } else {
        display.clearScreen();
        display.drawCenteredText("WiGLE failed", true);
      }
    }
    wifi_ops.deinitWiFi();
    delay(10);
    wifi_ops.initWiFi();
    delay(2000);
    this->current_menu = upload_menu.parentMenu;
  });
  this->addNodes(&upload_menu, "WDGWars", ST77XX_WHITE, NULL, 0, [this]() {
    if (wifi_ops.tryConnectToWiFi()) {
      delay(1000);
      if (wifi_ops.uploadFile("/" + sd_obj.selected_file_name, true, WDG_UPLOAD)) {
        display.clearScreen();
        display.drawCenteredText("WDG OK", true);
      } else {
        display.clearScreen();
        display.drawCenteredText("WDG failed", true);
      }
    }
    wifi_ops.deinitWiFi();
    delay(10);
    wifi_ops.initWiFi();
    delay(2000);
    this->current_menu = upload_menu.parentMenu;
  });
  this->addNodes(&upload_menu, "Both", ST77XX_WHITE, NULL, 0, [this]() {
    if (wifi_ops.tryConnectToWiFi()) {
      delay(1000);
      if (wifi_ops.uploadFile("/" + sd_obj.selected_file_name, true, BOTH_UPLOAD)) {
        display.clearScreen();
        display.drawCenteredText("Upload OK", true);
      } else {
        display.clearScreen();
        display.drawCenteredText("Upload failed", true);
      }
    }
    wifi_ops.deinitWiFi();
    delay(10);
    wifi_ops.initWiFi();
    delay(2000);
    this->current_menu = upload_menu.parentMenu;
  });

  // Mode Menu
  this->addNodes(&mode_menu, "Back", ST77XX_WHITE, NULL, 0, [this]() {
    this->current_menu = mode_menu.parentMenu;
  });
  this->addNodes(&mode_menu, "Solo", ST77XX_WHITE, NULL, 0, [this]() {
    wifi_ops.run_mode = SOLO_MODE;
    this->current_menu = mode_menu.parentMenu;
    display.clearScreen();
    display.drawCenteredText("Mode set", true);
    delay(2000);
  });
  this->addNodes(&mode_menu, "Core", ST77XX_WHITE, NULL, 0, [this]() {
    wifi_ops.run_mode = CORE_MODE;
    this->current_menu = mode_menu.parentMenu;
    display.clearScreen();
    display.drawCenteredText("Mode set", true);
    wifi_ops.startESPNow();
    delay(2000);
  });
  this->addNodes(&mode_menu, "Node", ST77XX_WHITE, NULL, 0, [this]() {
    wifi_ops.run_mode = NODE_MODE;
    this->current_menu = mode_menu.parentMenu;
    display.clearScreen();
    display.drawCenteredText("Mode set", true);
    wifi_ops.startESPNow();
    delay(2000);
  });


  this->current_menu = &sd_file_menu;
  this->init_time    = millis();
}

void UI::printFirmwareVersion() {
  display.tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display.tft->setCursor(0, 0);
  display.tft->print(FIRMWARE_VERSION);
}

void UI::printBatteryLevel(int8_t batteryLevel) {
  display.tft->setRotation(2);
  display.tft->setTextSize(1);
  display.tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);

  char buf[12];
  snprintf(buf, sizeof(buf), "Bat: %d%%", batteryLevel);

  uint8_t  charWidth = 6;
  uint16_t textWidth = (strlen(buf) + 5) * charWidth;
  uint16_t x         = TFT_WIDTH - textWidth - 2;

  display.tft->setCursor(x, 0);
  if (sd_obj.supported)
    display.tft->setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  else
    display.tft->setTextColor(ST77XX_RED, ST77XX_BLACK);
  display.tft->print("SD");
  display.tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  if (battery.i2c_supported) {
    display.tft->print(" | ");
    display.tft->print(buf);
  }
}

// ============================================================
// setDisplayMode — clean mode transition helper
// Resets incognito state, restores backlight, forces redraw
// ============================================================
void UI::setDisplayMode(uint8_t new_mode) {
  if (this->incognito_counting) {
    this->incognito_counting = false;
    display.ctrlBacklight(true);
  }
  this->stat_display_mode      = new_mode;
  this->last_stat_display_mode = 255;
  this->last_mode_change_ms    = millis();
  this->lastUpdateTime         = 0;
  if (new_mode == CHANNEL_POPULARITY) {
    this->popularity_page = 0;
    this->popularity_page_started_ms = millis();
  }
  if (new_mode == SD_FILES)
    this->buildSDFileMenu();
  if (new_mode != SD_FILES && new_mode != INCOGNITO)
    display.tft->fillScreen(ST77XX_BLACK);
}

void UI::drawChannelPopularity(uint32_t currentTime, bool do_now) {
  if (wifi_ops.run_mode != SOLO_MODE) {
    if ((currentTime - lastUpdateTime < UI_UPDATE_TIME) && (!do_now)) return;
    lastUpdateTime = currentTime;
    display.clearScreen();
    display.tft->setRotation(2);
    display.tft->setTextWrap(false);
    display.tft->setTextSize(1);
    display.tft->setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    display.tft->setCursor(19, 24);
    display.tft->print("CHANNEL POPULARITY");
    display.tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    String mode_label = wifi_ops.run_mode == CORE_MODE ? "CORE MODE" : "NODE MODE";
    display.tft->setCursor((TFT_WIDTH - mode_label.length() * 6) / 2, 40);
    display.tft->print(mode_label);
    display.tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    display.tft->setCursor(10, 56);
    display.tft->print("Available in SOLO mode");
    return;
  }

  const size_t channel_count = wifi_ops.getSoloChannelCount();
  const uint8_t page_count =
    (channel_count + CHANNELS_PER_PAGE - 1) / CHANNELS_PER_PAGE;
  bool page_changed = false;
  if ((currentTime - popularity_page_started_ms >= POPULARITY_PAGE_TIME_MS) &&
      page_count > 0) {
    popularity_page = (popularity_page + 1) % page_count;
    popularity_page_started_ms = currentTime;
    page_changed = true;
  }

  if ((currentTime - lastUpdateTime < UI_UPDATE_TIME) && (!do_now) &&
      (!page_changed)) return;
  lastUpdateTime = currentTime;

  display.clearScreen();
  display.tft->setRotation(2);
  display.tft->setTextWrap(false);
  display.tft->setTextSize(1);
  display.tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display.tft->setCursor(0, 0);
  display.tft->print("CHANNEL POPULARITY");

  char page_label[6];
  snprintf(page_label, sizeof(page_label), "%u/%u", popularity_page + 1, page_count);
  display.tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display.tft->setCursor(TFT_WIDTH - strlen(page_label) * 6, 0);
  display.tft->print(page_label);
  display.tft->drawFastHLine(0, 9, TFT_WIDTH, ST77XX_WHITE);

  const uint16_t peak = wifi_ops.getPeakSoloChannelPopularity();
  const size_t first_channel = popularity_page * CHANNELS_PER_PAGE;
  const uint8_t slot_width = TFT_WIDTH / CHANNELS_PER_PAGE;

  for (uint8_t slot = 0; slot < CHANNELS_PER_PAGE; slot++) {
    const size_t index = first_channel + slot;
    if (index >= channel_count) break;

    const uint8_t channel = wifi_ops.getSoloChannel(index);
    const uint16_t popularity = wifi_ops.getSoloChannelPopularity(index);
    const uint8_t bar_height = calculatePopularityBarHeight(
      popularity, peak, POPULARITY_MAX_BAR_HEIGHT);
    const int16_t x = slot * slot_width + 4;
    const uint16_t color = channel <= 14 ? ST77XX_CYAN : ST77XX_WHITE;

    if (bar_height > 0)
      display.tft->fillRect(x, POPULARITY_GRAPH_BOTTOM - bar_height,
                            slot_width - 8, bar_height, color);
    else
      display.tft->drawFastHLine(x, POPULARITY_GRAPH_BOTTOM,
                                 slot_width - 8, ST77XX_WHITE);

    String channel_label = String(channel);
    display.tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    display.tft->setCursor(slot * slot_width +
      (slot_width - channel_label.length() * 6) / 2, 54);
    display.tft->print(channel_label);
  }

  if (peak == 0) {
    display.tft->setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    if (wifi_ops.in_geofence) {
      display.tft->setCursor(56, 32);
      display.tft->print("GEOFENCE");
    }
    else {
      display.tft->setCursor(43, 32);
      display.tft->print("MEASURING...");
    }
  }
}

// ============================================================
// Screen 1 — new large-format stats display
// Layout for 160x80px:
//   y=0  : GPS status + battery % + scan status  (size 1)
//   y=19 : divider
//   y=21 : 2.4GHz / 5GHz / BLE labels            (size 1)
//   y=30 : big counts                             (size 2, 16px tall)
//   y=47 : divider
//   y=50 : NET / BLE totals                       (size 2)
//   y=71 : geofence label (only when inside zone) (size 1)
// ============================================================
void UI::drawStatsNew(uint32_t currentTime, uint32_t count2g4, uint32_t count5g,
                      uint32_t bleCount, int gpsSats, int8_t batteryLevel, bool do_now) {

  if ((currentTime - lastUpdateTime < UI_UPDATE_TIME) && (!do_now)) return;
  lastUpdateTime = currentTime;

  // Alternate between two compact OLED pages every 5 seconds.
  // Page 0 = live scan/GPS/SD counters
  // Page 1 = IP, GPS coordinates and active log file
  const uint8_t oledPage = (currentTime / 5000UL) % 2;

  display.clearScreen();
  display.tft->setRotation(2);
  display.tft->setTextWrap(false);
  display.tft->setTextSize(1);
  display.tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);

  if (oledPage == 0) {
    // ---- PAGE 1: live wardriving status ----
    display.tft->setCursor(0, 0);
    display.tft->print("GPS:");
    if (gps.getFixStatus()) {
      display.tft->print("FIX ");
      display.tft->print(gpsSats);
      display.tft->print(" SAT");
    } else if (gps.getGpsModuleStatus()) {
      display.tft->print("NO FIX");
    } else {
      display.tft->print("NOT FOUND");
    }

    display.tft->setCursor(82, 0);
    display.tft->print(wifi_ops.getCurrentScanMode() == WIFI_WARDRIVING ? "SCAN" : "WAIT");

    display.tft->setCursor(0, 10);
    display.tft->print("SD:");
    display.tft->print(sd_obj.supported ? "OK" : "ERR");
    if (battery.i2c_supported) {
      display.tft->setCursor(70, 10);
      display.tft->print("BAT:");
      display.tft->print(batteryLevel);
      display.tft->print('%');
    }

    display.tft->drawFastHLine(0, 20, TFT_WIDTH, ST77XX_WHITE);

    display.tft->setCursor(0, 24);
    display.tft->print("2.4:");
    display.tft->print(count2g4);
    display.tft->setCursor(45, 24);
    display.tft->print("5G:");
    display.tft->print(count5g);
    display.tft->setCursor(82, 24);
    display.tft->print("BLE:");
    display.tft->print(bleCount);

    display.tft->setCursor(0, 35);
    display.tft->print("NET TOTAL:");
    display.tft->print(wifi_ops.getTotalNetCount());

    display.tft->setCursor(0, 45);
    display.tft->print("BLE TOTAL:");
    display.tft->print(wifi_ops.getTotalBLECount());

    display.tft->setCursor(0, 56);
    if (wifi_ops.in_geofence) {
      display.tft->print("GEOFENCE PAUSED");
    } else if (buffer.getFileName().length()) {
      String f = buffer.getFileName();
      if (f.length() > 20) f = f.substring(0, 20);
      display.tft->print(f);
    } else {
      display.tft->print("NO ACTIVE LOG");
    }
  }
  else {
    // ---- PAGE 2: network + GPS details ----
    display.tft->setCursor(0, 0);
    display.tft->print("C5 WARDRIVER INFO");
    display.tft->drawFastHLine(0, 9, TFT_WIDTH, ST77XX_WHITE);

    display.tft->setCursor(0, 13);
    display.tft->print("IP:");
    if (WiFi.status() == WL_CONNECTED) {
      display.tft->print(WiFi.localIP().toString());
    } else {
      display.tft->print("192.168.4.1");
    }

    display.tft->setCursor(0, 23);
    if (gps.getFixStatus()) {
      String lat = gps.getLat();
      String lon = gps.getLon();
      if (lat.length() > 17) lat = lat.substring(0, 17);
      if (lon.length() > 17) lon = lon.substring(0, 17);
      display.tft->print("LAT:");
      display.tft->print(lat);
      display.tft->setCursor(0, 33);
      display.tft->print("LON:");
      display.tft->print(lon);
    } else {
      display.tft->print(gps.getGpsModuleStatus() ? "GPS waiting for fix" : "GPS module missing");
      display.tft->setCursor(0, 33);
      display.tft->print("SAT:");
      display.tft->print(gpsSats);
    }

    display.tft->setCursor(0, 44);
    display.tft->print("LOG:");
    String f = buffer.getFileName();
    if (!f.length()) f = "none";
    if (f.length() > 17) f = f.substring(0, 17);
    display.tft->print(f);

    display.tft->setCursor(0, 55);
    display.tft->print("SD:");
    display.tft->print(sd_obj.supported ? "OK" : "ERR");
    display.tft->print("  MODE:");
    if (wifi_ops.run_mode == SOLO_MODE) display.tft->print("SOLO");
    else if (wifi_ops.run_mode == CORE_MODE) display.tft->print("CORE");
    else display.tft->print("NODE");
  }

  display.show();
}

void UI::updateStats(uint32_t currentTime, uint32_t wifiCount, uint32_t count2g4,
                     uint32_t count5g, uint32_t bleCount, int gpsSats,
                     int8_t batteryLevel, bool do_now) {

  if ((currentTime - lastUpdateTime < UI_UPDATE_TIME) && (!do_now)) return;
  lastUpdateTime = currentTime;

  display.clearScreen();

  display.tft->setRotation(2);
  display.tft->setTextWrap(false);

  display.tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display.tft->setTextSize(1);

  this->printFirmwareVersion();
  this->printBatteryLevel(batteryLevel);

  display.tft->setCursor(0, 0);
  for (int i = 0; i < 2; i++) display.tft->println();

  if (wifi_ops.getCurrentScanMode() == WIFI_STANDBY)
    display.tft->println("Status: STANDBY ");
  else if (wifi_ops.getCurrentScanMode() == WIFI_WARDRIVING)
    display.tft->println("Status: SCANNING ");

  if (sd_obj.supported)
    display.tft->println("File: " + buffer.getFileName() + "   ");
  if (wifi_ops.run_mode == CORE_MODE)
    display.tft->println("Nodes: " + String(wifi_ops.getNodeCount()) + "   ");

  display.tft->println();

  display.tft->print("2.4GHz: ");
  display.tft->print(String(count2g4) + "   ");
  display.tft->print(" | ");
  display.tft->print("5GHz: ");
  display.tft->println(String(count5g) + "   ");

  display.tft->print("BLE: ");
  display.tft->print(String(bleCount) + "   ");
  display.tft->print(" | GPS Sats: ");
  display.tft->println(gpsSats > 0 ? String(gpsSats) + " " : "No Fix");

  display.tft->println();

  display.tft->setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  display.tft->print("Total Nets: ");
  display.tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display.tft->println(String(wifi_ops.getTotalNetCount()) + "   ");
  display.tft->setTextColor(CYAN, ST77XX_BLACK);
  display.tft->print("Total BLE: ");
  display.tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display.tft->println(String(wifi_ops.getTotalBLECount()) + "   ");
}

void UI::setupSDFileList() {
  sd_obj.sd_files->clear();
  delete sd_obj.sd_files;
  sd_obj.sd_files = new LinkedList<String>();
  sd_obj.listDirToLinkedList(sd_obj.sd_files, "/", ".log");
}

void UI::buildSDFileMenu() {
  if (sd_obj.supported) {
    this->setupSDFileList();

    sd_file_menu.list->clear();
    delete sd_file_menu.list;
    sd_file_menu.list = new LinkedList<MenuNode>();
    sd_file_menu.name = "Logs";
    sd_file_menu.selected = 0;
    sd_file_menu.scroll_offset = 0;

    this->addNodes(&sd_file_menu, "Back", ST77XX_WHITE, NULL, 0, [this]() {
      this->setDisplayMode(STATS_NEW);
      if (buffer.getFileName() == "") {
        Logger::log(STD_MSG, "Active log file was deleted. Creating new one...");
        wifi_ops.startLog(LOG_FILE_NAME);
        Logger::log(STD_MSG, "New log file: " + buffer.getFileName());
      }
      this->hard_refresh = true;
    });

    this->addNodes(&sd_file_menu, "Delete Wardrive Logs", ST77XX_WHITE, NULL, 0, [this]() {
      this->current_menu = &delete_all_menu;
    });

    this->addNodes(&sd_file_menu, "Upload All", ST77XX_WHITE, NULL, 0, [this]() {
      this->current_menu = &upload_all_menu;
    });

    this->addNodes(&sd_file_menu, "Mark New Geofence", ST77XX_WHITE, NULL, 0, [this]() {
      this->current_menu = &mark_geofence_menu;
    });

    this->addNodes(&sd_file_menu, "Mode", ST77XX_WHITE, NULL, 0, [this]() {
      this->current_menu = &mode_menu;
    });

    for (int i = 0; i < sd_obj.sd_files->size(); i++) {
      File current_file = sd_obj.getFile("/" + sd_obj.sd_files->get(i));
      if (sd_obj.sd_files->get(i).startsWith("wardrive_") || sd_obj.sd_files->get(i).startsWith("wigle-")) {
        this->addNodes(&sd_file_menu, sd_obj.sd_files->get(i), ST77XX_WHITE, NULL, 0, [this, i]() {
          sd_obj.selected_file_name = sd_obj.sd_files->get(i);
          Logger::log(STD_MSG, sd_obj.sd_files->get(i) + " selected");
          this->current_menu = &action_menu;
        }, current_file.size());
      }
    }

    Logger::log(STD_MSG, "Built SD file menu with " + (String)sd_obj.sd_files->size() + " files");
  } else {
    Logger::log(WARN_MSG, "SD Card not detected. Skipping menu creation...");
  }
}

void UI::addNodes(Menu * menu, String name, uint8_t color, Menu * child, int place,
                  std::function<void()> callable, uint32_t size, bool selected, String command) {
  menu->list->add(MenuNode{name, false, color, place, selected, callable, size});
}

void UI::drawCurrentMenu() {
  if (!current_menu || current_menu->list->size() == 0) return;

  const uint8_t max_visible_items = 7;
  const uint8_t header_height     = 8;

  display.tft->setRotation(2);
  display.tft->fillScreen(ST77XX_BLACK);
  display.tft->setTextSize(1);
  display.tft->setTextWrap(false);

  display.tft->setTextColor(ST77XX_WHITE);
  display.tft->setCursor(0, 0);
  display.tft->println(current_menu->name);

  if (current_menu->selected < current_menu->scroll_offset)
    current_menu->scroll_offset = current_menu->selected;
  else if (current_menu->selected >= current_menu->scroll_offset + max_visible_items)
    current_menu->scroll_offset = current_menu->selected - max_visible_items + 1;

  for (int i = 0; i < max_visible_items; i++) {
    int item_index = current_menu->scroll_offset + i;
    if (item_index >= current_menu->list->size()) break;

    MenuNode node = current_menu->list->get(item_index);
    int y = header_height + i * 8;

    if (item_index == current_menu->selected) {
      display.tft->setTextColor(ST77XX_BLACK, ST77XX_WHITE);
      display.tft->setCursor(0, y);
      display.tft->print("> ");
    } else {
      display.tft->setTextColor(node.color, ST77XX_BLACK);
      display.tft->setCursor(0, y);
      display.tft->print("  ");
    }
    display.tft->print(node.name);

    String sizeStr = "";
    if (node.fileSize > 0) {
      sizeStr  = String((node.fileSize + 1023) / 1024);
      sizeStr += " KB";
    }
    int xRightAlign = TFT_WIDTH - sizeStr.length() * 6;
    display.tft->setCursor(xRightAlign, y);
    display.tft->print(sizeStr);
  }

  if (current_menu->scroll_offset > 0) {
    display.tft->setCursor(TFT_WIDTH - 10, header_height);
    display.tft->setTextColor(ST77XX_WHITE);
    display.tft->print("^");
  }
  if (current_menu->scroll_offset + max_visible_items < current_menu->list->size()) {
    display.tft->setCursor(TFT_WIDTH - 10, header_height + (max_visible_items - 1) * 8);
    display.tft->setTextColor(ST77XX_WHITE);
    display.tft->print("v");
  }
}

void UI::handleMenuNavigation() {
  if (!current_menu || current_menu->list->size() == 0) return;

  int list_size = current_menu->list->size();

  if (u_btn.justPressed()) {
    if (current_menu->selected == 0)
      current_menu->selected = list_size - 1;
    else
      current_menu->selected--;
    drawCurrentMenu();
  }

  if (d_btn.justPressed()) {
    current_menu->selected = (current_menu->selected + 1) % list_size;
    drawCurrentMenu();
  }

  if (c_btn.justPressed()) {
    MenuNode node = current_menu->list->get(current_menu->selected);
    if (node.callable) node.callable();
    drawCurrentMenu();
  }
}

void UI::doHardRefresh() {
  if (this->hard_refresh) {
    Logger::log(STD_MSG, "Hard-refreshing display...");
    display.clearScreen();
    this->hard_refresh = false;
  }
}

void UI::main(uint32_t currentTime) {

  // Handle dock departure display reset
  extern bool g_force_display_redraw;
  if (g_force_display_redraw) {
    this->last_stat_display_mode = 255;
    this->lastUpdateTime         = 0;
    g_force_display_redraw       = false;
    display.tft->fillScreen(ST77XX_BLACK);
  }

  // Don't draw stats while docked — dock mode manages its own display
  if (wifi_ops.isDocked())
    return;

  bool in_stats = (this->stat_display_mode != SD_FILES);

  if (in_stats) {

    // ---- Screen 3: Incognito ----
    if (this->stat_display_mode == INCOGNITO) {

      if (!this->incognito_counting) {
        this->incognito_counting = true;
        this->incognito_start_ms = currentTime;
        this->incognito_last_sec = -1;
        display.tft->fillScreen(ST77XX_BLACK);
        this->last_stat_display_mode = INCOGNITO;
      }

      uint32_t elapsed  = currentTime - this->incognito_start_ms;
      int      secs_rem = (elapsed < 5000) ? (int)(5 - elapsed / 1000) : 0;

      if (elapsed < 5000) {
        if (secs_rem != this->incognito_last_sec) {
          this->incognito_last_sec = secs_rem;
          display.tft->fillScreen(ST77XX_BLACK);
          display.tft->setTextSize(1);
          display.tft->setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
          uint16_t lblX = (TFT_WIDTH - 14 * 6) / 2;
          display.tft->setCursor(lblX > 0 ? lblX : 0, 26);
          display.tft->print("INCOGNITO MODE");
          display.tft->setTextSize(3);
          display.tft->setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
          char buf[3];
          snprintf(buf, sizeof(buf), "%d", secs_rem);
          display.tft->setCursor((TFT_WIDTH - 18) / 2, 44);
          display.tft->print(buf);
        }
      } else {
        if (this->incognito_counting) {
          display.ctrlBacklight(false);
          display.tft->fillScreen(ST77XX_BLACK);
        }
      }

      // Any button press exits incognito — debounced
      if ((u_btn.justPressed() || d_btn.justPressed()) &&
          (currentTime - this->last_mode_change_ms >= 300)) {
        this->setDisplayMode(STATS_NEW);
        display.tft->fillScreen(ST77XX_BLACK);
      }
      return;
    }

    // Leaving incognito — restore backlight
    if (this->incognito_counting) {
      this->incognito_counting = false;
      display.ctrlBacklight(true);
    }

    this->doHardRefresh();

    // ---- Screen 1: new large-format stats ----
    if (this->stat_display_mode == STATS_NEW) {
      this->drawStatsNew(
        currentTime,
        wifi_ops.getCurrent2g4Count(),
        wifi_ops.getCurrent5gCount(),
        wifi_ops.getCurrentBLECount(),
        gps.getNumSats(),
        battery.getBatteryLevel(),
        false
      );
    }
    // ---- Screen 2: original stats ----
    else if (this->stat_display_mode == FULL_STATS) {
      this->updateStats(
        currentTime,
        wifi_ops.getCurrentNetCount(),
        wifi_ops.getCurrent2g4Count(),
        wifi_ops.getCurrent5gCount(),
        wifi_ops.getCurrentBLECount(),
        gps.getNumSats(),
        battery.getBatteryLevel()
      );
    }
    // ---- Screen 3: channel popularity ----
    else if (this->stat_display_mode == CHANNEL_POPULARITY) {
      this->drawChannelPopularity(currentTime);
    }

    // ---- Button handling — debounced at 300ms ----
    bool mode_change_ok = (currentTime - this->last_mode_change_ms >= 300);

    if (u_btn.justPressed() && mode_change_ok) {
      uint8_t next = (this->stat_display_mode >= MAX_DISPLAY_MODES - 1)
                       ? 0 : this->stat_display_mode + 1;
      this->setDisplayMode(next);
      if (next == SD_FILES)
        this->drawCurrentMenu();
      else if (next == STATS_NEW)
        this->drawStatsNew(currentTime,
          wifi_ops.getCurrent2g4Count(), wifi_ops.getCurrent5gCount(),
          wifi_ops.getCurrentBLECount(), gps.getNumSats(),
          battery.getBatteryLevel(), true);
      else if (next == FULL_STATS)
        this->updateStats(currentTime,
          wifi_ops.getCurrentNetCount(), wifi_ops.getCurrent2g4Count(),
          wifi_ops.getCurrent5gCount(), wifi_ops.getCurrentBLECount(),
          gps.getNumSats(), battery.getBatteryLevel(), true);
      else if (next == CHANNEL_POPULARITY)
        this->drawChannelPopularity(currentTime, true);
    }

    if (d_btn.justPressed() && mode_change_ok) {
      uint8_t next = (this->stat_display_mode == 0)
                       ? MAX_DISPLAY_MODES - 1 : this->stat_display_mode - 1;
      this->setDisplayMode(next);
      if (next == SD_FILES)
        this->drawCurrentMenu();
      else if (next == STATS_NEW)
        this->drawStatsNew(currentTime,
          wifi_ops.getCurrent2g4Count(), wifi_ops.getCurrent5gCount(),
          wifi_ops.getCurrentBLECount(), gps.getNumSats(),
          battery.getBatteryLevel(), true);
      else if (next == FULL_STATS)
        this->updateStats(currentTime,
          wifi_ops.getCurrentNetCount(), wifi_ops.getCurrent2g4Count(),
          wifi_ops.getCurrent5gCount(), wifi_ops.getCurrentBLECount(),
          gps.getNumSats(), battery.getBatteryLevel(), true);
      else if (next == CHANNEL_POPULARITY)
        this->drawChannelPopularity(currentTime, true);
    }

    if (c_btn.justPressed())
      Logger::log(STD_MSG, "C_BTN Pressed: " + (String)millis());

  } else if (this->stat_display_mode == SD_FILES) {
    this->handleMenuNavigation();
  }
}
