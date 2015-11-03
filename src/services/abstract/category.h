#ifndef CATEGORY_H
#define CATEGORY_H

#include "core/rootitem.h"


class Category : public RootItem {
  public:
    explicit Category(RootItem *parent = NULL);
    virtual ~Category();
};

#endif // CATEGORY_H
