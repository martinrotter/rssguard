// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include "core/messagesmodelsqllayer.h"
#include <QSqlQueryModel>

#include "core/message.h"
#include "definitions/definitions.h"
#include "services/abstract/rootitem.h"

#include <QFont>
#include <QIcon>

class MessagesModelCache;

class MessagesModel : public QSqlQueryModel, public MessagesModelSqlLayer {
  Q_OBJECT

  public:

    // Enum which describes basic filtering schemes
    // for messages.
    enum class MessageHighlighter {
      NoHighlighting = 100,
      HighlightUnread = 101,
      HighlightImportant = 102
    };

    // Constructors and destructors.
    explicit MessagesModel(QObject* parent = nullptr);
    virtual ~MessagesModel();

    // Fetches ALL available data to the model.
    // NOTE: This activates the SQL query and populates the model with new data.
    void repopulate();

    // Model implementation.
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    QVariant data(const QModelIndex& idx, int role = Qt::DisplayRole) const;
    QVariant data(int row, int column, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;

    // Returns message at given index.

    QList<Message> messagesAt(QList<int> row_indices) const;
    Message messageAt(int row_index) const;
    int messageId(int row_index) const;
    RootItem::Importance messageImportance(int row_index) const;

    RootItem* loadedItem() const;
    MessagesModelCache* cache() const;

    void setupFonts();
    void updateDateFormat();
    void updateFeedIconsDisplay();
    void reloadWholeLayout();

    // SINGLE message manipulators.
    bool switchMessageImportance(int row_index);
    bool setMessageRead(int row_index, RootItem::ReadStatus read);

    // BATCH messages manipulators.
    bool switchBatchMessageImportance(const QModelIndexList& messages);
    bool setBatchMessagesDeleted(const QModelIndexList& messages);
    bool setBatchMessagesRead(const QModelIndexList& messages, RootItem::ReadStatus read);
    bool setBatchMessagesRestored(const QModelIndexList& messages);

    // Highlights messages.
    void highlightMessages(MessageHighlighter highlight);

    // Loads messages of given feeds.
    void loadMessages(RootItem* item);

  public slots:

    // NOTE: These methods DO NOT actually change data in the DB, just in the model.
    // These are particularly used by msg browser.
    bool setMessageImportantById(int id, RootItem::Importance important);
    bool setMessageReadById(int id, RootItem::ReadStatus read);

  private:
    void setupHeaderData();
    void setupIcons();

    MessagesModelCache* m_cache;
    MessageHighlighter m_messageHighlighter;
    QString m_customDateFormat;
    RootItem* m_selectedItem;
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
    int m_itemHeight;
    bool m_displayFeedIcons;
};

Q_DECLARE_METATYPE(MessagesModel::MessageHighlighter)

#endif // MESSAGESMODEL_H
