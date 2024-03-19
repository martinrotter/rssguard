// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/processexception.h"

ProcessException::ProcessException(int exit_code,
                                   QProcess::ExitStatus exit_status,
                                   QProcess::ProcessError error,
                                   const QString& message)
  : ApplicationException(message), m_error(error), m_exitStatus(exit_status), m_exitCode(exit_code) {}

QProcess::ExitStatus ProcessException::exitStatus() const {
  return m_exitStatus;
}

int ProcessException::exitCode() const {
  return m_exitCode;
}

QProcess::ProcessError ProcessException::error() const {
  return m_error;
}
