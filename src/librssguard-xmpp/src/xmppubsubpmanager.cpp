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
          if (!item->message().m_customId.isEmpty()) {
            emit realTimeArticleObtained(service, node, item->message());
          }
          else {
            qWarningNN << LOGSEC_XMPP << "Passed PubSub item was not properly parsed as an ATOM entry.";
          }
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
