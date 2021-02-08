// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDLYACCOUNTDETAILS_H
#define FEEDLYACCOUNTDETAILS_H

#include <QWidget>

#include "ui_feedlyaccountdetails.h"

#include "services/feedly/feedlyserviceroot.h"

#include <QNetworkProxy>

#if defined (FEEDLY_OFFICIAL_SUPPORT)
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

#if defined (FEEDLY_OFFICIAL_SUPPORT)
  private slots:
    void onAuthFailed();
    void onAuthError(const QString& error, const QString& detailed_description);
    void onAuthGranted();

  private:
    void hookNetwork();
#endif

  private:
    Ui::FeedlyAccountDetails m_ui;

#if defined (FEEDLY_OFFICIAL_SUPPORT)
    OAuth2Service* m_oauth;
#endif
};

#endif // FEEDLYACCOUNTDETAILS_H
