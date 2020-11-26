// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef OWNCLOUDFEED_H
#define OWNCLOUDFEED_H

#include "services/abstract/feed.h"

class OwnCloudServiceRoot;

class OwnCloudFeed : public Feed {
  Q_OBJECT

  public:
    explicit OwnCloudFeed(RootItem* parent = nullptr);
    explicit OwnCloudFeed(const QSqlRecord& record);
    virtual ~OwnCloudFeed();

    bool canBeDeleted() const;
    bool deleteViaGui();

    bool removeItself();

    OwnCloudServiceRoot* serviceRoot() const;
    QList<Message> obtainNewMessages(bool* error_during_obtaining);
};

#endif // OWNCLOUDFEED_H
