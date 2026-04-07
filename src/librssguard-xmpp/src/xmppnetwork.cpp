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
#include <QXmppMucManager.h>
#include <QXmppPubSubSubscription.h>
#include <QXmppUtils.h>

#define SYNC_IN_IDLE_TIMEOUT 6000
#define XMPP_PROTOCOL_PUBSUB QSL("http://jabber.org/protocol/pubsub")

XmppNetwork::XmppNetwork(XmppServiceRoot* parent)
  : QObject(parent), m_root(parent), m_xmppClient(new QXmppClient(this)),
    m_discoveryManager(new QXmppDiscoveryManager()), m_pubSubManager(new PubSubManager(this)),
    m_mucManager(new QXmppMucManager()), m_extraNodes(defaultExtraServices()) {
  m_discoveryManager->setParent(this);
  m_mucManager->setParent(this);

  m_syncInTimer.setSingleShot(true);

  // #if !defined(NDEBUG)
  m_xmppClient->logger()->setLoggingType(QXmppLogger::LoggingType::SignalLogging);
  // #endif

  m_xmppClient->addExtension(m_discoveryManager);
  m_xmppClient->addExtension(m_pubSubManager);
  m_xmppClient->addExtension(m_mucManager);

  connect(m_xmppClient->logger(), &QXmppLogger::message, this, &XmppNetwork::onNewLogEntry);
  connect(m_xmppClient, &QXmppClient::connected, this, &XmppNetwork::onClientConnected);
  connect(m_xmppClient, &QXmppClient::disconnected, this, &XmppNetwork::onClientDisconnected);
  connect(m_xmppClient, &QXmppClient::errorOccurred, this, &XmppNetwork::onClientError);
  connect(m_xmppClient, &QXmppClient::messageReceived, this, &XmppNetwork::onMessageReceived);

  if (m_root != nullptr) {
    connect(m_pubSubManager,
            &PubSubManager::realTimeArticleObtained,
            m_root,
            &::XmppServiceRoot::onRealTimeArticleObtained);
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

void XmppNetwork::finalizeSyncInFinish(const QString& jid_to_remove, RootItem* new_tree) {
  m_syncInPendingServices.removeAll(jid_to_remove);

  if (m_syncInPendingServices.isEmpty()) {
    // reportSyncInFinish(new_tree);
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

void XmppNetwork::obtainServicesNodesTree() {
  m_syncInPendingServices.clear();
  m_syncInTimer.disconnect(this);
  m_syncInTimer.stop();
  m_syncInTimer.setInterval(12000);
  m_syncInSent = false;

  RootItem* root = new RootItem();

  connect(
    &m_syncInTimer,
    &QTimer::timeout,
    this,
    [=, this]() {
      reportSyncInFinish(root, true);
    },
    Qt::ConnectionType::SingleShotConnection);

  auto task = m_discoveryManager->items(m_xmppClient->configuration().domain());

  task.then(this, [=, this](auto result) {
    m_syncInTimer.start();

    if (auto items = std::get_if<QList<QXmppDiscoItem>>(&result)) {
      QStringList all_services = extraNodes();

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

void XmppNetwork::discoverService(const QString& jid, RootItem* new_tree) {
  m_syncInPendingServices.append(jid);

  auto task = m_discoveryManager->info(jid);

  task.then(this, [=, this](auto result) {
    m_syncInTimer.start();

    if (QXmppDiscoInfo* info = std::get_if<QXmppDiscoInfo>(&result)) {
      auto info_linq = qlinq::from(info->identities());

      bool is_pubsub_service = info_linq.any([](const QXmppDiscoIdentity& identity) {
        return identity.category() == QSL("pubsub") && identity.type() == QSL("service");
      });

      // Only add PEPs from individual accounts.
      bool is_pubsub_pep = jid.contains(QL1C('@')) && info_linq.any([](const QXmppDiscoIdentity& identity) {
        return identity.category() == QSL("pubsub") && identity.type() == QSL("pep");
      });

      bool is_chatroom = jid.contains(QL1C('@')) && info_linq.any([](const QXmppDiscoIdentity& identity) {
        return identity.category() == QSL("conference") && identity.type() == QSL("text");
      }) && qlinq::from(info->features()).any([](const QString& feature) {
        return feature == QXmpp::Private::ns_muc;
      });

      bool is_chat = jid.contains(QL1C('@')) && info_linq.any([](const QXmppDiscoIdentity& identity) {
        return identity.category() == QSL("account") && identity.type() == QSL("registered");
      });

      qDebugNN << LOGSEC_XMPP << "Discovering services/nodes from"
               << QUOTE_W_SPACE(jid) "and we found that:" << "\n  PubSub service:" << is_pubsub_service
               << "\n  PubSub PEP:" << is_pubsub_pep << "\n  single-user chat:" << is_chat
               << "\n  multi-user chat:" << is_chatroom;

      if (is_pubsub_service || is_pubsub_pep || is_chatroom || is_chat) {
        if (is_pubsub_service) {
          fetchPubSubSubscriptions(jid, XmppFeed::Type::PubSubServiceNode, new_tree);
        }
        else if (is_pubsub_pep) {
          fetchPubSubSubscriptions(jid, XmppFeed::Type::PubSubPep, new_tree);
        }

        if (is_chatroom) {
          fetchChatroom(jid, *info, XmppFeed::Type::MultiUserChatRoom, new_tree);
        }
        else if (is_chat) {
          fetchChatroom(jid, *info, XmppFeed::Type::SingleUserChat, new_tree);
        }

        return;
      }
    }
    else if (QXmppError* error = std::get_if<QXmppError>(&result)) {
      QString desc = XmppSimpleError::fromQXmppError(*error).m_description;

      qWarningNN << LOGSEC_XMPP << "Service checking failed:" << NONQUOTE_W_SPACE_DOT(desc);
    }

    finalizeSyncInFinish(jid, new_tree);
  });
}

void XmppNetwork::fetchPubSubSubscriptions(const QString& service, XmppFeed::Type pubsub_type, RootItem* new_tree) {
  auto task = m_pubSubManager->requestSubscriptions(service);

  task.then(this, [=, this](auto result) {
    m_syncInTimer.start();

    if (auto subs = std::get_if<QList<QXmppPubSubSubscription>>(&result)) {
      if (!subs->isEmpty()) {
        if (m_syncInSent) {
          return;
        }

        QList<RootItem*> childs;

        for (const QXmppPubSubSubscription& sub : *subs) {
          qDebugNN << LOGSEC_XMPP << "Detected subscription" << QUOTE_W_SPACE(sub.node()) << "from service"
                   << QUOTE_W_SPACE_DOT(service);

          if (sub.node().contains(QSL("urn:xmpp:microblog:0")) && pubsub_type != XmppFeed::Type::PubSubPep) {
            qWarningNN << LOGSEC_XMPP << "Microblogs (PEPs) are only supported from individual user accounts. Found in"
                       << QUOTE_W_SPACE_DOT(service);
            continue;
          }

          XmppFeed* feed = new XmppFeed();

          feed->setType(pubsub_type);
          feed->setTitle(sub.node());
          feed->setCustomId(sub.node());
          feed->setIcon(IconFactory::fromColor(TextFactory::generateColorFromText(sub.node()),
                                               sub.node().trimmed()[0]));

          childs.append(feed);
        }

        if (!childs.isEmpty()) {
          Category* service_folder = XmppServiceRoot::findCategory(new_tree, service);
          bool is_service_new = false;

          if (service_folder == nullptr) {
            service_folder = new Category();

            service_folder->setIcon(IconFactory::fromColor(TextFactory::generateColorFromText(service),
                                                           service.trimmed()[0]));
            service_folder->setCustomId(service);
            service_folder->setTitle(service);

            is_service_new = true;
          }

          for (RootItem* ch : childs) {
            service_folder->appendChild(ch);
          }

          if (is_service_new) {
            new_tree->appendChild(service_folder);
          }
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

    finalizeSyncInFinish(service, new_tree);
  });
}

void XmppNetwork::fetchChatroom(const QString& chatroom,
                                const QXmppDiscoInfo& info,
                                XmppFeed::Type chatroom_type,
                                RootItem* new_tree) {
  auto chat_domain = QXmppUtils::jidToDomain(chatroom);
  RootItem* service_folder = new_tree->getItemFromSubTree([=](const RootItem* child) {
    return child->kind() == RootItem::Kind::Category && child->title() == chat_domain;
  });

  if (service_folder == nullptr) {
    if (m_syncInSent) {
      return;
    }

    service_folder = new Category();
    service_folder->setIcon(IconFactory::fromColor(TextFactory::generateColorFromText(chat_domain),
                                                   chat_domain.trimmed()[0]));
    service_folder->setCustomId(chat_domain);
    service_folder->setTitle(chat_domain);

    new_tree->appendChild(service_folder);
  }

  XmppFeed* feed = new XmppFeed();
  auto extracted_title = qlinq::from(info.identities()).firstOrDefault([](const QXmppDiscoIdentity& identity) {
    return identity.category() == QSL("conference") && identity.type() == QSL("text");
  });

  feed->setType(chatroom_type);
  feed->setTitle(extracted_title.has_value() ? extracted_title->name() : chatroom);
  feed->setCustomId(chatroom);
  feed->setIcon(IconFactory::fromColor(TextFactory::generateColorFromText(chatroom), feed->title().trimmed()[0]));

  service_folder->appendChild(feed);

  finalizeSyncInFinish(chatroom, new_tree);
}

QStringList XmppNetwork::defaultExtraServices() {
  return {QSL("news.movim.eu")};
}

QHash<QString, QString> XmppNetwork::xepMappings() {
  QHash<QString, QString> xeps = {{QSL("gc-1.0"), QSL("XEP-0045")},

                                  {QSL("http://jabber.org/protocol/activity"), QSL("XEP-0108")},
                                  {QSL("http://jabber.org/protocol/address"), QSL("XEP-0033")},
                                  {QSL("http://jabber.org/protocol/amp"), QSL("XEP-0079")},
                                  {QSL("http://jabber.org/protocol/bytestreams"), QSL("XEP-0065")},
                                  {QSL("http://jabber.org/protocol/bytestreams#udp"), QSL("XEP-0065")},
                                  {QSL("http://jabber.org/protocol/caps"), QSL("XEP-0115")},
                                  {QSL("http://jabber.org/protocol/caps#optimize"), QSL("XEP-0115")},
                                  {QSL("http://jabber.org/protocol/chatstates"), QSL("XEP-0085")},
                                  {QSL("http://jabber.org/protocol/commands"), QSL("XEP-0050")},
                                  {QSL("http://jabber.org/protocol/compress"), QSL("XEP-0138")},
                                  {QSL("http://jabber.org/protocol/disco"), QSL("XEP-0030")},
                                  {QSL("http://jabber.org/protocol/feature-neg"), QSL("XEP-0020")},
                                  {QSL("http://jabber.org/protocol/geoloc"), QSL("XEP-0080")},
                                  {QSL("http://jabber.org/protocol/http-auth"), QSL("XEP-0072")},
                                  {QSL("http://jabber.org/protocol/httpbind"), QSL("XEP-0124")},
                                  {QSL("http://jabber.org/protocol/ibb"), QSL("XEP-0047")},
                                  {QSL("http://jabber.org/protocol/mood"), QSL("XEP-0107")},
                                  {QSL("http://jabber.org/protocol/muc"), QSL("XEP-0045")},
                                  {QSL("http://jabber.org/protocol/offline"), QSL("XEP-0013")},
                                  {QSL("http://jabber.org/protocol/physloc"), QSL("XEP-0080")},
                                  {QSL("http://jabber.org/protocol/pubsub"), QSL("XEP-0060")},
                                  {QSL("http://jabber.org/protocol/rsm"), QSL("XEP-0059")},
                                  {QSL("http://jabber.org/protocol/rosterx"), QSL("XEP-0144")},
                                  {QSL("http://jabber.org/protocol/sipub"), QSL("XEP-0137")},
                                  {QSL("http://jabber.org/protocol/soap"), QSL("XEP-0072")},
                                  {QSL("http://jabber.org/protocol/stats"), QSL("XEP-0039")},
                                  {QSL("http://jabber.org/protocol/soap"), QSL("XEP-0072")},
                                  {QSL("http://jabber.org/protocol/waitinglist"), QSL("XEP-0130")},
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

                                  {QSL("muc_"), QSL("XEP-0045")},

                                  {QSL("roster:delimiter"), QSL("XEP-0083")},

                                  {QSL("urn:ietf:rfc:3264"), QSL("XEP-0176")},

                                  {QSL("urn:xmpp:archive"), QSL("XEP-0136")},

                                  {QSL("urn:xmpp:avatar"), QSL("XEP-0084")},

                                  {QSL("urn:xmpp:delay"), QSL("XEP-0203")},

                                  {QSL("urn:xmpp:jingle:apps:rtp"), QSL("XEP-0167")},

                                  {QSL("urn:xmpp:ping"), QSL("XEP-0199")},
                                  {QSL("urn:xmpp:receipts"), QSL("XEP-0184")},
                                  {QSL("urn:xmpp:sid:0"), QSL("XEP-0359")},
                                  {QSL("urn:xmpp:ssn"), QSL("XEP-0155")},
                                  {QSL("urn:xmpp:time"), QSL("XEP-0202")},

                                  {QSL("vcard-temp"), QSL("XEP-0054")},

                                  {QSL("urn:xmpp:caps"), QSL("XEP-0390")},

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

  if (m_root != nullptr) {
    if (m_root->getSubTreeFeeds().isEmpty()) {
      m_root->requestSyncIn();
      m_root->requestItemExpand({m_root}, true);
    }
    else {
      joinRooms();
    }
  }

  static auto xep_map = xepMappings();

  m_discoveryManager->info(m_xmppClient->configuration().domain()).then(this, [this](auto result) {
    if (QXmppDiscoInfo* info = std::get_if<QXmppDiscoInfo>(&result)) {
      auto f = info->features();

      m_xeps = qlinq::from(f)
                 .select([](const QString& protocol) -> QString {
                   QString xep = xep_map.value(protocol);
                   return xep;
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
                                  tr("XMPP connection to server %1 has error: %2")
                                    .arg(m_domain, XmppSimpleError::fromQXmppError(error).m_description),
                                  QSystemTrayIcon::MessageIcon::Critical));
}

void XmppNetwork::onMessageReceived(const QXmppMessage& message) {
  // PubSub messages are processed in PubSub handler.
  // Multi-user chats are processed separately.
  // So we deal only with single-user chats here.
  qDebugNN << LOGSEC_XMPP << "Message received:" << "\n  Type: " << message.type() << "\n  From: " << message.from()
           << "\n  To: " << message.to() << "\n  Body: " << message.body();

  if (message.type() != QXmppMessage::Type::Chat || m_root == nullptr) {
    return;
  }

  Message article = XmppFeed::articleFromXmppMessage(message);

  if (article.m_contents.isEmpty() || article.m_title.isEmpty()) {
    return;
  }

  auto* feed = XmppServiceRoot::findFeed(m_root, message.from(), XmppFeed::Type::SingleUserChat);

  if (feed != nullptr) {
    m_root->onRealTimeArticleObtained({}, {}, article, feed);
  }
  else {
    // TODO: warning
  }
}

void XmppNetwork::joinRooms() {
  auto rooms = m_root->getSubTree<XmppFeed>([](const XmppFeed* fd) {
    return fd->type() == XmppFeed::Type::MultiUserChatRoom;
  });

  for (XmppFeed* room : rooms) {
    room->join(m_mucManager);
  }
}

XmppServiceRoot* XmppNetwork::root() const {
  return m_root;
}

QStringList XmppNetwork::extraNodes() const {
  return m_extraNodes;
}

void XmppNetwork::setExtraNodes(const QStringList& nodes) {
  m_extraNodes = nodes;
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
