// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef APPLICATIONEXCEPTION_H
#define APPLICATIONEXCEPTION_H

#include <QString>

class ApplicationException {
  public:
    explicit ApplicationException(QString message = {});
    virtual ~ApplicationException();

    QString message() const;

  protected:
    void setMessage(const QString& message);

  private:
    QString m_message;
};

#endif // APPLICATIONEXCEPTION_H
