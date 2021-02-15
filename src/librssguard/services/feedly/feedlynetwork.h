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

    // API operations.
    QVariantHash profile(const QNetworkProxy& network_proxy);

    RootItem* collections(bool obtain_icons);

    // Getters and setters.
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
    void onTokensRetrieved(const QString& access_token, const QString& refresh_token, int expires_in);
#endif

  private:
    enum class Service {
      Profile,
      Collections
    };

    QString fullUrl(Service service) const;
    QString bearer() const;
    RootItem* decodeCollections(const QByteArray& json, bool obtain_url) const;
    QPair<QByteArray, QByteArray> bearerHeader(const QString& bearer) const;

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
