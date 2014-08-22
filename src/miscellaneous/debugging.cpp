// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "miscellaneous/debugging.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"

#include <QDir>

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
  const char *file = qPrintable(QString(placement.file).section(QDir::separator(),
                                                                -1));

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
  Q_UNUSED(type)
  Q_UNUSED(placement)
  Q_UNUSED(message)
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
  Q_UNUSED(type)
  Q_UNUSED(message)
#endif
}
#endif

Debugging::Debugging() {
}
