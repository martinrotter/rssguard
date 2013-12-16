#ifndef FEEDSMODELCLASSICCATEGORY_H
#define FEEDSMODELCLASSICCATEGORY_H

#include <QDateTime>

#include "core/feedsmodelrootitem.h"


class FeedsModelFeed;

// Base class for all categories contained in FeedsModel.
// NOTE: This class should be derived to create PARTICULAR category types.
// NOTE: This class should not be instantiated directly.
class FeedsModelCategory : public FeedsModelRootItem {
  public:
    // Describes possible types of categories.
    // NOTE: This is equivavelnt to Categories(type).
    enum Type {
      Standard    = 0,
      Feedly      = 1,
      TinyTinyRss = 2
    };

    // Constructors and destructors
    explicit FeedsModelCategory(FeedsModelRootItem *parent_item = NULL);
    virtual ~FeedsModelCategory();

    // TODO: Otestovat metodu feeds.

    // Returns list of ALL feeds situated under this category.
    // NOTE: This is recursive.
    virtual QList<FeedsModelFeed*> feeds();

    // Counts of messages.
    // NOTE: Counts of messages in categories include
    // counts of messages from all children.
    int countOfAllMessages() const;
    int countOfUnreadMessages() const;

    // All types of categories offer these getters/setters.
    Type type() const;
    void setType(const Type &type);

    QString description() const;
    void setDescription(const QString &description);

    QDateTime creationDate() const;
    void setCreationDate(const QDateTime &creation_date);

  protected:
    Type m_type;
    QDateTime m_creationDate;
    QString m_description;
};

#endif // FEEDSMODELCLASSICCATEGORY_H
