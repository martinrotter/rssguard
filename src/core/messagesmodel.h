#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include <QSqlTableModel>


class MessagesModel : public QSqlTableModel {
    Q_OBJECT

  public:
    explicit MessagesModel(QObject *parent = 0);

  signals:

  public slots:

};

#endif // MESSAGESMODEL_H
