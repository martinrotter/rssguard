// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef OAUTHLOGIN_H
#define OAUTHLOGIN_H

#include <QDialog>

#include "ui_oauthlogin.h"

namespace Ui {
  class OAuthLogin;
}

class OAuthLogin : public QDialog {
  Q_OBJECT

  public:
    explicit OAuthLogin(QWidget* parent = 0);

    void login(const QString& consentPageUrl, const QString& redirect_uri);

  private slots:
    void urlChanged(QUrl url);

  signals:
    void authRejected();
    void authGranted(QString authCode);

  private:
    Ui::OAuthLogin m_ui;
    QString m_redirectUri;
};

#endif // OAUTHLOGIN_H
