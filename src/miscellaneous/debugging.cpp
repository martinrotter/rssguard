// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/debugging.h"

#include "miscellaneous/application.h"

#include <QDir>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>

Q_GLOBAL_STATIC(Debugging, qz_debug_acmanager)

Debugging * Debugging::instance() {
  return qz_debug_acmanager();
}

void Debugging::setTargetFile(const QString& targetFile) {
  m_targetFile = targetFile;

  if (!m_targetFile.isEmpty()) {
    m_targetFileHandle = new QFile(m_targetFile);
    m_targetFileHandle->open(QIODevice::WriteOnly | QIODevice::Append);
  }
}

QString Debugging::targetFile() const {
  return m_targetFile;
}

QFile* Debugging::targetFileHandle() {
  return m_targetFileHandle;
}

void Debugging::performLog(const char* message, QtMsgType type, const char* file, const char* function, int line) {
  const char* type_string = typeToString(type);
  QString date_str = QDateTime::currentDateTimeUtc().toString(QSL("yyyy-MM-dd HH:mm:ss.zzz UTC"));

  if (instance()->targetFile().isEmpty()) {

    // Write to console.
    if (file == nullptr || function == nullptr || line < 0) {
      fprintf(stderr, "[%s] %s: %s (%s)\n", APP_LOW_NAME, type_string, message, qPrintable(date_str));
    }
    else {
      fprintf(stderr, "[%s] %s (%s)\n  Type: %s\n  File: %s (line %d)\n  Function: %s\n\n",
              APP_LOW_NAME, message, qPrintable(date_str), type_string, file, line, function);
    }
  }
  else {
    if (file == nullptr || function == nullptr || line < 0) {
      instance()->targetFileHandle()->write(QString("[%1] %2: %3 (%4)\n").arg(APP_LOW_NAME, type_string,
                                                                              message, qPrintable(date_str)).toUtf8());
    }
    else {
      instance()->targetFileHandle()->write(QString("[%1] %2 (%3)\n  Type: %4\n  File: %5 (line %6)\n  Function: %7\n\n")
                                            .arg(APP_LOW_NAME, message, qPrintable(date_str), type_string,
                                                 file, QString::number(line), function).toUtf8());
    }

    instance()->targetFileHandle()->flush();
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

Debugging::Debugging() = default;
void Debugging::debugHandler(QtMsgType type, const QMessageLogContext& placement, const QString& message) {
#ifndef QT_NO_DEBUG_OUTPUT
  performLog(qPrintable(message), type, placement.file, placement.function, placement.line);
#else
  Q_UNUSED(type)
  Q_UNUSED(placement)
  Q_UNUSED(message)
#endif
}
