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
    void untagEntries(const QString& tag_id, const QStringList& msg_custom_ids);
    void tagEntries(const QString& tag_id, const QStringList& msg_custom_ids);
    void markers(const QString& action, const QStringList& msg_custom_ids);
    QList<Message> streamContents(const QString& stream_id);
    QVariantHash profile(const QNetworkProxy& network_proxy);
    QList<RootItem*> tags();
    RootItem* collections(bool obtain_icons);

    // Getters and setters.
    QString username() const;
    void setUsername(const QString& username);

    QString developerAccessToken() const;
    void setDeveloperAccessToken(const QString& dev_acc_token);

    bool downloadOnlyUnreadMessages() const;
    void setDownloadOnlyUnreadMessages(bool download_only_unread_messages);

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
      Collections,
      Tags,
      StreamContents,
      Markers,
      TagEntries
    };

    QString fullUrl(Service service) const;
    QString bearer() const;
    QList<Message> decodeStreamContents(const QByteArray& stream_contents, QString& continuation) const;
    RootItem* decodeCollections(const QByteArray& json, bool obtain_icons, const QNetworkProxy& proxy, int timeout = 0) const;
    QPair<QByteArray, QByteArray> bearerHeader(const QString& bearer) const;

  private:
    FeedlyServiceRoot* m_service;

#if defined (FEEDLY_OFFICIAL_SUPPORT)
    OAuth2Service* m_oauth;
#endif

    QString m_username;
    QString m_developerAccessToken;

    // Only download N newest messages per feed.
    int m_batchSize;

    // Only download unread messages.
    bool m_downloadOnlyUnreadMessages;
};

#endif // FEEDLYNETWORK_H
