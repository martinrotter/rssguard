// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include "core/message.h"
#include "core/messagesmodelsqllayer.h"
#include "definitions/definitions.h"
#include "services/abstract/rootitem.h"

#include <QAbstractTableModel>
#include <QFont>
#include <QIcon>

class MessagesView;

class MessagesModel : public QAbstractTableModel, public MessagesModelSqlLayer {
    Q_OBJECT

  public:
    // Enum which describes basic highlighting schemes
    // for messages.
    enum class MessageHighlighter {
      NoHighlighting = 1,
      HighlightUnread = 2,
      HighlightImportant = 4
    };

    enum class MessageUnreadIcon {
      Dot = 1,
      Envelope = 2,
      FeedIcon = 3
    };

    Q_ENUM(MessageUnreadIcon)

    // Constructors and destructors.
    explicit MessagesModel(QObject* parent = nullptr);
    virtual ~MessagesModel();

    // Fetches ALL available data to the model.
    // NOTE: This activates the SQL query and populates the model with new data.
    void repopulate(int additional_article_id = 0);
    void fetchMoreArticles();

    // Model implementation.
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual bool setData(const QModelIndex& idx, const QVariant& value, int role = Qt::ItemDataRole::EditRole);
    virtual QVariant data(const QModelIndex& idx, int role = Qt::ItemDataRole::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;

    const Message& messageForRow(int row) const;
    Message& messageForRow(int row);

    QList<Message> messagesAt(const QList<int>& row_indices) const;

    QVariant data(int row, int column, int role = Qt::DisplayRole) const;
    int messageId(int row_index) const;
    RootItem::Importance messageImportance(int row_index) const;

    RootItem* loadedItem() const;

    void setupIcons();
    void setupFonts();
    void updateDateFormat();
    void updateFeedIconsDisplay();
    void reloadWholeLayout();

    // SINGLE message manipulators.
    bool switchMessageImportance(int row_index);
    bool switchMessageReadUnread(int row_index);
    bool setMessageRead(int row_index, RootItem::ReadStatus read);

    // BATCH messages manipulators.
    bool switchBatchMessageImportance(const QModelIndexList& messages);
    bool setBatchMessagesDeleted(const QModelIndexList& messages);
    bool setBatchMessagesRead(const QModelIndexList& messages, RootItem::ReadStatus read);
    bool setBatchMessagesRestored(const QModelIndexList& messages);

    // Highlights messages.
    void highlightMessages(MessageHighlighter highlighter);

    // Loads messages of given feeds.
    void loadMessages(RootItem* item);

    MessagesView* view() const;
    void setView(MessagesView* new_view);

    static QIcon generateIconForScore(double score);
    static QIcon generateUnreadIcon();
    static QString descriptionOfUnreadIcon(MessagesModel::MessageUnreadIcon type);

  public slots:
    bool setMessageImportantById(int id, RootItem::Importance important);
    bool setMessageReadById(int id, RootItem::ReadStatus read);
    bool setMessageLabelsById(int id, const QStringList& label_ids);

  private:
    void fillComputedMessageData(Message* msg);
    void setupHeaderData();

  private:
    QList<Message> m_messages;
    bool m_canFetchMoreArticles;
    MessagesView* m_view;
    MessageHighlighter m_messageHighlighter;
    QString m_customDateFormat;
    QString m_customTimeFormat;
    QString m_customFormatForDatesOnly;
    int m_newerArticlesRelativeTime;
    RootItem* m_selectedItem;
    QHash<QString, Feed*> m_hashedFeeds;
    QList<QString> m_headerData;
    QList<QString> m_tooltipData;
    QFont m_normalFont;
    QFont m_boldFont;
    QFont m_normalStrikedFont;
    QFont m_boldStrikedFont;
    QIcon m_favoriteIcon;
    QIcon m_readIcon;
    QIcon m_unreadIcon;
    QIcon m_enclosuresIcon;
    QList<QIcon> m_scoreIcons;
    MessageUnreadIcon m_unreadIconType;
    bool m_multilineListItems;
};

Q_DECLARE_METATYPE(MessagesModel::MessageHighlighter)
Q_DECLARE_METATYPE(MessagesModel::MessageUnreadIcon)

#endif // MESSAGESMODEL_H
