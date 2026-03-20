#include "src/xmppnetwork.h"

#include "src/xmppserviceroot.h"
#include "src/xmppubsubpmanager.h"

#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>

#include <QXmppAuthenticationError.h>
#include <QXmppDiscoveryManager.h>

XmppNetwork::XmppNetwork(XmppServiceRoot* parent)
  : QObject(parent), m_root(parent), m_xmppClient(new QXmppClient(this)),
    m_discoveryManager(new QXmppDiscoveryManager()), m_pubSubManager(new PubSubManager(this)),
    m_extraServices(defaultExtraServices()) {
  m_discoveryManager->setParent(this);

  connect(m_xmppClient, &QXmppClient::connected, this, &XmppNetwork::onClientConnected);
  connect(m_xmppClient, &QXmppClient::disconnected, this, &XmppNetwork::onClientDisconnected);
  connect(m_xmppClient, &QXmppClient::errorOccurred, this, &XmppNetwork::onClientError);
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

void XmppNetwork::connectToServer() {
  m_xmppClient->connectToServer(xmppConfiguration());
}

void XmppNetwork::disconnectFromServer() {
  m_xmppClient->disconnectFromServer();
}

RootItem* XmppNetwork::obtainServicesNodesTree() const {
  RootItem* root = new RootItem();

  return root;
}

QStringList XmppNetwork::defaultExtraServices() {
  return {QSL("alt.movim.eu"),
          QSL("blabla.movim.eu"),
          QSL("blog.teftera.com"),
          QSL("comics.movim.eu"),
          QSL("comics.xmpp.com"),
          QSL("comments.chalec.org"),
          QSL("comments.monocles.eu"),
          QSL("comments.movim.eu"),
          QSL("comments.xmpp.earth"),
          QSL("earth.movim.eu"),
          QSL("feed.31337.one"),
          QSL("feed.xmpp.earth"),
          QSL("gaming.movim.eu"),
          QSL("news.chalec.org"),
          QSL("news.movim.eu"),
          QSL("news.xmpp.com"),
          QSL("news.xmpp.org"),
          QSL("nsfw.movim.eu"),
          QSL("nsfw.xmpp.com"),
          QSL("pictures.movim.eu"),
          QSL("pubsub.07f.de"),
          QSL("pubsub.31337.one"),
          QSL("pubsub.chalec.org"),
          QSL("pubsub.chatterboxtown.us"),
          QSL("pubsub.conversations.im"),
          QSL("pubsub.dazeilad.eu"),
          QSL("pubsub.disroot.org"),
          QSL("pubsub.dw.live"),
          QSL("pubsub.envs.net"),
          QSL("pubsub.jabb3r.org"),
          QSL("pubsub.jabber.cz"),
          QSL("pubsub.kirgroup.net"),
          QSL("pubsub.lightwitch.org"),
          QSL("pubsub.linuxoid.in"),
          QSL("pubsub.monocles.eu"),
          QSL("pubsub.movim.eu"),
          QSL("pubsub.openim.de"),
          QSL("pubsub.openim.nl"),
          QSL("pubsub.sakulstar.org"),
          QSL("pubsub.slavino.sk"),
          QSL("pubsub.slidge.im"),
          QSL("pubsub.slrpnk.net"),
          QSL("pubsub.snug.moe"),
          QSL("pubsub.suchat.org"),
          QSL("pubsub.tea.bingo"),
          QSL("pubsub.thecat.zone"),
          QSL("pubsub.thrivecommunitygroup.net"),
          QSL("pubsub.xabber.org"),
          QSL("pubsub.xmpp.earth"),
          QSL("pubsub.xmpp.jp"),
          QSL("pubsub.xmpp.newboerg"),
          QSL("pubsub.xmpp.social"),
          QSL("pubsub.yax.im"),
          QSL("spaces.movim.eu"),
          QSL("sport.movim.eu")};
}

void XmppNetwork::onClientConnected() {
  qApp->showGuiMessage(Notification::Event::LoginProgressOrSuccessful,
                       GuiMessage(tr("XMPP server connected"),
                                  tr("XMPP connection to server %1 is alive.").arg(m_domain),
                                  QSystemTrayIcon::MessageIcon::Information,
                                  m_root->icon()));
}

void XmppNetwork::onClientDisconnected() {
  qApp->showGuiMessage(Notification::Event::Logout,
                       GuiMessage(tr("XMPP server disconnected"),
                                  tr("XMPP connection to server %1 is down.").arg(m_domain)));
}

void XmppNetwork::onClientError(const QXmppError& error) {
  qApp->showGuiMessage(Notification::Event::GeneralEvent,
                       GuiMessage(tr("XMPP error"), tr("XMPP connection to server %1 is down.").arg(m_domain)));
}

QStringList XmppNetwork::extraServices() const {
  return m_extraServices;
}

void XmppNetwork::setExtraServices(const QStringList& services) {
  m_extraServices = services;
}

QXmppConfiguration XmppNetwork::xmppConfiguration() const {
  QXmppConfiguration conf;

  conf.setPassword(password());
  conf.setJid(username());
  conf.setDomain(domain());

  if (m_root != nullptr && m_root->networkProxy().type() != QNetworkProxy::ProxyType::DefaultProxy) {
    conf.setNetworkProxy(m_root->networkProxy());
  }

  return conf;
}

XmppSimpleError XmppSimpleError::fromQXmppError(const QXmppError& error) {
  XmppSimpleError err;

  if (error.isNetworkError()) {
    std::optional<QNetworkReply::NetworkError> net_err = error.value<QNetworkReply::NetworkError>();
    err.m_description = QObject::tr("network error: '%1'.").arg(NetworkFactory::networkErrorText(*net_err));
  }
  else if (error.isFileError()) {
    std::optional<QFileDevice::FileError> fil_err = error.value<QFileDevice::FileError>();
    err.m_description = QObject::tr("file error: '%1'.").arg(QString::number(*fil_err));
  }
  else if (error.isStanzaError()) {
    std::optional<QXmppStanza::Error> sta_err = error.value<QXmppStanza::Error>();
    err.m_description =
      QObject::tr("client error: '%1 - %2'.").arg(QString::number((*sta_err).type()), (*sta_err).text());
  }
  else {
    std::optional<QXmpp::AuthenticationError> auth_err = error.value<QXmpp::AuthenticationError>();

    if (auth_err.has_value()) {
      err.m_description =
        QObject::tr("auth error: '%1 - %2'.").arg(QString::number((*auth_err).type), (*auth_err).text);
    }
    else {
      err.m_description = QObject::tr("error: '%1'.").arg(error.description);
    }
  }

  return err;
}
