// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FILTERINGEXCEPTION_H
#define FILTERINGEXCEPTION_H

#include "exceptions/applicationexception.h"

#include <QJSValue>

class RSSGUARD_DLLSPEC FilteringException : public ApplicationException {
  public:
    explicit FilteringException(const QJSValue& js_error, const QString& message = QString());

    QJSValue::ErrorType errorType() const;

  private:
    QString decodeError(const QJSValue& js_error, const QString &message);

    QJSValue::ErrorType m_errorType;
};

#endif // FILTERINGEXCEPTION_H
