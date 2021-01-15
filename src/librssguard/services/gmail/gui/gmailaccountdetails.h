// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GMAILACCOUNTDETAILS_H
#define GMAILACCOUNTDETAILS_H

#include <QWidget>

#include "ui_gmailaccountdetails.h"

class OAuth2Service;

class GmailAccountDetails : public QWidget {
  Q_OBJECT

  friend class FormEditGmailAccount;

  public:
    explicit GmailAccountDetails(QWidget* parent = nullptr);

  private slots:
    void registerApi();
    void testSetup();
    void checkOAuthValue(const QString& value);
    void checkUsername(const QString& username);
    void onAuthFailed();
    void onAuthError(const QString& error, const QString& detailed_description);
    void onAuthGranted();

  private:
    void hookNetwork();

  private:
    Ui::GmailAccountDetails m_ui;
    OAuth2Service* m_oauth;
};

#endif // GMAILACCOUNTDETAILS_H
