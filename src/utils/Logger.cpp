#include "utils/Logger.h"

#include <cstdarg>
#include <cstdio>

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <chrono>
#endif

namespace {
// Horodatage portable (ms depuis le demarrage) : millis() sur firmware,
// horloge monotone sur simulateur/tests desktop.
unsigned long portableMillis() {
#ifdef ARDUINO
  return millis();
#else
  using namespace std::chrono;
  static const auto start = steady_clock::now();
  return static_cast<unsigned long>(duration_cast<milliseconds>(steady_clock::now() - start).count());
#endif
}

void writeLine(const char *tag, const char *message, unsigned long ms) {
#ifdef ARDUINO
  Serial.printf("[%8lu][%s] %s\n", ms, tag, message);
#else
  std::fprintf(stderr, "[%8lu][%s] %s\n", ms, tag, message);
#endif
}
}  // namespace

LogLevel Logger::currentLevel_ = LogLevel::kInfo;

void Logger::setLevel(LogLevel level) { currentLevel_ = level; }

void Logger::log(LogLevel level, const char *tag, const char *format, va_list args) {
  if (level > currentLevel_) return;
  char message[256];
  vsnprintf(message, sizeof(message), format, args);
  writeLine(tag, message, portableMillis());
}

void Logger::error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  log(LogLevel::kError, "ERROR", format, args);
  va_end(args);
}

void Logger::warn(const char *format, ...) {
  va_list args;
  va_start(args, format);
  log(LogLevel::kWarn, "WARN ", format, args);
  va_end(args);
}

void Logger::info(const char *format, ...) {
  va_list args;
  va_start(args, format);
  log(LogLevel::kInfo, "INFO ", format, args);
  va_end(args);
}

void Logger::debug(const char *format, ...) {
  va_list args;
  va_start(args, format);
  log(LogLevel::kDebug, "DEBUG", format, args);
  va_end(args);
}
