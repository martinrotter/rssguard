#ifndef FEEDSMODELSTANDARDCATEGORY_H
#define FEEDSMODELSTANDARDCATEGORY_H

#include <QSqlRecord>
#include <QDateTime>

#include "core/feedsmodelcategory.h"


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

    QVariant data(int column, int role) const;

    // Performs update on all children of this category.
    void update();

    static FeedsModelStandardCategory *loadFromRecord(const QSqlRecord &record);
};

#endif // FEEDSMODELSTANDARDCATEGORY_H
