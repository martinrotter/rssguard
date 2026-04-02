#ifndef XMPPNETWORK_H
#define XMPPNETWORK_H

#include "src/xmppfeed.h"
#include "src/xmppserviceroot.h"

#include <QObject>
#include <QTimer>
#include <QXmppConfiguration.h>
#include <QXmppDiscoveryIq.h>
#include <QXmppError.h>
#include <QXmppLogger.h>

class RootItem;
class PubSubManager;
class QXmppClient;
class QXmppMucManager;
class QXmppDiscoveryManager;

struct XmppSimpleError {
    QString m_description;

    static XmppSimpleError fromQXmppError(const QXmppError& error);
};

class XmppNetwork : public QObject {
    Q_OBJECT

  public:
    explicit XmppNetwork(XmppServiceRoot* parent = nullptr);

    // Properties.
    QString username() const;
    void setUsername(const QString& username);

    QString password() const;
    void setPassword(const QString& password);

    QString domain() const;
    void setDomain(const QString& domain);

    QStringList extraNodes() const;
    void setExtraNodes(const QStringList& nodes);

    XmppServiceRoot* root() const;
    QXmppConfiguration xmppConfiguration() const;

    QStringList xeps() const;

    QString clientState() const;

    // Managers and clients.
    QXmppClient* xmppClient() const;
    QXmppDiscoveryManager* discoveryManager() const;
    PubSubManager* pubSubManager() const;

    // Statics.
    static QStringList defaultExtraServices();
    static QHash<QString, QString> xepMappings();

  public slots:
    void connectToServer();
    void disconnectFromServer();
    void obtainServicesNodesTree();
    void reconnect();

  private slots:
    void onNewLogEntry(QXmppLogger::MessageType type, const QString& text);
    void onClientConnected();
    void onClientDisconnected();
    void onClientError(const QXmppError& error);
    void onMessageReceived(const QXmppMessage& message);

  private:
    void joinRooms();
    void discoverService(const QString& jid, RootItem* new_tree);
    void fetchPubSubSubscriptions(const QString& service, XmppFeed::Type pubsub_type, RootItem* new_tree);
    void fetchChatroom(const QString& chatroom,
                       const QXmppDiscoInfo& info,
                       XmppFeed::Type chatroom_type,
                       RootItem* new_tree);
    void reportSyncInFinish(const ServiceRoot::SyncInResult& result, bool timed_out = false);
    void finalizeSyncInFinish(const QString& jid_to_remove, RootItem* new_tree);

  private:
    XmppServiceRoot* m_root;
    QXmppClient* m_xmppClient;
    QXmppDiscoveryManager* m_discoveryManager;
    PubSubManager* m_pubSubManager;
    QXmppMucManager* m_mucManager;

    QString m_username;
    QString m_password;
    QString m_domain;
    QStringList m_extraNodes;
    QStringList m_xeps;
    QTimer m_syncInTimer;

    QList<QString> m_syncInPendingServices;
    bool m_syncInSent;
};

#endif // XMPPNETWORK_H
