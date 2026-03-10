// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef APPLICATIONEXCEPTION_H
#define APPLICATIONEXCEPTION_H

#include <QException>
#include <QString>

#define THROW_EX(type, msg) throw type((msg), __FILE__, __LINE__)

class RSSGUARD_DLLSPEC ApplicationException : public QException {
  public:
    explicit ApplicationException(const QString& message = {}, const QString& file = {}, int line = 0);
    virtual ~ApplicationException();

    QString message() const;

    virtual void raise() const;
    virtual ApplicationException* clone() const;

  protected:
    void setMessage(const QString& message);

  private:
    QString processException(const QString& message, const QString& file, int line);

  private:
    QString m_message;
};

#endif // APPLICATIONEXCEPTION_H
