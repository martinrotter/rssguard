#ifndef FEEDSMODELFEED_H
#define FEEDSMODELFEED_H

#include "core/feedsmodelrootitem.h"

#include <QMetaType>


// Represents BASE class for feeds contained in FeedsModel.
// NOTE: This class should be derived to create PARTICULAR feed types.
class FeedsModelFeed : public FeedsModelRootItem {
  public:
    // Describes possible types of feeds.
    // NOTE: This is equivalent to attribute Feeds(type).
    enum Type {
      StandardRss0X = 0,
      StandardRss2X = 1,
      StandardRdf   = 2,
      StandardAtom10  = 3
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

    // Each feed can be "updated".
    // NOTE: This method is used in the "update worker".
    // For example, it can fetch new messages from a remote destination
    // and store them in a local database and so on.
    virtual void update() {
    }

    // Other getters/setters.
    inline Type type() const {
      return m_type;
    }

    inline void setType(const Type &type) {
      m_type = type;
    }

    inline bool passwordProtected() const {
      return m_passwordProtected;
    }

    inline void setPasswordProtected(bool passwordProtected) {
      m_passwordProtected = passwordProtected;
    }

    inline QString username() const {
      return m_username;
    }

    inline void setUsername(const QString &username) {
      m_username = username;
    }

    inline QString password() const {
      return m_password;
    }

    inline void setPassword(const QString &password) {
      m_password = password;
    }

    // Converts particular feed type to string.
    static QString typeToString(Type type);

  public slots:
    // Updates counts of all/unread messages for this feed.
    void updateCounts(bool including_total_count = true);

  protected:
    bool m_passwordProtected;
    QString m_username;
    QString m_password;

    Type m_type;
    int m_totalCount;
    int m_unreadCount;
};

Q_DECLARE_METATYPE(FeedsModelFeed::Type)

#endif // FEEDSMODELFEED_H
