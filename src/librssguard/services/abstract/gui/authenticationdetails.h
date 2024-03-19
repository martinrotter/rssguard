// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef AUTHENTICATIONDETAILS_H
#define AUTHENTICATIONDETAILS_H

#include "network-web/networkfactory.h"
#include "services/abstract/feed.h"

#include <QWidget>

namespace Ui {
  class AuthenticationDetails;
}

class RSSGUARD_DLLSPEC AuthenticationDetails : public QWidget {
    Q_OBJECT

  public:
    explicit AuthenticationDetails(bool only_basic, QWidget* parent = nullptr);
    virtual ~AuthenticationDetails();

    QString username() const;
    void setUsername(const QString& username);

    QString password() const;
    void setPassword(const QString& password);

    void setAuthenticationType(NetworkFactory::NetworkFactory::NetworkAuthentication protect);
    NetworkFactory::NetworkFactory::NetworkAuthentication authenticationType() const;

  private slots:
    void onUsernameChanged(const QString& new_username);
    void onPasswordChanged(const QString& new_password);
    void onAuthenticationSwitched();

  private:
    QScopedPointer<Ui::AuthenticationDetails> m_ui;
};

#endif // AUTHENTICATIONDETAILS_H
