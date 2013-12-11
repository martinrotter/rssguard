#ifndef FEEDSMODEL_H
#define FEEDSMODEL_H

#include <QAbstractItemModel>


class FeedsModelRootItem;

class FeedsModel : public QAbstractItemModel {
    Q_OBJECT

  public:
    explicit FeedsModel(QObject *parent = 0);
    virtual ~FeedsModel();

  signals:

  public slots:


  private:
    FeedsModelRootItem *m_rootItem;

};

#endif // FEEDSMODEL_H
