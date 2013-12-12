#ifndef FEEDSMODEL_H
#define FEEDSMODEL_H

#include <QAbstractItemModel>
#include <QIcon>


class FeedsModelRootItem;

class FeedsModel : public QAbstractItemModel {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedsModel(QObject *parent = 0);
    virtual ~FeedsModel();

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;



  private:
    FeedsModelRootItem *m_rootItem;
    QList<QString> m_headerData;
    QIcon m_countsIcon;

};

#endif // FEEDSMODEL_H
