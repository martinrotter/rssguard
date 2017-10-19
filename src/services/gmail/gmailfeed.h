// For license of this file, see <object-root-folder>/LICENSE.md.

#ifndef GMAILFEED_H
#define GMAILFEED_H

#include "services/abstract/feed.h"

class GmailServiceRoot;

class GmailFeed : public Feed {
  public:
    explicit GmailFeed(RootItem* parent = nullptr);
    explicit GmailFeed(const QSqlRecord& record);

    GmailServiceRoot* serviceRoot() const;

  private:
    QList<Message> obtainNewMessages(bool* error_during_obtaining);
};

#endif // GMAILFEED_H
