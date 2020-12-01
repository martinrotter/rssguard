// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef AUTHENTICATIONDETAILS_H
#define AUTHENTICATIONDETAILS_H

#include <QWidget>

#include "ui_authenticationdetails.h"

class AuthenticationDetails : public QWidget, public Ui::AuthenticationDetails {
  Q_OBJECT

  public:
    explicit AuthenticationDetails(QWidget* parent = nullptr);

  private slots:
    void onUsernameChanged(const QString& new_username);
    void onPasswordChanged(const QString& new_password);
    void onAuthenticationSwitched();
};

#endif // AUTHENTICATIONDETAILS_H
