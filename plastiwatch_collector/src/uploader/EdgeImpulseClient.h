// =============================================================================
// EdgeImpulseClient.h — PlastiWatch: Edge Impulse Data Ingestion API Client
//
// Uploads a labeled SampleBuffer to Edge Impulse's training dataset via the
// Data Acquisition Format (CBOR-compatible JSON) ingestion REST endpoint.
//
// API reference:
//   https://docs.edgeimpulse.com/reference/data-ingestion/ingestion-api
// =============================================================================
#pragma once

#include <Arduino.h>
#include "../core/SampleBuffer.h"

// -----------------------------------------------------------------------------
// Upload result codes
// -----------------------------------------------------------------------------
enum class UploadResult : uint8_t {
    OK              = 0,
    ERR_NO_WIFI     = 1,    ///< Not connected to WiFi
    ERR_NO_SAMPLES  = 2,    ///< Buffer is empty
    ERR_ALLOC       = 3,    ///< Failed to allocate payload buffer
    ERR_OVERFLOW    = 4,    ///< Payload exceeded buffer — increase JSON_PAYLOAD_MAX_BYTES
    ERR_HTTP        = 5,    ///< Non-2xx HTTP response
    ERR_TIMEOUT     = 6,    ///< Connection / request timeout
};

// -----------------------------------------------------------------------------
// EdgeImpulseClient — static-only uploader
// -----------------------------------------------------------------------------
class EdgeImpulseClient {
public:
    EdgeImpulseClient() = delete;

    /// Upload @p buffer to Edge Impulse with the given @p label.
    /// @return UploadResult::OK on success; error code otherwise.
    static UploadResult upload(const SampleBuffer& buffer, const char* label);

    /// Human-readable description of an UploadResult.
    static const char* resultStr(UploadResult result);

private:
    /// Build the Edge Impulse JSON payload into @p outBuf (null-terminated).
    /// @return Number of bytes written, or 0 on overflow.
    static size_t buildPayload(const SampleBuffer& buffer,
                               char* outBuf, size_t bufSize);
};
