// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef INOREADERACCOUNTDETAILS_H
#define INOREADERACCOUNTDETAILS_H

#include <QWidget>

#include "ui_inoreaderaccountdetails.h"

class OAuth2Service;

class InoreaderAccountDetails : public QWidget {
  Q_OBJECT

  friend class FormEditInoreaderAccount;

  public:
    explicit InoreaderAccountDetails(QWidget* parent = nullptr);

  private slots:
    void registerApi();
    void testSetup();;
    void checkOAuthValue(const QString& value);
    void checkUsername(const QString& username);
    void onAuthFailed();
    void onAuthError(const QString& error, const QString& detailed_description);
    void onAuthGranted();

  private:
    void hookNetwork();

  private:
    Ui::InoreaderAccountDetails m_ui;

    // Testing OAuth service. Only for testing. Resulting tokens
    //
    OAuth2Service* m_oauth;
};

#endif // INOREADERACCOUNTDETAILS_H
