// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDSMODELCATEGORY_H
#define FEEDSMODELCATEGORY_H

#include "services/abstract/category.h"

#include <QCoreApplication>
#include <QSqlRecord>

class FeedsModel;
class StandardServiceRoot;

// Base class for all categories contained in FeedsModel.
// NOTE: This class should be derived to create PARTICULAR category types.
// NOTE: This class should not be instantiated directly.
class StandardCategory : public Category {
  Q_OBJECT

  public:
    explicit StandardCategory(RootItem* parent_item = nullptr);
    explicit StandardCategory(const StandardCategory& other) = default;
    explicit StandardCategory(const QSqlRecord& record);
    virtual ~StandardCategory() = default;

    StandardServiceRoot* serviceRoot() const;

    // Returns the actual data representation of standard category.
    Qt::ItemFlags additionalFlags() const;
    bool performDragDropChange(RootItem* target_item);

    bool canBeEdited() const;
    bool canBeDeleted() const;

    bool editViaGui();
    bool deleteViaGui();

    bool addItself(RootItem* parent);
    bool editItself(StandardCategory* new_category_data);
    bool removeItself();
};

#endif // FEEDSMODELCLASSICCATEGORY_H
