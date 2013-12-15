#ifndef FEEDSMODELFEED_H
#define FEEDSMODELFEED_H

#include "core/feedsmodelrootitem.h"


// Represents BASE class for feeds contained in FeedsModel.
// NOTE: This class should be derived to create PARTICULAR feed types.
class FeedsModelFeed : public FeedsModelRootItem {
  public:
    // Describes possible types of feeds.
    // NOTE: This is equivalent to attribute Feeds(type).
    enum Type {
      StandardRss   = 0,
      StandardRdf   = 1,
      StandardAtom  = 2
    };

    // Constructors and destructors.
    explicit FeedsModelFeed(FeedsModelRootItem *parent_item = NULL);
    virtual ~FeedsModelFeed();

    // Returns 0, feeds have no children.
    int childCount() const;

    // Getters/setters for count of messages.
    // NOTE: For feeds, counts are stored internally
    // and can be updated from the database.
    int countOfAllMessages() const;
    int countOfUnreadMessages() const;

    // Other getters/setters.
    Type type() const;
    void setType(const Type &type);

    QString title() const;
    void setTitle(const QString &title);

  protected:
    Type m_type;
    QString m_title;
    int m_totalCount;
    int m_unreadCount;
};

#endif // FEEDSMODELFEED_H
