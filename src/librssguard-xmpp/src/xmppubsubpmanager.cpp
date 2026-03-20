// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/xmppubsubpmanager.h"

#include <QXmppClient.h>
#include <QXmppConstants_p.h>
#include <QXmppDiscoveryManager.h>
#include <QXmppLogger.h>
#include <QXmppPubSubEvent.h>
#include <QXmppPubSubSubscription.h>
#include <QXmppTask.h>

void AtomPubSubBaseItem::writeDomNode(QXmlStreamWriter* writer, const QDomNode& node) {
  if (node.isElement()) {
    writeDomElement(writer, node.toElement());
  }
  else if (node.isText()) {
    writer->writeCharacters(node.nodeValue());
  }
  else if (node.isCDATASection()) {
    writer->writeCDATA(node.nodeValue());
  }
}

void AtomPubSubBaseItem::writeDomElement(QXmlStreamWriter* writer, const QDomElement& element) {
  if (element.isNull()) {
    return;
  }

  writer->writeStartElement(element.tagName());

  // attributes
  auto attrs = element.attributes();
  for (int i = 0; i < attrs.count(); ++i) {
    QDomAttr attr = attrs.item(i).toAttr();
    writer->writeAttribute(attr.name(), attr.value());
  }

  // children
  for (QDomNode child = element.firstChild(); !child.isNull(); child = child.nextSibling()) {
    writeDomNode(writer, child);
  }

  writer->writeEndElement();
}

void AtomPubSubBaseItem::parsePayload(const QDomElement& payloadElement) {
  payload = payloadElement;

  QString xml;
  QTextStream stream(&xml);

  payloadElement.save(stream, 2); // 2 = indentation spaces

  // Atom namespace entries
  QDomElement e;

  e = payloadElement.firstChildElement("id");
  if (!e.isNull()) {
    aid = e.text();
  }

  e = payloadElement.firstChildElement("title");
  if (!e.isNull()) {
    title = e.text();
  }

  e = payloadElement.firstChildElement("content");
  if (!e.isNull()) {
    content = e.text();
  }

  QDomElement authorElem = payloadElement.firstChildElement("author");
  if (!authorElem.isNull()) {
    QDomElement nameElem = authorElem.firstChildElement("name");
    if (!nameElem.isNull()) {
      author = nameElem.text();
    }
  }
}

void AtomPubSubBaseItem::serializePayload(QXmlStreamWriter* writer) const {
  if (payload.isNull()) {
    return;
  }

  // write the stored XML payload
  writeDomElement(writer, payload);
}

PubSubManager::PubSubManager(QObject* parent) : QXmppPubSubManager(), QXmppPubSubEventHandler() {
  setParent(parent);
}

bool PubSubManager::handlePubSubEvent(const QDomElement& element,
                                      const QString& pubSubService,
                                      const QString& nodeName) {
  qDebug() << "\nPUBSUB EVENT";
  qDebug() << "service:" << pubSubService;
  qDebug() << "node:" << nodeName;

  if (QXmppPubSubEvent<AtomPubSubBaseItem>::isPubSubEvent(element)) {
    QXmppPubSubEvent<AtomPubSubBaseItem> event;
    event.parse(element);

    qDebug() << "event:" << event.type();

    if (!event.items().isEmpty()) {
      QString eid = event.items().constFirst().id();
      QXmppTask<QXmppPubSubManager::ItemResult<AtomPubSubBaseItem>> task =
        requestItem<AtomPubSubBaseItem>(pubSubService, nodeName, eid);

      task.then(this, [pubSubService, nodeName](auto result) {
        if (AtomPubSubBaseItem* item = std::get_if<AtomPubSubBaseItem>(&result)) {
          qDebug() << "Got item:" << item->id();
        }
        else if (QXmppError* err = std::get_if<QXmppError>(&result)) {
          qDebug() << "Failed to fetch item:" << err->description;
        }
      });
    }
  }

  QString xml;
  QTextStream stream(&xml);
  element.save(stream, 2); // indent = 2 spaces

  qDebug().noquote() << xml;

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
