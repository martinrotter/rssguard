// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/filteringexception.h"

FilteringException::FilteringException(QJSValue::ErrorType js_error, QString message)
  : ApplicationException(message), m_errorType(js_error) {}

QJSValue::ErrorType FilteringException::errorType() const {
  return m_errorType;
}
