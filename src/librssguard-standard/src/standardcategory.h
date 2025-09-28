// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDSMODELCATEGORY_H
#define FEEDSMODELCATEGORY_H

#include <librssguard/services/abstract/category.h>

class StandardServiceRoot;

class StandardCategory : public Category {
    Q_OBJECT

  public:
    explicit StandardCategory(RootItem* parent_item = nullptr);
    explicit StandardCategory(const StandardCategory& other) = default;

    StandardServiceRoot* serviceRoot() const;

    virtual Qt::ItemFlags additionalFlags() const;
    virtual bool performDragDropChange(RootItem* target_item);
    virtual bool canBeEdited() const;
    virtual bool canBeDeleted() const;
    virtual bool deleteItem();

  private:
    bool removeItself();
};

#endif // FEEDSMODELCLASSICCATEGORY_H
