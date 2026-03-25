#ifndef XMPPNETWORK_H
#define XMPPNETWORK_H

#include <QObject>
#include <QTimer>
#include <QXmppConfiguration.h>
#include <QXmppError.h>
#include <QXmppLogger.h>

class RootItem;
class XmppServiceRoot;
class PubSubManager;
class QXmppClient;
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

    QStringList extraServices() const;
    void setExtraServices(const QStringList& services);

    XmppServiceRoot* root() const;
    QXmppConfiguration xmppConfiguration() const;

    // Managers and clients.
    QXmppClient* xmppClient() const;
    QXmppDiscoveryManager* discoveryManager() const;
    PubSubManager* pubSubManager() const;

    // API.
    void connectToServer();
    void disconnectFromServer();

    void obtainServicesNodesTree();

    // Statics.
    static QStringList defaultExtraServices();

  private slots:
    void onNewLogEntry(QXmppLogger::MessageType type, const QString& text);
    void onClientConnected();
    void onClientDisconnected();
    void onClientError(const QXmppError& error);

  private:
    void checkService(const QString& jid, RootItem* new_tree);
    void fetchSubscriptions(const QString& service, RootItem* new_tree);

  private:
    XmppServiceRoot* m_root;
    QXmppClient* m_xmppClient;
    QXmppDiscoveryManager* m_discoveryManager;
    PubSubManager* m_pubSubManager;

    QString m_username;
    QString m_password;
    QString m_domain;
    QStringList m_extraServices;

    QTimer m_syncInTimer;
};

#endif // XMPPNETWORK_H
