#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include <QSqlTableModel>
#include <QFont>
#include <QIcon>
#include <QDateTime>

#include "core/defs.h"


// Represents single message.
// NOTE: This is primarily used for transfering data
// to WebBrowser responsible for displaying of messages.
class Message {
  private:
    QString m_title;
    QString m_url;
    QString m_author;
    QString m_contents;
    QDateTime m_updated;

    friend class WebBrowser;
    friend class MessagesModel;
};


class MessagesModel : public QSqlTableModel {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesModel(QObject *parent = 0);
    virtual ~MessagesModel();

    // Model implementation.
    bool setData(const QModelIndex &idx, const QVariant &value, int role = Qt::EditRole);
    QVariant data(const QModelIndex &idx, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &idx) const;

  public:
    // Sets up all icons which are used directly by this model.
    void setupIcons();

    // Returns const reference to message at given index.
    Message messageAt(int row_index) const;

  public slots:
    // Fetches ALL available data to the model.
    // NOTE: This is almost needed when sorting
    // and makes the model more predictable.
    void fetchAll();

    // Loads messages of given feeds.
    void loadMessages(const QList<int> feed_ids);

  private:
    // Sets up header data.
    void setupHeaderData();

    // Creates "normal" and "bold" fonts.
    void setupFonts();

  private:
    QList<QString> m_headerData;
    bool m_isInEditingMode;

    QFont m_normalFont;
    QFont m_boldFont;

    QIcon m_favoriteIcon;
    QIcon m_readIcon;
    QIcon m_unreadIcon;
};

#endif // MESSAGESMODEL_H
