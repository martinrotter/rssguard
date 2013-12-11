#ifndef FEEDSPROXYMODEL_H
#define FEEDSPROXYMODEL_H

#include <QSortFilterProxyModel>


class FeedsModel;

class FeedsProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedsProxyModel(QObject *parent = 0);
    virtual ~FeedsProxyModel();

    FeedsModel *sourceModel();

  protected:
    // Compares two rows of data.
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

  private:
    // Source model pointer.
    FeedsModel *m_sourceModel;
};

#endif // FEEDSPROXYMODEL_H
