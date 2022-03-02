// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef PROCESSEXCEPTION_H
#define PROCESSEXCEPTION_H

#include "exceptions/applicationexception.h"

#include <QProcess>

class ProcessException : public ApplicationException {
  public:
    ProcessException(int exit_code, QProcess::ExitStatus exit_status,
                     QProcess::ProcessError error, const QString& message = QString());

    QProcess::ExitStatus exitStatus() const;
    int exitCode() const;
    QProcess::ProcessError error() const;

  private:
    QProcess::ProcessError m_error;
    QProcess::ExitStatus m_exitStatus;
    int m_exitCode;
};

#endif // PROCESSEXCEPTION_H
