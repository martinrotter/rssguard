// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef APPLICATIONEXCEPTION_H
#define APPLICATIONEXCEPTION_H

#include <QString>

class ApplicationException {
  public:
    explicit ApplicationException(QString message = QString());

    QString message() const;

  private:
    QString m_message;
};

#endif // APPLICATIONEXCEPTION_H
