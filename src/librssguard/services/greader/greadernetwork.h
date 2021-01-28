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
      ClientLogin
    };

    explicit GreaderNetwork(QObject* parent = nullptr);

    // Network operations.
    QList<Message> messages(ServiceRoot* root, const QString& stream_id, Feed::Status& error);

    // Performs client login, if successful, then saves SID, LSID and Auth.
    QNetworkReply::NetworkError clientLogin(const QNetworkProxy& proxy);

    // Metadata.
    GreaderServiceRoot::Service service() const;
    void setService(const GreaderServiceRoot::Service& service);

    QString username() const;
    void setUsername(const QString& username);

    QString password() const;
    void setPassword(const QString& password);

    QString baseUrl() const;
    void setBaseUrl(const QString& base_url);

    static QString serviceToString(GreaderServiceRoot::Service service);

    int batchSize() const;
    void setBatchSize(int batch_size);

  private:
    QString sanitizedBaseUrl() const;
    QString generateFullUrl(Operations operation) const;

  private:
    GreaderServiceRoot::Service m_service;
    QString m_username;
    QString m_password;
    QString m_baseUrl;
    int m_batchSize;
};

#endif // GREADERNETWORK_H
