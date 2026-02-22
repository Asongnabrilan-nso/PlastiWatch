// =============================================================================
// NetworkManager.h — PlastiWatch: WiFi Connection Management
// =============================================================================
#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// NetworkManager — singleton-style WiFi helper
// -----------------------------------------------------------------------------
class NetworkManager {
public:
    NetworkManager() = delete;

    /// Connect to the configured WiFi network.
    /// Blocks until connected or timeout is reached.
    /// @return true on success; false on timeout.
    static bool connect();

    /// Re-establish connection if it has been dropped.
    /// Non-blocking: returns immediately if still connected.
    /// @return true if connected after the call.
    static bool ensureConnected();

    /// Disconnect and power down the radio.
    static void disconnect();

    /// @return true when an IP address has been assigned.
    static bool isConnected();

    /// @return the device's assigned IPv4 address as a string.
    static String localIP();

    /// @return RSSI of the current AP in dBm (0 if not connected).
    static int32_t rssi();

    /// Print connection details to the logger.
    static void printStatus();

private:
    static unsigned long s_lastReconnectMs;
};
