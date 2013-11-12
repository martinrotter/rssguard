#ifndef FEEDSMODEL_H
#define FEEDSMODEL_H

#include <QAbstractItemModel>


class FeedsModel : public QAbstractItemModel {
    Q_OBJECT

  public:
    explicit FeedsModel(QObject *parent = 0);

  signals:

  public slots:

};

#endif // FEEDSMODEL_H
