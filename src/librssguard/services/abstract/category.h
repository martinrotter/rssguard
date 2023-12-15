// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef CATEGORY_H
#define CATEGORY_H

#include "services/abstract/rootitem.h"

class Category : public RootItem {
    Q_OBJECT

  public:
    explicit Category(RootItem* parent = nullptr);
    explicit Category(const Category& other);

    virtual void updateCounts(bool including_total_count);
    virtual bool cleanMessages(bool clean_read_only);
    virtual bool markAsReadUnread(ReadStatus status);
    virtual QString additionalTooltip() const;
};

#endif // CATEGORY_H
