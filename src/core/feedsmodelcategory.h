#ifndef FEEDSMODELCLASSICCATEGORY_H
#define FEEDSMODELCLASSICCATEGORY_H

#include "core/feedsmodelrootitem.h"


// Base class for all categories contained in FeedsModel.
// NOTE: This class should be derived to create PARTICULAR category types.
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

    int countOfAllMessages() const;
    int countOfUnreadMessages() const;

    Type type() const;
    void setType(const Type &type);

  protected:
    Type m_type;

};

#endif // FEEDSMODELCLASSICCATEGORY_H
