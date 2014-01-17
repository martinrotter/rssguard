#ifndef FEEDSMODELCATEGORY_H
#define FEEDSMODELCATEGORY_H

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
    explicit FeedsModelCategory(const FeedsModelCategory &other);
    virtual ~FeedsModelCategory();

    // All types of categories offer these getters/setters.
    inline Type type() const {
      return m_type;
    }

    inline void setType(const Type &type) {
      m_type = type;
    }

  protected:
    Type m_type;
};

#endif // FEEDSMODELCLASSICCATEGORY_H
