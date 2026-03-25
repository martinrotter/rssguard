// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/xmppubsubpmanager.h"

#include "src/parsers/atomparser.h"

#include <librssguard/definitions/definitions.h>

#include <QXmppClient.h>
#include <QXmppConstants_p.h>
#include <QXmppDiscoveryManager.h>
#include <QXmppLogger.h>
#include <QXmppPubSubEvent.h>
#include <QXmppPubSubSubscription.h>
#include <QXmppTask.h>

Message AtomPubSubBaseItem::message() const {
  return m_message;
}

void AtomPubSubBaseItem::parsePayload(const QDomElement& element) {
  QString xml;
  QTextStream stream(&xml);

  element.save(stream, 0);

  AtomParser parser(xml);
  auto messages = parser.messages();

  if (messages.isEmpty()) {
    m_message = Message();
  }
  else {
    m_message = messages.first();
    m_message.m_customId = id();
  }
}

void AtomPubSubBaseItem::serializePayload(QXmlStreamWriter* writer) const {}

PubSubManager::PubSubManager(QObject* parent) : QXmppPubSubManager(), QXmppPubSubEventHandler() {
  setParent(parent);
}

bool PubSubManager::handlePubSubEvent(const QDomElement& element, const QString& service, const QString& node) {
  if (QXmppPubSubEvent<QXmppPubSubBaseItem>::isPubSubEvent(element)) {
    QXmppPubSubEvent<QXmppPubSubBaseItem> event;
    event.parse(element);

    for (const QXmppPubSubBaseItem& item : event.items()) {
      QString eid = item.id();
      QXmppTask<QXmppPubSubManager::ItemResult<AtomPubSubBaseItem>> task =
        requestItem<AtomPubSubBaseItem>(service, node, eid);

      task.then(this, [service, node, this](auto result) {
        if (AtomPubSubBaseItem* item = std::get_if<AtomPubSubBaseItem>(&result)) {
          emit pushArticleObtained(service, node, item->message());
        }
        else if (QXmppError* err = std::get_if<QXmppError>(&result)) {
          qDebugNN << LOGSEC_XMPP << "Failed to fetch item:" << QUOTE_W_SPACE_DOT(err->description);
        }
      });
    }
  }

  /*
  QString xml;
  QTextStream stream(&xml);
  element.save(stream, 2); // indent = 2 spaces

  qDebug().noquote() << xml;
  */

  return true;
}
/*
PubSubExplorer::PubSubExplorer(QXmppClient* client)
  : QObject(client), m_client(client), m_disco(client->findExtension<QXmppDiscoveryManager>()),
    m_pubsub(client->findExtension<QXmppPubSubManager>()) {}

void PubSubExplorer::start() {
  qDebug() << "Connected. Discovering services…";
  qDebug() << "JID for services:" << m_client->configuration().jid();

  auto task = m_disco->items(m_client->configuration().domain());

  task.then(this, [this](auto result) {
    if (auto items = std::get_if<QList<QXmppDiscoveryIq::Item>>(&result)) {
      for (const auto& item : *items) {
        checkService(item.jid());
      }
    }
    else {
      qDebug() << "Service discovery failed";
    }
  });
}

void PubSubExplorer::checkService(const QString& jid) {
  auto task = m_disco->info(jid);

  task.then(this, [this, jid](auto result) {
    if (auto info = std::get_if<QXmppDiscoInfo>(&result)) {
      if (info->features().contains("http://jabber.org/protocol/pubsub")) {
        discoverNodes(jid);
        fetchSubscriptions(jid);
      }
    }
  });
}

void PubSubExplorer::discoverNodes(const QString& service) {
  auto task = m_disco->items(service);

  task.then(this, [service, this](auto result) {
    if (auto items = std::get_if<QList<QXmppDiscoveryIq::Item>>(&result)) {
      qDebug() << "\nPubSub service:" << service;
      qDebug() << "Available nodes:";

      for (const auto& item : *items) {
        qDebug() << "  node:" << item.node() << "name:" << item.name();
      }
    }
    else {
      qDebug() << "Failed discovering nodes";
    }
  });
}

void PubSubExplorer::fetchSubscriptions(const QString& service) {
  auto task = m_pubsub->requestSubscriptions(service);

  task.then(this, [this, service](auto result) {
    if (auto subs = std::get_if<QList<QXmppPubSubSubscription>>(&result)) {
      qDebug() << "\nPubSub service:" << service;
      qDebug() << "Active subscriptions:";

      for (const auto& sub : *subs) {
        qDebug() << "  sub:" << sub.node();

        // fetchItems(service, sub.node());
      }
    }
    else {
      qDebug() << "Failed fetching subscriptions";
    }
  });
}

void PubSubExplorer::fetchItems(const QString& service, const QString& node, const QStringList& itemIds) {
  auto task = m_pubsub->requestItems<AtomPubSubBaseItem>(service, node, itemIds);

  task.then(this, [service, node](auto result) {
    if (auto items = std::get_if<QXmppPubSubManager::Items<AtomPubSubBaseItem>>(&result)) {
      qDebug() << "Items for service/node:" << service << node;

      for (const auto& item : items->items) {
        qDebug() << "  item id:" << item.id();
      }
    }
    else {
      qDebug() << "Failed fetching items for node:" << node;
    }
  });
}
*/
