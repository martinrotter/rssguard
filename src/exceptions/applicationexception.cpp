// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/applicationexception.h"

ApplicationException::ApplicationException(QString message) : m_message(std::move(message)) {}

QString ApplicationException::message() const {
  return m_message;
}
