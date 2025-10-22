// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SQLEXCEPTION_H
#define SQLEXCEPTION_H

#include "exceptions/applicationexception.h"

#include <QSqlError>

class RSSGUARD_DLLSPEC SqlException : public ApplicationException {
  public:
    explicit SqlException(const QSqlError& error);

  private:
    QString messageForError(const QSqlError& error) const;
};

#endif // SQLEXCEPTION_H
