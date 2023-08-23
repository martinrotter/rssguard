// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef REDDITACCOUNTDETAILS_H
#define REDDITACCOUNTDETAILS_H

#include <QWidget>

#include "ui_redditaccountdetails.h"

#include <QNetworkProxy>

class OAuth2Service;

class RedditAccountDetails : public QWidget {
  Q_OBJECT

  friend class FormEditRedditAccount;

  public:
    explicit RedditAccountDetails(QWidget* parent = nullptr);

  public slots:
    void testSetup(const QNetworkProxy& custom_proxy);

  private slots:
    void registerApi();
    void checkOAuthValue(const QString& value);
    void checkUsername(const QString& username);
    void onAuthFailed();
    void onAuthError(const QString& error, const QString& detailed_description);
    void onAuthGranted();

  private:
    void hookNetwork();

  private:
    Ui::RedditAccountDetails m_ui;

    // Pointer to live OAuth.
    OAuth2Service* m_oauth;
    QNetworkProxy m_lastProxy;
};

#endif // REDDITACCOUNTDETAILS_H
