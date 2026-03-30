#include "src/xmppnetwork.h"

#include "src/xmppentrypoint.h"
#include "src/xmppfeed.h"
#include "src/xmppubsubpmanager.h"

#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/textfactory.h>
#include <librssguard/network-web/webfactory.h>
#include <librssguard/services/abstract/category.h>
#include <qtlinq/qtlinq.h>

#include <QXmppAuthenticationError.h>
#include <QXmppDiscoveryManager.h>
#include <QXmppPubSubSubscription.h>

#define SYNC_IN_IDLE_TIMEOUT 6000
#define XMPP_PROTOCOL_PUBSUB QSL("http://jabber.org/protocol/pubsub")

XmppNetwork::XmppNetwork(XmppServiceRoot* parent)
  : QObject(parent), m_root(parent), m_xmppClient(new QXmppClient(this)),
    m_discoveryManager(new QXmppDiscoveryManager()), m_pubSubManager(new PubSubManager(this)),
    m_extraServices(defaultExtraServices()) {
  m_discoveryManager->setParent(this);

  m_syncInTimer.setSingleShot(true);

  m_xmppClient->logger()->setLoggingType(QXmppLogger::LoggingType::SignalLogging);

  m_xmppClient->addExtension(m_discoveryManager);
  m_xmppClient->addExtension(m_pubSubManager);

  connect(m_xmppClient->logger(), &QXmppLogger::message, this, &XmppNetwork::onNewLogEntry);
  connect(m_xmppClient, &QXmppClient::connected, this, &XmppNetwork::onClientConnected);
  connect(m_xmppClient, &QXmppClient::disconnected, this, &XmppNetwork::onClientDisconnected);
  connect(m_xmppClient, &QXmppClient::errorOccurred, this, &XmppNetwork::onClientError);

  if (m_root != nullptr) {
    connect(m_pubSubManager, &PubSubManager::pushArticleObtained, m_root, &::XmppServiceRoot::pushArticleObtained);
  }
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

void XmppNetwork::fetchSubscriptions(const QString& service, RootItem* new_tree) {
  auto task = m_pubSubManager->requestSubscriptions(service);

  task.then(this, [=, this](auto result) {
    m_syncInTimer.start();

    if (auto subs = std::get_if<QList<QXmppPubSubSubscription>>(&result)) {
      if (!subs->isEmpty()) {
        if (m_syncInSent) {
          return;
        }

        Category* service_folder = new Category();
        QString tld = qApp->web()->urlToTld(QUrl::fromUserInput(service));
        QPixmap output_icon;

        if (NetworkFactory::downloadIcon({IconLocation(tld, false)}, 5000, output_icon, {}, m_root->networkProxy()) ==
            QNetworkReply::NetworkError::NoError) {
          service_folder->setIcon(QIcon(output_icon));
        }
        else {
          service_folder->setIcon(IconFactory::fromColor(TextFactory::generateColorFromText(service),
                                                         service.trimmed()[0]));
        }

        service_folder->setCustomId(service);
        service_folder->setTitle(service);

        new_tree->appendChild(service_folder);

        for (const QXmppPubSubSubscription& sub : *subs) {
          XmppFeed* feed = new XmppFeed();

          feed->setTitle(sub.node());
          feed->setCustomId(sub.node());
          feed->setIcon(IconFactory::fromColor(TextFactory::generateColorFromText(sub.node()),
                                               sub.node().trimmed()[0]));

          service_folder->appendChild(feed);
        }
      }
    }
    else if (QXmppError* error = std::get_if<QXmppError>(&result)) {
      QString desc = XmppSimpleError::fromQXmppError(*error).m_description;

      qDebugNN << LOGSEC_XMPP << "Subscription checking failed:" << NONQUOTE_W_SPACE_DOT(desc);
    }
    else {
      qDebugNN << LOGSEC_XMPP << "Subscription checking failed with unspecified error.";
    }

    m_syncInPendingServices.removeAll(service);

    if (m_syncInPendingServices.isEmpty()) {
      reportSyncInFinish(new_tree);
    }
  });
}

void XmppNetwork::reportSyncInFinish(const ServiceRoot::SyncInResult& result, bool timed_out) {
  bool should_send = !m_syncInSent && m_xmppClient->isConnected();

  m_syncInSent = true;
  m_syncInTimer.disconnect(this);
  m_syncInTimer.stop();
  m_syncInPendingServices.clear();

  if (should_send) {
    emit m_root->syncInFinished(result);
  }
  else {
    qWarningNN
      << LOGSEC_XMPP
      << "Finish of sync-in discovery is reported, but either client is not running or the sync-in was already sent.";
  }
}

QStringList XmppNetwork::xeps() const {
  return m_xeps;
}

QString XmppNetwork::clientState() const {
  switch (m_xmppClient->state()) {
    case QXmppClient::DisconnectedState:
      return tr("disconnected");

    case QXmppClient::ConnectingState:
      return tr("connecting");

    case QXmppClient::ConnectedState:
      return tr("connected");

    default:
      return tr("unknown");
  }
}

void XmppNetwork::discoverService(const QString& jid, RootItem* new_tree) {
  m_syncInPendingServices.append(jid);

  auto task = m_discoveryManager->info(jid);

  task.then(this, [=, this](auto result) {
    m_syncInTimer.start();

    if (QXmppDiscoInfo* info = std::get_if<QXmppDiscoInfo>(&result)) {
      if (info->features().contains(XMPP_PROTOCOL_PUBSUB)) {
        fetchSubscriptions(jid, new_tree);
        return;
      }
    }
    else if (QXmppError* error = std::get_if<QXmppError>(&result)) {
      QString desc = XmppSimpleError::fromQXmppError(*error).m_description;

      qWarningNN << LOGSEC_XMPP << "Service checking failed:" << NONQUOTE_W_SPACE_DOT(desc);
    }

    m_syncInPendingServices.removeAll(jid);

    if (m_syncInPendingServices.isEmpty()) {
      reportSyncInFinish(new_tree);
    }
  });
}

void XmppNetwork::obtainServicesNodesTree() {
  m_syncInPendingServices.clear();
  m_syncInTimer.disconnect(this);
  m_syncInTimer.stop();
  m_syncInTimer.setInterval(6000);
  m_syncInSent = false;

  RootItem* root = new RootItem();

  connect(
    &m_syncInTimer,
    &QTimer::timeout,
    this,
    [=, this]() {
      reportSyncInFinish(root, true);
    },
#if QT_VERSION_MAJOR >= 6
    Qt::ConnectionType::SingleShotConnection);
#else
    Qt::ConnectionType::AutoConnection);
#endif

  auto task = m_discoveryManager->items(m_xmppClient->configuration().domain());

  task.then(this, [=, this](auto result) {
    m_syncInTimer.start();

    if (auto items = std::get_if<QList<QXmppDiscoItem>>(&result)) {
      QStringList all_services = extraServices();

      for (const QXmppDiscoItem& item : *items) {
        all_services.append(item.jid());
      }

      all_services.removeDuplicates();

      for (const auto& serv : all_services) {
        const auto serv_trim = serv.trimmed();

        if (serv_trim.isEmpty()) {
          continue;
        }

        discoverService(serv_trim, root);
      }
    }
    else if (QXmppError* error = std::get_if<QXmppError>(&result)) {
      delete root;

      QString desc = XmppSimpleError::fromQXmppError(*error).m_description;
      qDebugNN << LOGSEC_XMPP << "Service discovery failed:" << NONQUOTE_W_SPACE_DOT(desc);
      reportSyncInFinish(ApplicationException(tr("error during services discovery, %1").arg(desc)));
    }
    else {
      delete root;

      qDebugNN << LOGSEC_XMPP << "Service discovery failed with unspecified error. This is a problem.";
      reportSyncInFinish(ApplicationException(tr("unspecified error during services discovery")));
    }
  });
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

QHash<QString, QString> XmppNetwork::xepMappings() {
  QHash<QString, QString> xeps = {
    {QSL("gc-1.0"), QSL("XEP-0045")},

    {QSL("http://jabber.org/protocol/activity"), QSL("XEP-0108")},
    {QSL("http://jabber.org/protocol/address"), QSL("XEP-0033")},
    {QSL("http://jabber.org/protocol/amp"), QSL("XEP-0079")},
    {QSL("http://jabber.org/protocol/amp#errors"), QSL("XEP-0079")},
    {QSL("http://jabber.org/protocol/amp?action=alert"), QSL("XEP-0079")},
    {QSL("http://jabber.org/protocol/amp?action=drop"), QSL("XEP-0079")},
    {QSL("http://jabber.org/protocol/amp?action=error"), QSL("XEP-0079")},
    {QSL("http://jabber.org/protocol/amp?action=notify"), QSL("XEP-0079")},
    {QSL("http://jabber.org/protocol/amp?condition=deliver"), QSL("XEP-0079")},
    {QSL("http://jabber.org/protocol/amp?condition=expire-at"), QSL("XEP-0079")},
    {QSL("http://jabber.org/protocol/amp?condition=match-resource"), QSL("XEP-0079")},

    {QSL("http://jabber.org/protocol/bytestreams"), QSL("XEP-0065")},
    {QSL("http://jabber.org/protocol/bytestreams#udp"), QSL("XEP-0065")},

    {QSL("http://jabber.org/protocol/caps"), QSL("XEP-0115")},
    {QSL("http://jabber.org/protocol/caps#optimize"), QSL("XEP-0115")},

    {QSL("http://jabber.org/protocol/chatstates"), QSL("XEP-0085")},
    {QSL("http://jabber.org/protocol/commands"), QSL("XEP-0050")},
    {QSL("http://jabber.org/protocol/compress"), QSL("XEP-0138")},

    {QSL("http://jabber.org/protocol/disco#info"), QSL("XEP-0030")},
    {QSL("http://jabber.org/protocol/disco#items"), QSL("XEP-0030")},

    {QSL("http://jabber.org/protocol/feature-neg"), QSL("XEP-0020")},
    {QSL("http://jabber.org/protocol/geoloc"), QSL("XEP-0080")},
    {QSL("http://jabber.org/protocol/http-auth"), QSL("XEP-0072")},
    {QSL("http://jabber.org/protocol/httpbind"), QSL("XEP-0124")},
    {QSL("http://jabber.org/protocol/ibb"), QSL("XEP-0047")},
    {QSL("http://jabber.org/protocol/mood"), QSL("XEP-0107")},

    {QSL("http://jabber.org/protocol/muc"), QSL("XEP-0045")},
    {QSL("http://jabber.org/protocol/muc#admin"), QSL("XEP-0045")},
    {QSL("http://jabber.org/protocol/muc#owner"), QSL("XEP-0045")},
    {QSL("http://jabber.org/protocol/muc#register"), QSL("XEP-0045")},
    {QSL("http://jabber.org/protocol/muc#roomconfig"), QSL("XEP-0045")},
    {QSL("http://jabber.org/protocol/muc#roominfo"), QSL("XEP-0045")},
    {QSL("http://jabber.org/protocol/muc#user"), QSL("XEP-0045")},

    {QSL("http://jabber.org/protocol/offline"), QSL("XEP-0013")},
    {QSL("http://jabber.org/protocol/physloc"), QSL("XEP-0080")},

    {QSL("http://jabber.org/protocol/pubsub#access-authorize"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#access-open"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#access-presence"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#access-roster"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#access-whitelist"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#auto-create"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#auto-subscribe"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#collections"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#config-node"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#create-and-configure"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#create-nodes"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#delete-any"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#delete-nodes"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#filtered-notifications"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#get-pending"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#instant-nodes"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#item-ids"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#last-published"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#leased-subscription"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#manage-subscription"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#member-affiliation"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#meta-data"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#modify-affiliations"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#multi-collection"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#multi-subscribe"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#outcast-affiliation"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#persistent-items"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#presence-notifications"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#presence-subscribe"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#publish"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#publish-options"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#publisher-affiliation"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#purge-nodes"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#retract-items"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#retrieve-affiliations"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#retrieve-default"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#retrieve-items"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#retrieve-subscriptions"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#subscribe"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#subscription-options"), QSL("XEP-0060")},
    {QSL("http://jabber.org/protocol/pubsub#subscription-notifications"), QSL("XEP-0060")},

    {QSL("http://jabber.org/protocol/rosterx"), QSL("XEP-0144")},
    {QSL("http://jabber.org/protocol/sipub"), QSL("XEP-0137")},
    {QSL("http://jabber.org/protocol/soap"), QSL("XEP-0072")},
    {QSL("http://jabber.org/protocol/soap#fault"), QSL("XEP-0072")},
    {QSL("http://jabber.org/protocol/waitinglist"), QSL("XEP-0130")},
    {QSL("http://jabber.org/protocol/waitinglist/schemes/mailto"), QSL("XEP-0130")},
    {QSL("http://jabber.org/protocol/waitinglist/schemes/tel"), QSL("XEP-0130")},
    {QSL("http://jabber.org/protocol/xhtml-im"), QSL("XEP-0071")},
    {QSL("http://jabber.org/protocol/xdata-layout"), QSL("XEP-0141")},
    {QSL("http://jabber.org/protocol/xdata-validate"), QSL("XEP-0122")},

    {QSL("jabber:component:accept"), QSL("XEP-0114")},
    {QSL("jabber:component:connect"), QSL("XEP-0114")},
    {QSL("jabber:iq:auth"), QSL("XEP-0078")},
    {QSL("jabber:iq:browse"), QSL("XEP-0011")},
    {QSL("jabber:iq:gateway"), QSL("XEP-0100")},
    {QSL("jabber:iq:last"), QSL("XEP-0012")},
    {QSL("jabber:iq:oob"), QSL("XEP-0066")},
    {QSL("jabber:iq:pass"), QSL("XEP-0003")},
    {QSL("jabber:iq:private"), QSL("XEP-0049")},
    {QSL("jabber:iq:register"), QSL("XEP-0077")},
    {QSL("jabber:iq:rpc"), QSL("XEP-0009")},
    {QSL("jabber:iq:search"), QSL("XEP-0055")},
    {QSL("jabber:iq:time"), QSL("XEP-0202")},
    {QSL("jabber:iq:version"), QSL("XEP-0092")},

    {QSL("jabber:x:data"), QSL("XEP-0004")},
    {QSL("jabber:x:delay"), QSL("XEP-0203")},
    {QSL("jabber:x:encrypted"), QSL("XEP-0027")},
    {QSL("jabber:x:event"), QSL("XEP-0022")},
    {QSL("jabber:x:expire"), QSL("XEP-0023")},
    {QSL("jabber:x:oob"), QSL("XEP-0066")},
    {QSL("jabber:x:roster"), QSL("XEP-0093")},
    {QSL("jabber:x:signed"), QSL("XEP-0027")},

    {QSL("msgoffline"), QSL("XEP-0160")},

    {QSL("muc_hidden"), QSL("XEP-0045")},
    {QSL("muc_membersonly"), QSL("XEP-0045")},
    {QSL("muc_moderated"), QSL("XEP-0045")},
    {QSL("muc_nonanonymous"), QSL("XEP-0045")},
    {QSL("muc_open"), QSL("XEP-0045")},
    {QSL("muc_passwordprotected"), QSL("XEP-0045")},
    {QSL("muc_persistent"), QSL("XEP-0045")},
    {QSL("muc_public"), QSL("XEP-0045")},
    {QSL("muc_rooms"), QSL("XEP-0045")},
    {QSL("muc_semianonymous"), QSL("XEP-0045")},
    {QSL("muc_temporary"), QSL("XEP-0045")},
    {QSL("muc_unmoderated"), QSL("XEP-0045")},
    {QSL("muc_unsecured"), QSL("XEP-0045")},

    {QSL("roster:delimiter"), QSL("XEP-0083")},

    {QSL("urn:ietf:rfc:3264"), QSL("XEP-0176")},

    {QSL("urn:xmpp:archive:auto"), QSL("XEP-0136")},
    {QSL("urn:xmpp:archive:manage"), QSL("XEP-0136")},
    {QSL("urn:xmpp:archive:manual"), QSL("XEP-0136")},
    {QSL("urn:xmpp:archive:pref"), QSL("XEP-0136")},

    {QSL("urn:xmpp:avatar:data"), QSL("XEP-0084")},
    {QSL("urn:xmpp:avatar:metadata"), QSL("XEP-0084")},

    {QSL("urn:xmpp:delay"), QSL("XEP-0203")},

    {QSL("urn:xmpp:jingle:apps:rtp:audio"), QSL("XEP-0167")},
    {QSL("urn:xmpp:jingle:apps:rtp:video"), QSL("XEP-0167")},

    {QSL("urn:xmpp:ping"), QSL("XEP-0199")},
    {QSL("urn:xmpp:receipts"), QSL("XEP-0184")},
    {QSL("urn:xmpp:sid:0"), QSL("XEP-0359")},
    {QSL("urn:xmpp:ssn"), QSL("XEP-0155")},
    {QSL("urn:xmpp:time"), QSL("XEP-0202")},

    {QSL("vcard-temp"), QSL("XEP-0054")},

    {QSL("urn:xmpp:caps"), QSL("XEP-0390")},
    {QSL("urn:xmpp:caps:optimize"), QSL("XEP-0390")},

    {QSL("urn:xmpp:styling:0"), QSL("XEP-0393")}};

  return xeps;
}

void XmppNetwork::reconnect() {
  if (m_xmppClient->isConnected()) {
    QMetaObject::Connection conn_id = connect(m_xmppClient, &QXmppClient::disconnected, this, [&, this]() {
      disconnect(conn_id);
      connectToServer();
    });

    disconnectFromServer();
  }
  else {
    connectToServer();
  }
}

void XmppNetwork::onNewLogEntry(QXmppLogger::MessageType type, const QString& text) {
  qDebugNN << LOGSEC_XMPP << text;
}

void XmppNetwork::onClientConnected() {
  m_xmppClient->configuration().setAutoReconnectionEnabled(true);

  qApp->showGuiMessage(Notification::Event::LoginProgressOrSuccessful,
                       GuiMessage(tr("XMPP server connected"),
                                  tr("XMPP connection to server %1 is alive.").arg(m_domain),
                                  QSystemTrayIcon::MessageIcon::Information,
                                  XmppEntryPoint().icon()));

  if (m_root != nullptr && m_root->getSubTreeFeeds().isEmpty()) {
    m_root->requestSyncIn();
    m_root->requestItemExpand({m_root}, true);
  }

  m_discoveryManager->info(m_xmppClient->configuration().domain()).then(this, [this](auto result) {
    if (QXmppDiscoInfo* info = std::get_if<QXmppDiscoInfo>(&result)) {
      auto f = info->features();

      static auto xep_map = xepMappings();

      m_xeps = qlinq::from(f)
                 .select([](const QString& protocol) -> QString {
                   QString xep = xep_map.value(protocol);

                   return xep.isEmpty() ? protocol : xep;
                 })
                 .where([](const QString& xep) {
                   return !xep.isEmpty();
                 })
                 .distinct()
                 .toList();
    }
    else if (QXmppError* error = std::get_if<QXmppError>(&result)) {
      QString desc = XmppSimpleError::fromQXmppError(*error).m_description;

      qDebugNN << LOGSEC_XMPP << "Service checking failed:" << NONQUOTE_W_SPACE_DOT(desc);
    }
  });
}

void XmppNetwork::onClientDisconnected() {
  qApp->showGuiMessage(Notification::Event::Logout,
                       GuiMessage(tr("XMPP server disconnected"),
                                  tr("XMPP connection to server %1 is down.").arg(m_domain),
                                  QSystemTrayIcon::MessageIcon::Warning));
}

void XmppNetwork::onClientError(const QXmppError& error) {
  qApp->showGuiMessage(Notification::Event::GeneralEvent,
                       GuiMessage(tr("XMPP error"),
                                  tr("XMPP connection to server %1 is down: %1")
                                    .arg(m_domain, XmppSimpleError::fromQXmppError(error).m_description),
                                  QSystemTrayIcon::MessageIcon::Critical));
}

XmppServiceRoot* XmppNetwork::root() const {
  return m_root;
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
