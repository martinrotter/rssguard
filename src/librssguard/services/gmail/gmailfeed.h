// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GMAILFEED_H
#define GMAILFEED_H

#include "services/abstract/feed.h"

class GmailServiceRoot;

class GmailFeed : public Feed {
  public:
    explicit GmailFeed(RootItem* parent = nullptr);
    explicit GmailFeed(const QString& title, const QString& custom_id, const QIcon& icon, RootItem* parent = nullptr);
    explicit GmailFeed(const QSqlRecord& record);

    GmailServiceRoot* serviceRoot() const;
    QList<Message> obtainNewMessages(bool* error_during_obtaining);
};

#endif // GMAILFEED_H
