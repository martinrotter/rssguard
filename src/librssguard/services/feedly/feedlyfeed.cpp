// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/feedly/feedlyfeed.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/feedly/feedlynetwork.h"
#include "services/feedly/feedlyserviceroot.h"

FeedlyFeed::FeedlyFeed(RootItem* parent) : Feed(parent) {}

FeedlyFeed::FeedlyFeed(const QSqlRecord& record) : Feed(record) {}

FeedlyServiceRoot* FeedlyFeed::serviceRoot() const {
  return qobject_cast<FeedlyServiceRoot*>(getParentServiceRoot());
}

QList<Message> FeedlyFeed::obtainNewMessages(bool* error_during_obtaining) {
  return {};

  /*
     Feed::Status error = Feed::Status::Normal;
     QList<Message> messages = serviceRoot()->network()->streamContents(getParentServiceRoot(),
                                                                     customId(),
                                                                     error,
                                                                     getParentServiceRoot()->networkProxy());

     setStatus(error);

     if (error == Feed::Status::NetworkError || error == Feed::Status::AuthError) {
   * error_during_obtaining = true;
     }

     return messages;*/
}
