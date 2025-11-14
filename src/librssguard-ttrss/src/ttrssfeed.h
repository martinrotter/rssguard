// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TTRSSFEED_H
#define TTRSSFEED_H

#include <librssguard/services/abstract/feed.h>

class TtRssServiceRoot;

class TtRssFeed : public Feed {
    Q_OBJECT

  public:
    explicit TtRssFeed(RootItem* parent = nullptr);

    virtual bool canBeDeleted() const;
    virtual void deleteItem();
    // virtual QList<QAction*> contextMenuFeedsList();

  private:
    TtRssServiceRoot* serviceRoot() const;
    void removeItself();

  private:
    QAction* m_actionShareToPublished;
};

#endif // TTRSSFEED_H
