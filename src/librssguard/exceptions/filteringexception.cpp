// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/filteringexception.h"

#include "definitions/definitions.h"

#include <QObject>
#include <QString>

FilteringException::FilteringException(const QJSValue& js_error, const QString& message)
  : ApplicationException(decodeError(js_error, message)) {}

QString FilteringException::decodeError(const QJSValue& js_error, const QString& message) {
  m_errorType = js_error.errorType();

  if (message.isEmpty()) {
    return QObject::tr("%1\n"
                       "line: %2\n"
                       "stack: %3")
      .arg(js_error.toString(),
           js_error.property(QSL("lineNumber")).toString(),
           js_error.property(QSL("stack")).toString());
  }
  else {
    return message;
  }
}
