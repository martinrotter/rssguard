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

    QString description() const;
    void setDescription(const QString &description);

    QDateTime creationDate() const;
    void setCreationDate(const QDateTime &creation_date);

    static FeedsModelStandardCategory *loadFromRecord(const QSqlRecord &record);

  private:
    QDateTime m_creationDate;
    QString m_description;

};

#endif // FEEDSMODELSTANDARDCATEGORY_H
