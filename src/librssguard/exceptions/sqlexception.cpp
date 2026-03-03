// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/sqlexception.h"

#include "definitions/definitions.h"

SqlException::SqlException(Type type, const QString& message) : ApplicationException(message), m_type(type) {}

SqlException::SqlException(const QSqlError& error, const QString& file, int line)
  : ApplicationException(messageForError(error), file, line), m_type(Type::GeneralError) {}

QString SqlException::messageForError(const QSqlError& error) const {
  if (!error.isValid()) {
    return {};
  }

  auto error_code = error.nativeErrorCode();

  if (error_code.isEmpty()) {
    return error.text();
  }
  else {
    return QSL("%1/%2").arg(error_code, error.text());
  }
}

SqlException::Type SqlException::type() const {
  return m_type;
}

void SqlException::setType(Type type) {
  m_type = type;
}
