// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDLYACCOUNTDETAILS_H
#define FEEDLYACCOUNTDETAILS_H

#include "services/feedly/feedlyserviceroot.h"

#include "ui_feedlyaccountdetails.h"

#include <QNetworkProxy>
#include <QWidget>

#if defined(FEEDLY_OFFICIAL_SUPPORT)
class OAuth2Service;
#endif

class FeedlyAccountDetails : public QWidget {
    Q_OBJECT

    friend class FormEditFeedlyAccount;

  public:
    explicit FeedlyAccountDetails(QWidget* parent = nullptr);

  private slots:
    void getDeveloperAccessToken();
    void performTest(const QNetworkProxy& custom_proxy);
    void onUsernameChanged();
    void onDeveloperAccessTokenChanged();

#if defined(FEEDLY_OFFICIAL_SUPPORT)
  private slots:
    void onAuthFailed();
    void onAuthError(const QString& error, const QString& detailed_description);
    void onAuthGranted();

  private:
    void hookNetwork();
#endif

  private:
    Ui::FeedlyAccountDetails m_ui;

#if defined(FEEDLY_OFFICIAL_SUPPORT)
    OAuth2Service* m_oauth;
#endif

    QNetworkProxy m_lastProxy;
};

#endif // FEEDLYACCOUNTDETAILS_H
