#pragma once

#include "config.h"
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <WiFi.h>

class OTAManager {
public:
  OTAManager();
  void init();
  void handle(); // Call in main loop or task if needed (ElegantOTA 3 is async
                 // mostly)

private:
  AsyncWebServer server;
};
