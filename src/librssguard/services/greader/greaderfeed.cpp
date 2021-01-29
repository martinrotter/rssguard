// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/greader/greaderfeed.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/greader/greadernetwork.h"
#include "services/greader/greaderserviceroot.h"

GreaderFeed::GreaderFeed(RootItem* parent) : Feed(parent) {}

GreaderFeed::GreaderFeed(const QSqlRecord& record) : Feed(record) {}

GreaderServiceRoot* GreaderFeed::serviceRoot() const {
  return qobject_cast<GreaderServiceRoot*>(getParentServiceRoot());
}

QList<Message> GreaderFeed::obtainNewMessages(bool* error_during_obtaining) {
  Feed::Status error = Feed::Status::Normal;
  QList<Message> messages = serviceRoot()->network()->streamContents(getParentServiceRoot(),
                                                                     customId(),
                                                                     error,
                                                                     getParentServiceRoot()->networkProxy());

  setStatus(error);

  if (error == Feed::Status::NetworkError || error == Feed::Status::AuthError) {
    *error_during_obtaining = true;
  }

  return messages;
}
