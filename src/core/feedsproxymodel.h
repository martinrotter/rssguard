#ifndef FEEDSPROXYMODEL_H
#define FEEDSPROXYMODEL_H

#include <QSortFilterProxyModel>


class FeedsProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

  public:
    explicit FeedsProxyModel(QObject *parent = 0);
    virtual ~FeedsProxyModel();

  signals:

  public slots:

};

#endif // FEEDSPROXYMODEL_H
