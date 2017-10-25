// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/debugging.h"

#include "miscellaneous/application.h"

#include <QDir>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>

Debugging::Debugging() {}

void Debugging::performLog(const char* message, QtMsgType type, const char* file, const char* function, int line) {
  const char* type_string = typeToString(type);

  std::time_t t = std::time(nullptr);
  char mbstr[32];

  std::strftime(mbstr, sizeof(mbstr), "%y/%d/%m %H:%M:%S", std::localtime(&t));

  // Write to console.
  if (file == 0 || function == 0 || line < 0) {
    fprintf(stderr, "[%s] %s: %s (%s)\n", APP_LOW_NAME, type_string, message, mbstr);
  }
  else {
    fprintf(stderr, "[%s] %s (%s)\n  Type: %s\n  File: %s (line %d)\n  Function: %s\n\n",
            APP_LOW_NAME, message, mbstr, type_string, file, line, function);
  }

  if (type == QtFatalMsg) {
    qApp->exit(EXIT_FAILURE);
  }
}

const char* Debugging::typeToString(QtMsgType type) {
  switch (type) {
    case QtDebugMsg:
      return "DEBUG";

    case QtWarningMsg:
      return "WARNING";

    case QtCriticalMsg:
      return "CRITICAL";

    case QtFatalMsg:
    default:
      return "FATAL (terminating application)";
  }
}

void Debugging::debugHandler(QtMsgType type, const QMessageLogContext& placement, const QString& message) {
#ifndef QT_NO_DEBUG_OUTPUT
  performLog(qPrintable(message), type, placement.file, placement.function, placement.line);
#else
  Q_UNUSED(type)
  Q_UNUSED(placement)
  Q_UNUSED(message)
#endif
}
