// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SCRIPTEXCEPTION_H
#define SCRIPTEXCEPTION_H

#include "exceptions/applicationexception.h"

#include <QCoreApplication>

class ScriptException : public ApplicationException {
  Q_DECLARE_TR_FUNCTIONS(ScriptException)

  public:
    enum class Reason {
      ExecutionLineInvalid,
      InterpreterNotFound,
      InterpreterError,
      InterpreterTimeout,
      OtherError
    };

    explicit ScriptException(Reason reason = Reason::OtherError, QString message = QString());

    Reason reason() const;

  private:
    QString messageForReason(Reason reason) const;

  private:
    Reason m_reason;
};

#endif // SCRIPTEXCEPTION_H
