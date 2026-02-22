// =============================================================================
// NetworkManager.cpp — PlastiWatch: WiFi Connection Management
// =============================================================================

#include "NetworkManager.h"
#include <WiFi.h>
#include "../config/Config.h"
#include "../core/Logger.h"

static const char* TAG = "Network";

// Static member definition
unsigned long NetworkManager::s_lastReconnectMs = 0;

// =============================================================================
// connect()
// =============================================================================

bool NetworkManager::connect() {
    if (isConnected()) {
        Logger::debug(TAG, "Already connected — skipping");
        return true;
    }

    Logger::infof(TAG, "Connecting to \"%s\"...", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    const unsigned long deadline = millis() + WIFI_CONNECT_TIMEOUT_MS;
    uint8_t dotCount = 0;

    while (!isConnected() && millis() < deadline) {
        delay(250);
        if (++dotCount % 20 == 0) Serial.println();
        Serial.print('.');
    }
    Serial.println();

    if (!isConnected()) {
        Logger::errorf(TAG,
            "Connection failed after %u ms. "
            "Check SSID / password in Config.h.",
            WIFI_CONNECT_TIMEOUT_MS);
        return false;
    }

    Logger::infof(TAG, "Connected — IP: %s  RSSI: %d dBm",
        localIP().c_str(), rssi());
    return true;
}

// =============================================================================
// ensureConnected()
// =============================================================================

bool NetworkManager::ensureConnected() {
    if (isConnected()) return true;

    const unsigned long now = millis();
    if (now - s_lastReconnectMs < WIFI_RECONNECT_INTERVAL_MS) return false;

    s_lastReconnectMs = now;
    Logger::warn(TAG, "Connection lost — attempting reconnect");
    return connect();
}

// =============================================================================
// disconnect()
// =============================================================================

void NetworkManager::disconnect() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Logger::info(TAG, "Disconnected");
}

// =============================================================================
// isConnected()
// =============================================================================

bool NetworkManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

// =============================================================================
// localIP()
// =============================================================================

String NetworkManager::localIP() {
    return WiFi.localIP().toString();
}

// =============================================================================
// rssi()
// =============================================================================

int32_t NetworkManager::rssi() {
    return isConnected() ? WiFi.RSSI() : 0;
}

// =============================================================================
// printStatus()
// =============================================================================

void NetworkManager::printStatus() {
    if (isConnected()) {
        Logger::infof(TAG, "SSID: %s  IP: %s  RSSI: %d dBm",
            WiFi.SSID().c_str(), localIP().c_str(), rssi());
    } else {
        Logger::warn(TAG, "Not connected");
    }
}
