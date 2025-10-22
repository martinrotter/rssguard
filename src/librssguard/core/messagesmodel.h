// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include "core/message.h"
#include "core/messagesmodelsqllayer.h"
#include "services/abstract/rootitem.h"

#include <QAbstractTableModel>
#include <QFont>
#include <QIcon>

#define BATCH_SIZE 10000

class MessagesView;

class MessagesModel : public QAbstractTableModel, public MessagesModelSqlLayer {
    Q_OBJECT

  public:
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

    // Fetches available data to the model.
    // NOTE: This activates the SQL query and populates the model with new data.
    // Not all data are necessarily fetched, some might be lazy-fetched later.
    void fetchInitialArticles(int batch_size = BATCH_SIZE);
    void fetchMoreArticles(int batch_size = BATCH_SIZE);

    // Model implementation.
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual bool setData(const QModelIndex& idx, const QVariant& value, int role = Qt::ItemDataRole::EditRole);
    virtual QVariant data(const QModelIndex& idx, int role = Qt::ItemDataRole::EditRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;

    const Message& messageForRow(int row) const;
    Message& messageForRow(int row);
    int rowForMessage(int message_id) const;
    QModelIndex indexForMessage(int message_id) const;

    QList<Message> messagesAt(const QList<int>& row_indices) const;

    QVariant data(int row, int column, int role = Qt::ItemDataRole::EditRole) const;

    int messageId(int row_index) const;
    RootItem::Importance messageImportance(int row_index) const;

    RootItem* loadedItem() const;

    // Loads messages of given feeds.
    void loadMessages(RootItem* item, bool keep_additional_article_id = false);

    void setupIcons();
    void setupFonts();
    void updateDateFormat();
    void updateFeedIconsDisplay();
    void reloadLazyLoading();
    void reloadWholeLayout();
    void reloadChangedLayout(const QModelIndexList& indices);

    // SINGLE message manipulators.
    bool switchMessageImportance(int row_index);
    bool switchMessageReadUnread(int row_index);
    bool setMessageRead(int row_index, RootItem::ReadStatus read);

    // BATCH messages manipulators.
    void switchBatchMessageImportance(const QModelIndexList& messages);
    void setBatchMessagesDeleted(const QModelIndexList& messages);
    void setBatchMessagesRead(const QModelIndexList& messages, RootItem::ReadStatus read);
    void setBatchMessagesRestored(const QModelIndexList& messages);

    // DATA only manipulators.
    // NOTE: These only edit model data and do not make any DB writes.
    void markArticleDataReadUnread(bool read);

    // Highlights messages.
    void highlightMessages(MessageHighlighter highlighter);

    // NOTE: Additional article ID, which should NOT be filtered out
    // when sorting/filtering with proxy model.
    //
    // This is usually selected article etc.
    int additionalArticleId() const;
    void setAdditionalArticleId(int additional_article_id);

    MessagesView* view() const;
    void setView(MessagesView* new_view);

    static QIcon generateIconForScore(double score);
    static QIcon generateUnreadIcon();
    static QString descriptionOfUnreadIcon(MessagesModel::MessageUnreadIcon type);

    bool lazyLoading() const;

  public slots:
    void fetchAllArticles();

    bool setMessageImportantById(int id, RootItem::Importance important);
    bool setMessageReadById(int id, RootItem::ReadStatus read);
    bool setMessageLabelsById(int id, const QList<Label*>& labels);

  private:
    QString formatLabels(const QList<Label*>& labels) const;

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
    QHash<int, Feed*> m_hashedFeeds;
    QHash<QString, Label*> m_hashedLabels;
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
    int m_additionalArticleId;
    bool m_lazyLoading;
};

Q_DECLARE_METATYPE(MessagesModel::MessageHighlighter)
Q_DECLARE_METATYPE(MessagesModel::MessageUnreadIcon)

#endif // MESSAGESMODEL_H
