// =============================================================================
// Logger.h — PlastiWatch: Lightweight Serial Logger (header-only)
//
// Provides leveled, tagged log output over Serial.
// Compile with -DPLASTIWATCH_DEBUG=1 to enable DEBUG level messages.
// =============================================================================
#pragma once

#include <Arduino.h>
#include <stdarg.h>

// -----------------------------------------------------------------------------
// Log Level
// -----------------------------------------------------------------------------
enum class LogLevel : uint8_t {
    DEBUG   = 0,
    INFO    = 1,
    WARNING = 2,
    ERROR   = 3,
    NONE    = 4     ///< Suppress all output
};

// -----------------------------------------------------------------------------
// Logger — static-only class (no instantiation needed)
// -----------------------------------------------------------------------------
class Logger {
public:
    Logger() = delete;

    // -- Configuration --------------------------------------------------------

    static void setLevel(LogLevel level) { s_level = level; }
    static LogLevel getLevel()           { return s_level; }

    // -- Convenience wrappers -------------------------------------------------

    static void debug(const char* tag, const char* msg) {
        log(LogLevel::DEBUG, tag, msg);
    }

    static void info(const char* tag, const char* msg) {
        log(LogLevel::INFO, tag, msg);
    }

    static void warn(const char* tag, const char* msg) {
        log(LogLevel::WARNING, tag, msg);
    }

    static void error(const char* tag, const char* msg) {
        log(LogLevel::ERROR, tag, msg);
    }

    // -- Printf-style wrappers ------------------------------------------------

    static void debugf(const char* tag, const char* fmt, ...) {
        if (s_level > LogLevel::DEBUG) return;
        va_list args; va_start(args, fmt);
        logv(LogLevel::DEBUG, tag, fmt, args);
        va_end(args);
    }

    static void infof(const char* tag, const char* fmt, ...) {
        if (s_level > LogLevel::INFO) return;
        va_list args; va_start(args, fmt);
        logv(LogLevel::INFO, tag, fmt, args);
        va_end(args);
    }

    static void warnf(const char* tag, const char* fmt, ...) {
        if (s_level > LogLevel::WARNING) return;
        va_list args; va_start(args, fmt);
        logv(LogLevel::WARNING, tag, fmt, args);
        va_end(args);
    }

    static void errorf(const char* tag, const char* fmt, ...) {
        if (s_level > LogLevel::ERROR) return;
        va_list args; va_start(args, fmt);
        logv(LogLevel::ERROR, tag, fmt, args);
        va_end(args);
    }

    // -- Separator helpers ----------------------------------------------------

    static void separator(char ch = '-', uint8_t width = 60) {
        if (s_level > LogLevel::INFO) return;
        for (uint8_t i = 0; i < width; i++) Serial.print(ch);
        Serial.println();
    }

    static void banner(const char* text) {
        if (s_level > LogLevel::INFO) return;
        separator('=');
        Serial.printf("  %s\n", text);
        separator('=');
    }

private:
    static LogLevel s_level;

    static const char* levelStr(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DBG";
            case LogLevel::INFO:    return "INF";
            case LogLevel::WARNING: return "WRN";
            case LogLevel::ERROR:   return "ERR";
            default:                return "???";
        }
    }

    static void log(LogLevel level, const char* tag, const char* msg) {
        if (level < s_level) return;
        Serial.printf("[%s][%-12s] %s\n", levelStr(level), tag, msg);
    }

    static void logv(LogLevel level, const char* tag, const char* fmt, va_list args) {
        if (level < s_level) return;
        char buf[256];
        vsnprintf(buf, sizeof(buf), fmt, args);
        log(level, tag, buf);
    }
};

// Default log level: DEBUG if debug build, else INFO
#ifdef PLASTIWATCH_DEBUG
inline LogLevel Logger::s_level = LogLevel::DEBUG;
#else
inline LogLevel Logger::s_level = LogLevel::INFO;
#endif
