// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef CATEGORY_H
#define CATEGORY_H

#include "services/abstract/rootitem.h"

class RSSGUARD_DLLSPEC Category : public RootItem {
    Q_OBJECT

  public:
    explicit Category(RootItem* parent = nullptr);
    explicit Category(const Category& other);

    virtual void updateCounts();
    virtual void cleanMessages(bool clean_read_only);
    virtual void markAsReadUnread(ReadStatus status);
    virtual QString additionalTooltip() const;
};

#endif // CATEGORY_H
