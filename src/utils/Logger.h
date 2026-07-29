#pragma once

#include <cstdarg>

enum class LogLevel { kError = 0, kWarn = 1, kInfo = 2, kDebug = 3 };

// Journalisation minimale sur port serie. Ne jamais logger de mot de passe,
// jeton ou secret ici (voir docs/API.md et regle de securite du projet).
class Logger {
 public:
  static void setLevel(LogLevel level);
  static void error(const char *format, ...);
  static void warn(const char *format, ...);
  static void info(const char *format, ...);
  static void debug(const char *format, ...);

 private:
  static void log(LogLevel level, const char *tag, const char *format, va_list args);
  static LogLevel currentLevel_;
};
