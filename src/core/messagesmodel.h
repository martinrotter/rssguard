#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include <QSqlTableModel>
#include <QStringList>


class MessagesModel : public QSqlTableModel {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesModel(QObject *parent = 0);
    virtual ~MessagesModel();

    // Model implementation.
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

  private:
    void setupHeaderData();

  signals:

  public slots:

  private:
    QStringList m_headerData;

};

#endif // MESSAGESMODEL_H
