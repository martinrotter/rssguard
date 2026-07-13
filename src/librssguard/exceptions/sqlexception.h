// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SQLEXCEPTION_H
#define SQLEXCEPTION_H

#include "exceptions/applicationexception.h"

#include <QSqlError>

class RSSGUARD_DLLSPEC SqlException : public ApplicationException {
  public:
    enum class Type {
      GeneralError,
      TooOldIncompatibleDbSchema,
      TooNewIncompatibleDbSchema
    };

    explicit SqlException(Type type, const QString& message = {});
    explicit SqlException(const QSqlError& error, const QString& file = QString(), int line = 0);

    void raise() const override { throw *this; }
    SqlException* clone() const override { return new SqlException(*this); }

    Type type() const;
    void setType(Type type);

  private:
    QString messageForError(const QSqlError& error) const;

    Type m_type;
};

#endif // SQLEXCEPTION_H
