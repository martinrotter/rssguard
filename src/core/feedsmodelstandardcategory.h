#ifndef FEEDSMODELSTANDARDCATEGORY_H
#define FEEDSMODELSTANDARDCATEGORY_H

#include "core/feedsmodelcategory.h"

#include <QSqlRecord>
#include <QDateTime>


// Represents STANDARD category container.
// Standard category container can contain:
//  a) other standard category containers,
//  b) standard feeds,
//  c) other containers and feeds (synchronized ones).
class FeedsModelStandardCategory : public FeedsModelCategory {
  public:
    // Constructors and destructors.
    explicit FeedsModelStandardCategory(FeedsModelRootItem *parent_item = NULL);
    virtual ~FeedsModelStandardCategory();

    // Returns the actual data representation of standard category.
    QVariant data(int column, int role) const;

    // Removes category and all its children from persistent
    // database.
    bool removeItself();

    // Loads particular "standard category" from given sql record.
    static FeedsModelStandardCategory *loadFromRecord(const QSqlRecord &record);
};

#endif // FEEDSMODELSTANDARDCATEGORY_H
