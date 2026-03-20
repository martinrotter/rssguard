#ifndef XMPPNETWORK_H
#define XMPPNETWORK_H

#include <QObject>
#include <QXmppConfiguration.h>
#include <QXmppError.h>

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

    QXmppConfiguration xmppConfiguration() const;

    // Managers and clients.
    QXmppClient* xmppClient() const;
    QXmppDiscoveryManager* discoveryManager() const;
    PubSubManager* pubSubManager() const;

    // API.
    void connectToServer();
    void disconnectFromServer();

    RootItem* obtainServicesNodesTree() const;

    // Statics.
    static QStringList defaultExtraServices();

  private slots:
    void onClientConnected();
    void onClientDisconnected();
    void onClientError(const QXmppError& error);

  private:
    XmppServiceRoot* m_root;
    QXmppClient* m_xmppClient;
    QXmppDiscoveryManager* m_discoveryManager;
    PubSubManager* m_pubSubManager;

    QString m_username;
    QString m_password;
    QString m_domain;
    QStringList m_extraServices;
};

#endif // XMPPNETWORK_H
