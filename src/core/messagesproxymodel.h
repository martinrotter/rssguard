#ifndef MESSAGESPROXYMODEL_H
#define MESSAGESPROXYMODEL_H

#include <QSortFilterProxyModel>


class MessagesProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

  public:
    explicit MessagesProxyModel(QObject *parent = 0);

  signals:

  public slots:

};

#endif // MESSAGESPROXYMODEL_H
