#include <QApplication>
#include <QDir>

#include "core/defs.h"
#include "core/debugging.h"

#include <cstdio>

#ifndef QT_NO_DEBUG_OUTPUT
#if QT_VERSION >= 0x050000
#define DEBUG_OUTPUT_WORKER(type_string, file, line, message) \
  fprintf(stderr, "[%s] %s (%s:%d): %s\n", \
  APP_LOW_NAME, \
  type_string, \
  file, \
  line, \
  qPrintable(message));
#else
#define DEBUG_OUTPUT_WORKER(type_string, message) \
  fprintf(stderr, "[%s] %s: %s\n", \
  APP_LOW_NAME, \
  type_string, \
  message);
#endif
#endif


#if QT_VERSION >= 0x050000
void Debugging::debugHandler(QtMsgType type,
                             const QMessageLogContext &placement,
                             const QString &message) {
#ifndef QT_NO_DEBUG_OUTPUT
  const char *file = qPrintable(QString(placement.file).section(QDir::separator(), -1));
  switch (type) {
    case QtDebugMsg:
      DEBUG_OUTPUT_WORKER("INFO", file, placement.line, message);
      break;
    case QtWarningMsg:
      DEBUG_OUTPUT_WORKER("WARNING", file, placement.line, message);
      break;
    case QtCriticalMsg:
      DEBUG_OUTPUT_WORKER("CRITICAL", file, placement.line, message);
      break;
    case QtFatalMsg:
      DEBUG_OUTPUT_WORKER("FATAL", file, placement.line, message);
      qApp->exit(EXIT_FAILURE);
    default:
      break;
  }
#else
  Q_UNUSED(type);
  Q_UNUSED(placement);
  Q_UNUSED(message);
#endif
}
#else
void Debugging::debugHandler(QtMsgType type, const char *message) {
  #ifndef QT_NO_DEBUG_OUTPUT
  switch (type) {
    case QtDebugMsg:
      DEBUG_OUTPUT_WORKER("INFO", message);
      break;
    case QtWarningMsg:
      DEBUG_OUTPUT_WORKER("WARNING", message);
      break;
    case QtCriticalMsg:
      DEBUG_OUTPUT_WORKER("CRITICAL", message);
      break;
    case QtFatalMsg:
      DEBUG_OUTPUT_WORKER("FATAL", message);
      qApp->exit(EXIT_FAILURE);
    default:
      break;
  }
#else
  Q_UNUSED(type);
  Q_UNUSED(message);
#endif
}
#endif

Debugging::Debugging() {
}
