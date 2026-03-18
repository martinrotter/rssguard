#include "src/xmppnetwork.h"

#include "src/xmppserviceroot.h"
#include "src/xmppubsubpmanager.h"

#include <QXmppQt6/QXmppDiscoveryManager.h>

XmppNetwork::XmppNetwork(XmppServiceRoot* parent)
  : QObject(parent), m_xmppClient(new QXmppClient(this)), m_discoveryManager(new QXmppDiscoveryManager()),
    m_pubSubManager(new PubSubManager(this)) {
  m_discoveryManager->setParent(this);
}

QString XmppNetwork::username() const {
  return m_username;
}

void XmppNetwork::setUsername(const QString& username) {
  m_username = username;
}

QString XmppNetwork::password() const {
  return m_password;
}

void XmppNetwork::setPassword(const QString& password) {
  m_password = password;
}

QString XmppNetwork::domain() const {
  return m_domain;
}

void XmppNetwork::setDomain(const QString& domain) {
  m_domain = domain;
}

QXmppClient* XmppNetwork::xmppClient() const {
  return m_xmppClient;
}

QXmppDiscoveryManager* XmppNetwork::discoveryManager() const {
  return m_discoveryManager;
}

PubSubManager* XmppNetwork::pubSubManager() const {
  return m_pubSubManager;
}
