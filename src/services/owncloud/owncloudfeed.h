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

    bool canBeEdited() const;
    bool editViaGui();
    bool canBeDeleted() const;
    bool deleteViaGui();

    bool editItself(OwnCloudFeed* new_feed_data);
    bool removeItself();

    OwnCloudServiceRoot* serviceRoot() const;

  private:
    QList<Message> obtainNewMessages(bool* error_during_obtaining);
};

#endif // OWNCLOUDFEED_H
