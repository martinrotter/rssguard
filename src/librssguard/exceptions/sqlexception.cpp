// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/sqlexception.h"

#include "definitions/definitions.h"

SqlException::SqlException(const QSqlError& error) : ApplicationException(messageForError(error)) {}

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
