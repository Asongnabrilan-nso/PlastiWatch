// =============================================================================
// EdgeImpulseClient.cpp — PlastiWatch: Edge Impulse Ingestion API Client
// =============================================================================

#include "EdgeImpulseClient.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "../config/Config.h"
#include "../core/Logger.h"
#include "../network/NetworkManager.h"

static const char* TAG = "EIClient";

// =============================================================================
// Internal helper — PayloadBuilder
// Wraps a raw char buffer with safe append operations and overflow tracking.
// =============================================================================
namespace {

class PayloadBuilder {
public:
    PayloadBuilder(char* buf, size_t capacity)
        : m_buf(buf), m_capacity(capacity), m_written(0), m_overflow(false)
    {
        m_buf[0] = '\0';
    }

    void append(const char* str) {
        if (m_overflow || !str) return;
        size_t len = strlen(str);
        if (m_written + len + 1 > m_capacity) { m_overflow = true; return; }
        memcpy(m_buf + m_written, str, len);
        m_written += len;
        m_buf[m_written] = '\0';
    }

    void appendf(const char* fmt, ...) {
        if (m_overflow) return;
        va_list args;
        va_start(args, fmt);
        int n = vsnprintf(m_buf + m_written,
                          m_capacity - m_written, fmt, args);
        va_end(args);
        if (n < 0 || static_cast<size_t>(n) >= m_capacity - m_written) {
            m_overflow = true;
            return;
        }
        m_written += static_cast<size_t>(n);
    }

    size_t written()    const { return m_written; }
    bool   isOverflow() const { return m_overflow; }

private:
    char*  m_buf;
    size_t m_capacity;
    size_t m_written;
    bool   m_overflow;
};

} // anonymous namespace

// =============================================================================
// buildPayload()
// =============================================================================

size_t EdgeImpulseClient::buildPayload(const SampleBuffer& buffer,
                                       char* outBuf, size_t bufSize) {
    PayloadBuilder pb(outBuf, bufSize);

    // -- Protected header (unsigned, no signature required for ingestion API) --
    pb.append(
        "{"
          "\"protected\":{"
            "\"ver\":\"v1\","
            "\"alg\":\"none\","
            "\"iat\":0"
          "},"
          "\"signature\":"
            "\"0000000000000000000000000000000000000000000000000000000000000000\","
          "\"payload\":{"
    );

    // -- Device metadata -------------------------------------------------------
    pb.appendf(
        "\"device_name\":\"%s\","
        "\"device_type\":\"%s\","
        "\"interval_ms\":%.4f,",
        EI_DEVICE_NAME,
        EI_DEVICE_TYPE,
        buffer.intervalMs()
    );

    // -- Sensor descriptors ---------------------------------------------------
    pb.append(
        "\"sensors\":["
          "{\"name\":\"accX\",\"units\":\"m/s2\"},"
          "{\"name\":\"accY\",\"units\":\"m/s2\"},"
          "{\"name\":\"accZ\",\"units\":\"m/s2\"},"
          "{\"name\":\"gyrX\",\"units\":\"dps\"},"
          "{\"name\":\"gyrY\",\"units\":\"dps\"},"
          "{\"name\":\"gyrZ\",\"units\":\"dps\"}"
        "],"
        "\"values\":["
    );

    // -- Sample values --------------------------------------------------------
    for (size_t i = 0; i < buffer.count(); i++) {
        if (i > 0) pb.append(",");
        const IMUSample& s = buffer[i];
        pb.appendf("[%.6f,%.6f,%.6f,%.6f,%.6f,%.6f]",
            s.accX, s.accY, s.accZ,
            s.gyrX, s.gyrY, s.gyrZ);
    }

    // -- Closing --------------------------------------------------------------
    pb.append("]}}");

    if (pb.isOverflow()) {
        Logger::error(TAG,
            "Payload buffer overflow! "
            "Increase JSON_PAYLOAD_MAX_BYTES in Config.h");
        return 0;
    }

    Logger::debugf(TAG, "Payload built: %u bytes", pb.written());
    return pb.written();
}

// =============================================================================
// upload()
// =============================================================================

UploadResult EdgeImpulseClient::upload(const SampleBuffer& buffer,
                                       const char* label) {
    // -- Pre-flight checks ----------------------------------------------------
    if (!NetworkManager::isConnected()) {
        Logger::error(TAG, "Upload aborted — no WiFi connection");
        return UploadResult::ERR_NO_WIFI;
    }
    if (buffer.isEmpty()) {
        Logger::error(TAG, "Upload aborted — sample buffer is empty");
        return UploadResult::ERR_NO_SAMPLES;
    }

    // -- Allocate payload buffer on heap --------------------------------------
    const size_t bufSize = JSON_PAYLOAD_MAX_BYTES;
    char* payload = new (std::nothrow) char[bufSize];
    if (!payload) {
        Logger::errorf(TAG,
            "Failed to allocate %u bytes for payload", bufSize);
        return UploadResult::ERR_ALLOC;
    }

    size_t payloadLen = buildPayload(buffer, payload, bufSize);
    if (payloadLen == 0) {
        delete[] payload;
        return UploadResult::ERR_OVERFLOW;
    }

    // -- Build URL ------------------------------------------------------------
    String url = String("https://") + EI_INGESTION_HOST + EI_INGESTION_PATH
               + "?label=" + label;

    String fileName = String(EI_DEVICE_NAME) + "." + label + ".json";

    Logger::infof(TAG, "Uploading %u samples (label: \"%s\", payload: %u B)...",
        buffer.count(), label, payloadLen);

    // -- HTTP POST ------------------------------------------------------------
    WiFiClientSecure secureClient;
    // NOTE: setInsecure() skips certificate validation.
    // For production, replace with secureClient.setCACert(rootCA) using the
    // ISRG Root X1 certificate (Edge Impulse ingestion endpoint uses Let's Encrypt).
    secureClient.setInsecure();

    HTTPClient http;
    http.setTimeout(EI_HTTP_TIMEOUT_MS);

    if (!http.begin(secureClient, url)) {
        Logger::error(TAG, "HTTPClient::begin() failed");
        delete[] payload;
        return UploadResult::ERR_TIMEOUT;
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-api-key",    EI_API_KEY);
    http.addHeader("x-file-name",  fileName);
    http.addHeader("x-label",      label);

    int httpCode = http.POST(reinterpret_cast<uint8_t*>(payload), payloadLen);
    delete[] payload;  // Free as soon as POST is done

    // -- Handle response ------------------------------------------------------
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
        Logger::infof(TAG, "Upload successful (HTTP %d)", httpCode);
        http.end();
        return UploadResult::OK;
    }

    if (httpCode < 0) {
        Logger::errorf(TAG, "HTTP error: %s", http.errorToString(httpCode).c_str());
        http.end();
        return UploadResult::ERR_TIMEOUT;
    }

    Logger::errorf(TAG, "Server rejected upload (HTTP %d): %s",
        httpCode, http.getString().c_str());
    http.end();
    return UploadResult::ERR_HTTP;
}

// =============================================================================
// resultStr()
// =============================================================================

const char* EdgeImpulseClient::resultStr(UploadResult result) {
    switch (result) {
        case UploadResult::OK:             return "OK";
        case UploadResult::ERR_NO_WIFI:    return "No WiFi";
        case UploadResult::ERR_NO_SAMPLES: return "Empty buffer";
        case UploadResult::ERR_ALLOC:      return "Memory allocation failed";
        case UploadResult::ERR_OVERFLOW:   return "JSON payload overflow";
        case UploadResult::ERR_HTTP:       return "HTTP error";
        case UploadResult::ERR_TIMEOUT:    return "Connection timeout";
        default:                           return "Unknown error";
    }
}
