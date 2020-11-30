// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef AUTHENTICATIONDETAILS_H
#define AUTHENTICATIONDETAILS_H

#include <QWidget>

#include "ui_authenticationdetails.h"

class AuthenticationDetails : public QWidget {
  Q_OBJECT

  friend class FormStandardFeedDetails;

  public:
    explicit AuthenticationDetails(QWidget* parent = nullptr);

  private slots:
    void onUsernameChanged(const QString& new_username);
    void onPasswordChanged(const QString& new_password);
    void onAuthenticationSwitched();

  private:
    Ui::AuthenticationDetails m_ui;
};

#endif // AUTHENTICATIONDETAILS_H
