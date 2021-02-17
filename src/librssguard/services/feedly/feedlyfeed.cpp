// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/feedly/feedlyfeed.h"

#include "exceptions/applicationexception.h"
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
  try {
    QList<Message> messages = serviceRoot()->network()->streamContents(customId());

    setStatus(Feed::Status::Normal);
    *error_during_obtaining = false;
    return messages;;
  }
  catch (const ApplicationException& ex) {
    setStatus(Feed::Status::NetworkError);
    *error_during_obtaining = true;

    qCriticalNN << LOGSEC_FEEDLY
                << "Problem"
                << QUOTE_W_SPACE(ex.message())
                << "when obtaining messages for feed"
                << QUOTE_W_SPACE_DOT(customId());

    return {};
  }
}
