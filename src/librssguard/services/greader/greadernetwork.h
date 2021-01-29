// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GREADERNETWORK_H
#define GREADERNETWORK_H

#include <QObject>

#include "network-web/networkfactory.h"
#include "services/abstract/feed.h"
#include "services/greader/greaderserviceroot.h"

class GreaderNetwork : public QObject {
  Q_OBJECT

  public:
    enum class Operations {
      ClientLogin,
      TagList,
      SubscriptionList,
      StreamContents
    };

    explicit GreaderNetwork(QObject* parent = nullptr);

    // Stream contents for a feed/label/etc.
    QList<Message> streamContents(ServiceRoot* root, const QString& stream_id,
                                  Feed::Status& error, const QNetworkProxy& proxy);

    // Downloads and structures full tree for sync-in.
    RootItem* categoriesFeedsLabelsTree(bool obtain_icons, const QNetworkProxy& proxy);

    // Performs client login, if successful, then saves SID, LSID and Auth.
    QNetworkReply::NetworkError clientLogin(const QNetworkProxy& proxy);

    // Getters/setters.
    GreaderServiceRoot::Service service() const;
    void setService(const GreaderServiceRoot::Service& service);

    QString username() const;
    void setUsername(const QString& username);

    QString password() const;
    void setPassword(const QString& password);

    QString baseUrl() const;
    void setBaseUrl(const QString& base_url);

    int batchSize() const;
    void setBatchSize(int batch_size);

    void clearCredentials();

    static QString serviceToString(GreaderServiceRoot::Service service);

  private:
    QPair<QByteArray, QByteArray> authHeader() const;

    // Make sure we are logged in and if we are not, return error.
    bool ensureLogin(const QNetworkProxy& proxy);

    QList<Message> decodeStreamContents(ServiceRoot* root, const QString& stream_json_data, const QString& stream_id);
    RootItem* decodeTagsSubscriptions(const QString& categories, const QString& feeds, bool obtain_icons);
    QString sanitizedBaseUrl() const;
    QString generateFullUrl(Operations operation) const;

  private:
    GreaderServiceRoot::Service m_service;
    QString m_username;
    QString m_password;
    QString m_baseUrl;
    int m_batchSize;
    QString m_authSid;
    QString m_authAuth;
};

#endif // GREADERNETWORK_H
