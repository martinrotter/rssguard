// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FILTERINGEXCEPTION_H
#define FILTERINGEXCEPTION_H

#include "exceptions/applicationexception.h"

#include <QJSValue>

class FilteringException : public ApplicationException {
  public:
    explicit FilteringException(QJSValue::ErrorType js_error, QString message = QString());

    QJSValue::ErrorType errorType() const;

  private:
    QJSValue::ErrorType m_errorType;
};

#endif // FILTERINGEXCEPTION_H
