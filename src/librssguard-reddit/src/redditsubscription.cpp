// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/redditsubscription.h"

#include "definitions/definitions.h"
#include "src/redditserviceroot.h"

RedditSubscription::RedditSubscription(RootItem* parent) : Feed(parent), m_prefixedName(QString()) {}

RedditServiceRoot* RedditSubscription::serviceRoot() const {
  return qobject_cast<RedditServiceRoot*>(account());
}

QString RedditSubscription::prefixedName() const {
  return m_prefixedName;
}

void RedditSubscription::setPrefixedName(const QString& prefixed_name) {
  m_prefixedName = prefixed_name;
}

QVariantHash RedditSubscription::customDatabaseData() const {
  QVariantHash data;

  data.insert(QSL("prefixed_name"), prefixedName());

  return data;
}

void RedditSubscription::setCustomDatabaseData(const QVariantHash& data) {
  setPrefixedName(data.value(QSL("prefixed_name")).toString());
}
