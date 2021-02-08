// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDLYNETWORK_H
#define FEEDLYNETWORK_H

#include <QObject>

#include "network-web/networkfactory.h"
#include "services/abstract/feed.h"

#if defined (FEEDLY_OFFICIAL_SUPPORT)
class OAuth2Service;
#endif

class FeedlyServiceRoot;

class FeedlyNetwork : public QObject {
  Q_OBJECT

  public:
    explicit FeedlyNetwork(QObject* parent = nullptr);

    QString username() const;
    void setUsername(const QString& username);

    QString developerAccessToken() const;
    void setDeveloperAccessToken(const QString& dev_acc_token);

    int batchSize() const;
    void setBatchSize(int batch_size);

    void setService(FeedlyServiceRoot* service);

#if defined (FEEDLY_OFFICIAL_SUPPORT)
    OAuth2Service* oauth() const;
    void setOauth(OAuth2Service* oauth);

  private slots:
    void onTokensError(const QString& error, const QString& error_description);
    void onAuthFailed();
    void onTokensReceived(const QString& access_token, const QString& refresh_token, int expires_in);
#endif

  private:
    FeedlyServiceRoot* m_service;

#if defined (FEEDLY_OFFICIAL_SUPPORT)
    OAuth2Service* m_oauth;
#endif

    QString m_username;
    QString m_developerAccessToken;
    int m_batchSize;
};

#endif // FEEDLYNETWORK_H
