// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/scriptexception.h"

#include "definitions/definitions.h"

ScriptException::ScriptException(Reason reason, const QString& message)
  : ApplicationException(message), m_reason(reason) {
  if (message.isEmpty()) {
    setMessage(messageForReason(reason));
  }
  else if (reason == ScriptException::Reason::InterpreterError || reason == ScriptException::Reason::OtherError) {
    setMessage(messageForReason(reason) + QSL(": '%1'").arg(message));
  }
}

ScriptException::Reason ScriptException::reason() const {
  return m_reason;
}

QString ScriptException::messageForReason(ScriptException::Reason reason) const {
  switch (reason) {
    case ScriptException::Reason::ExecutionLineInvalid:
      return tr("script line is not well-formed");

    case ScriptException::Reason::InterpreterError:
      return tr("script threw an error");

    case ScriptException::Reason::InterpreterNotFound:
      return tr("script's interpreter was not found");

    case ScriptException::Reason::InterpreterTimeout:
      return tr("script execution took too long");

    case ScriptException::Reason::OtherError:
    default:
      return tr("unknown error");
  }
}
