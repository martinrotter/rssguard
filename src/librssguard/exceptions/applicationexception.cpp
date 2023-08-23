// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/applicationexception.h"

ApplicationException::ApplicationException(QString message) : m_message(std::move(message)) {}

ApplicationException::~ApplicationException() {}

QString ApplicationException::message() const {
  return m_message;
}

void ApplicationException::setMessage(const QString& message) {
  m_message = message;
}
