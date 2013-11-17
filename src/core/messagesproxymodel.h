#ifndef MESSAGESPROXYMODEL_H
#define MESSAGESPROXYMODEL_H

#include <QSortFilterProxyModel>


class MessagesModel;

class MessagesProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesProxyModel(QObject *parent = 0);
    virtual ~MessagesProxyModel();

    // Source model getter.
    MessagesModel *sourceModel();

  protected:
    // Compares two rows of data.
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

  private:
    // Source model pointer.
    MessagesModel *m_sourceModel;
};

#endif // MESSAGESPROXYMODEL_H
