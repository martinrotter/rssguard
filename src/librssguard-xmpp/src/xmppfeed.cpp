// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/xmppfeed.h"

#include "src/definitions.h"
#include "src/xmppnetwork.h"
#include "src/xmppserviceroot.h"
#include "src/xmppubsubpmanager.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>

#include <QPointer>

XmppFeed::XmppFeed(RootItem* parent) : Feed(parent) {
  QTimer::singleShot(5000, this, &XmppFeed::obtainArticles);
}

XmppServiceRoot* XmppFeed::serviceRoot() const {
  return qobject_cast<XmppServiceRoot*>(account());
}

bool XmppFeed::canBeDeleted() const {
  return true;
}

void XmppFeed::deleteItem() {
  /*
  serviceRoot()->network()->subscriptionEdit(QSL(XMPP_API_EDIT_SUBSCRIPTION_DELETE),
                                             customId(),
                                             {},
                                             {},
                                             {},
                                             serviceRoot()->networkProxy());
*/
  removeItself();
  serviceRoot()->requestItemRemoval(this, false);
}

void XmppFeed::obtainArticles() {
  QStringList existing_article_ids;

  qApp->database()->worker()->read([&](const QSqlDatabase& db) {
    existing_article_ids << DatabaseQueries::bagOfMessages(db, ServiceRoot::BagOfMessages::Read, this);
    existing_article_ids << DatabaseQueries::bagOfMessages(db, ServiceRoot::BagOfMessages::Unread, this);
  });

  auto service = serviceName();

  if (service.isEmpty()) {
    return;
  }

  serviceRoot()->network()->pubSubManager()->requestItemIds(service, customId()).then(this, [=, this](auto result) {
    if (auto items = std::get_if<QVector<QString>>(&result)) {
      // Determine IDs we do not have yet.
      QSet<QString> set_existing(existing_article_ids.cbegin(), existing_article_ids.cend());
      QSet<QString> set_new(items->cbegin(), items->cend());
      QSet<QString> set_to_download = set_new.subtract(set_existing);

      if (set_to_download.isEmpty()) {
        return;
      }

      QStringList list_do_download(set_to_download.cbegin(), set_to_download.cend());

      serviceRoot()
        ->network()
        ->pubSubManager()
        ->requestItems<AtomPubSubBaseItem>(service, customId(), list_do_download.mid(0, 10))
        .then(this, [=, this](auto items_result) {
          if (auto items = std::get_if<QXmppPubSubManager::Items<AtomPubSubBaseItem>>(&items_result)) {
            for (const auto& item : items->items) {
              Message msg = item.message();

              if (msg.m_customId.isEmpty()) {
                continue;
              }

              m_articles.append(msg);
            }
          };
        });
    }
    else if (QXmppError* error = std::get_if<QXmppError>(&result)) {
      QString desc = XmppSimpleError::fromQXmppError(*error).m_description;

      qDebugNN << LOGSEC_XMPP << "Getting entries IDs failed:" << NONQUOTE_W_SPACE_DOT(desc);
    }
    else {
      qDebugNN << LOGSEC_XMPP << "Getting entries IDs failed with unspecified error.";
    }
  });
}

void XmppFeed::storeRealTimeArticle(const Message& message) {
  m_articles.append(message);
}

void XmppFeed::removeItself() {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::deleteFeed(db, this, serviceRoot()->accountId());
  });
}

void XmppFeed::setArticles(const QList<Message>& articles) {
  m_articles = articles;
}

QString XmppFeed::serviceName() const {
  return parent() == nullptr ? QString() : parent()->customId();
}

QList<Message> XmppFeed::articles() const {
  return m_articles;
}
