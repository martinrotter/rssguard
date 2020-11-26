// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TTRSSFEED_H
#define TTRSSFEED_H

#include "services/abstract/feed.h"

#include <QSqlRecord>

class TtRssServiceRoot;

class TtRssFeed : public Feed {
  Q_OBJECT

  public:
    explicit TtRssFeed(RootItem* parent = nullptr);
    explicit TtRssFeed(const QSqlRecord& record);
    virtual ~TtRssFeed();

    TtRssServiceRoot* serviceRoot() const;

    bool canBeDeleted() const;
    bool deleteViaGui();

    bool removeItself();

    QList<Message> obtainNewMessages(bool* error_during_obtaining);
};

#endif // TTRSSFEED_H
