#ifndef FEEDSMODELRECYCLEBIN_H
#define FEEDSMODELRECYCLEBIN_H

#include "core/feedsmodelrootitem.h"

#include <QCoreApplication>


class FeedsModelRecycleBin : public FeedsModelRootItem {
    Q_DECLARE_TR_FUNCTIONS(FeedsModelRecycleBin)

  public:
    explicit FeedsModelRecycleBin(FeedsModelRootItem *parent = NULL);
    virtual ~FeedsModelRecycleBin();

    int childCount() const;
    void appendChild(FeedsModelRootItem *child);
    int countOfUnreadMessages() const;
    int countOfAllMessages() const;
    QVariant data(int column, int role) const;
};

#endif // FEEDSMODELRECYCLEBIN_H
