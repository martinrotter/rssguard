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
    explicit GreaderNetwork(QObject* parent = nullptr);

    // Network operations.
    QList<Message> messages(ServiceRoot* root, const QString& stream_id, Feed::Status& error);

    NetworkResult status(const QNetworkProxy& custom_proxy) const;

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
    GreaderServiceRoot::Service m_service;
    QString m_username;
    QString m_password;
    QString m_baseUrl;
    int m_batchSize;
};

#endif // GREADERNETWORK_H
