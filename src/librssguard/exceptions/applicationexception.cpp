// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/applicationexception.h"

#include "definitions/definitions.h"

#include <QFileInfo>

ApplicationException::ApplicationException(const QString& message, const QString& file, int line)
  : m_message(processException(message, file, line)) {}

ApplicationException::~ApplicationException() {}

QString ApplicationException::message() const {
  return m_message;
}

void ApplicationException::setMessage(const QString& message) {
  m_message = message;
}

QString ApplicationException::processException(const QString& message, const QString& file, int line) {
  if (line > 0) {
    return QSL("%1 [%2:%3]").arg(message, QFileInfo(file).fileName(), QString::number(line));
  }
  else {
    return message;
  }
}
