// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef INOREADERNETWORKFACTORY_H
#define INOREADERNETWORKFACTORY_H

#include <QObject>

#include "core/message.h"

#include "services/abstract/feed.h"
#include "services/abstract/rootitem.h"

#include <QNetworkReply>

class RootItem;
class InoreaderServiceRoot;
class OAuth2Service;

class InoreaderNetworkFactory : public QObject {
  Q_OBJECT

  public:
    explicit InoreaderNetworkFactory(QObject* parent = nullptr);

    void setService(InoreaderServiceRoot* service);

    OAuth2Service* oauth() const;

    QString userName() const;
    void setUsername(const QString& username);

    // Gets/sets the amount of messages to obtain during single feed update.
    int batchSize() const;
    void setBatchSize(int batch_size);

    // Returns tree of feeds/categories.
    // Top-level root of the tree is not needed here.
    // Returned items do not have primary IDs assigned.
    RootItem* feedsCategories(bool obtain_icons);

    QList<RootItem*> getLabels();

    QList<Message> messages(ServiceRoot* root, const QString& stream_id, Feed::Status& error);

    QNetworkReply::NetworkError editLabels(const QString& state, bool assign, const QStringList& msg_custom_ids);

    QNetworkReply::NetworkError markMessagesRead(RootItem::ReadStatus status, const QStringList& msg_custom_ids);
    QNetworkReply::NetworkError markMessagesStarred(RootItem::Importance importance, const QStringList& msg_custom_ids);

  private slots:
    void onTokensError(const QString& error, const QString& error_description);
    void onAuthFailed();

  private:
    QList<Message> decodeMessages(ServiceRoot* root, const QString& messages_json_data, const QString& stream_id);
    RootItem* decodeFeedCategoriesData(const QString& categories, const QString& feeds, bool obtain_icons);

    void initializeOauth();

  private:
    InoreaderServiceRoot* m_service;
    QString m_username;
    int m_batchSize;
    OAuth2Service* m_oauth2;
};

#endif // INOREADERNETWORKFACTORY_H
