// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/ttrssfeed.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "services/tt-rss/ttrssserviceroot.h"

#include <QPointer>

TtRssFeed::TtRssFeed(RootItem* parent) : Feed(parent) {}

TtRssFeed::TtRssFeed(const QSqlRecord& record) : Feed(record) {}

TtRssFeed::~TtRssFeed() = default;

TtRssServiceRoot* TtRssFeed::serviceRoot() const {
  return qobject_cast<TtRssServiceRoot*>(getParentServiceRoot());
}

bool TtRssFeed::canBeDeleted() const {
  return true;
}

bool TtRssFeed::deleteViaGui() {
  TtRssUnsubscribeFeedResponse response = serviceRoot()->network()->unsubscribeFeed(customNumericId(),
                                                                                    getParentServiceRoot()->networkProxy());

  if (response.code() == UFF_OK && removeItself()) {
    serviceRoot()->requestItemRemoval(this);
    return true;
  }
  else {
    qWarningNN << LOGSEC_TTRSS
               << "Unsubscribing from feed failed, received JSON:"
               << QUOTE_W_SPACE_DOT(response.toString());
    return false;
  }
}

QList<Message> TtRssFeed::obtainNewMessages(bool* error_during_obtaining) {
  QList<Message> messages;
  int newly_added_messages = 0;
  int limit = TTRSS_MAX_MESSAGES;
  int skip = 0;

  do {
    TtRssGetHeadlinesResponse headlines = serviceRoot()->network()->getHeadlines(customId().toInt(), limit, skip,
                                                                                 true, true, false,
                                                                                 serviceRoot()->network()->downloadOnlyUnreadMessages(),
                                                                                 getParentServiceRoot()->networkProxy());

    if (serviceRoot()->network()->lastError() != QNetworkReply::NoError) {
      setStatus(Feed::Status::NetworkError);
      *error_during_obtaining = true;
      serviceRoot()->itemChanged(QList<RootItem*>() << this);
      return QList<Message>();
    }
    else {
      QList<Message> new_messages = headlines.messages(getParentServiceRoot());

      messages.append(new_messages);
      newly_added_messages = new_messages.size();
      skip += newly_added_messages;
    }
  }
  while (newly_added_messages > 0);

  *error_during_obtaining = false;
  return messages;
}

bool TtRssFeed::removeItself() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  return DatabaseQueries::deleteFeed(database, customId().toInt(), serviceRoot()->accountId());
}
