// For license of this file, see <object-root-folder>/LICENSE.md.

#include "services/inoreader/inoreaderfeed.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/inoreader/inoreaderserviceroot.h"
#include "services/inoreader/network/inoreadernetworkfactory.h"

InoreaderFeed::InoreaderFeed(RootItem* parent) : Feed(parent) {}

InoreaderFeed::InoreaderFeed(const QSqlRecord& record) : Feed(record) {}

InoreaderServiceRoot* InoreaderFeed::serviceRoot() const {
  return qobject_cast<InoreaderServiceRoot*>(getParentServiceRoot());
}

QList<Message> InoreaderFeed::obtainNewMessages(bool* error_during_obtaining) {
  Feed::Status error;
  QList<Message> messages = serviceRoot()->network()->messages(customId(), error);

  setStatus(error);

  if (error == Feed::Status::NetworkError || error == Feed::Status::AuthError) {
    *error_during_obtaining = true;
  }

  return messages;
}
