// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/xmppfeed.h"

#include "src/definitions.h"
#include "src/xmppnetwork.h"
#include "src/xmppserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>

#include <QPointer>

XmppFeed::XmppFeed(RootItem* parent) : Feed(parent) {}

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

QList<Message> XmppFeed::articles() const {
  return m_articles;
}
