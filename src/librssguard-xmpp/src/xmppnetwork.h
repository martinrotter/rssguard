#ifndef XMPPNETWORK_H
#define XMPPNETWORK_H

#include <QObject>

class XmppServiceRoot;
class PubSubManager;
class QXmppClient;
class QXmppDiscoveryManager;

class XmppNetwork : public QObject {
    Q_OBJECT

  public:
    explicit XmppNetwork(XmppServiceRoot* parent = nullptr);

    QString username() const;
    void setUsername(const QString& username);

    QString password() const;
    void setPassword(const QString& password);

    QString domain() const;
    void setDomain(const QString& domain);

    QXmppClient* xmppClient() const;
    QXmppDiscoveryManager* discoveryManager() const;
    PubSubManager* pubSubManager() const;

  private:
    QXmppClient* m_xmppClient;
    QXmppDiscoveryManager* m_discoveryManager;
    PubSubManager* m_pubSubManager;

    QString m_username;
    QString m_password;
    QString m_domain;
};

#endif // XMPPNETWORK_H
