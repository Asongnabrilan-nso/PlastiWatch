#include "managers/OTAManager.h"

OTAManager::OTAManager() : server(OTA_PORT) {}

void OTAManager::init() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Non-blocking wait for connection (optional, or just let it connect in
  // background) For a wearable, maybe we only connect when needed or requested?
  // For now, we'll just try to connect at boot.

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Plastiwatch OTA Server");
  });

  ElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();
  Serial.println("OTA Server Started");
}

void OTAManager::handle() { ElegantOTA.loop(); }
