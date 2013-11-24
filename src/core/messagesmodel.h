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
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const;
    QVariant data(int row, int column, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &idx) const;

  public:
    // Sets up all icons which are used directly by this model.
    void setupIcons();

    // Returns message at given index.
    Message messageAt(int row_index) const;
    int messageId(int row_index) const;

  public slots:
    // CORE messages manipulators.
    // NOTE: These are used to change properties of one message.
    // NOTE: Model is NOT reset after these methods are applied.
    bool switchMessageImportance(int row_index);
    bool setMessageDeleted(int row_index, int deleted);
    bool setMessageRead(int row_index, int read);

    // BATCH messages manipulators.
    // NOTE: These methods are used for changing of attributes of
    // many messages via DIRECT SQL calls.
    // NOTE: Model is reset after these methods is applied.
    bool switchBatchMessageImportance(const QModelIndexList &messages);
    bool setBatchMessagesDeleted(const QModelIndexList &messages, int deleted);
    bool setBatchMessagesRead(const QModelIndexList &messages, int read);

    // Fetches ALL available data to the model.
    // NOTE: This is almost always needed when sorting
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
    QList<int> m_currentFeeds;
    QList<QString> m_headerData;
    bool m_isInEditingMode;

    QFont m_normalFont;
    QFont m_boldFont;

    QIcon m_favoriteIcon;
    QIcon m_readIcon;
    QIcon m_unreadIcon;
};

#endif // MESSAGESMODEL_H
