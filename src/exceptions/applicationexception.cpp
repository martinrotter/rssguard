// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/applicationexception.h"

ApplicationException::ApplicationException(const QString& message) : m_message(message) {}

ApplicationException::~ApplicationException() {}

QString ApplicationException::message() const {
  return m_message;
}
