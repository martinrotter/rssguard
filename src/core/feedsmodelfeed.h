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
      StandardRss0X = 0,
      StandardRss1X = 1,
      StandardRss2X = 2,
      StandardRdf   = 3,
      StandardAtom  = 4
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
    void setCountOfAllMessages(int count);

    int countOfUnreadMessages() const;
    void setCountOfUnreadMessages(int count);

    // Other getters/setters.
    Type type() const;
    void setType(const Type &type);

    static QString typeToString(Type type);

  public slots:
    // Updates counts of all/unread messages for this feed.
    void updateCounts(bool including_total_count = true);

  protected:
    Type m_type;
    int m_totalCount;
    int m_unreadCount;
};

#endif // FEEDSMODELFEED_H
