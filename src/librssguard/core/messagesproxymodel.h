// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESPROXYMODEL_H
#define MESSAGESPROXYMODEL_H

#include <QSortFilterProxyModel>

class MessagesModel;
class Message;

class MessagesProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

  public:
    // Enum which describes basic filtering schemes
    // for messages.
    enum class MessageListFilter {
      NoFiltering = 1,
      ShowUnread = 2,
      ShowImportant = 4,
      ShowToday = 8,
      ShowYesterday = 16,
      ShowLast24Hours = 32,
      ShowLast48Hours = 64,
      ShowThisWeek = 128,
      ShowLastWeek = 256,
      ShowOnlyWithAttachments = 512,
      ShowOnlyWithScore = 1024,
      ShowRead = 2048
    };

    explicit MessagesProxyModel(MessagesModel* source_model, QObject* parent = nullptr);
    virtual ~MessagesProxyModel();

    QModelIndex getNextPreviousImportantItemIndex(int default_row) const;
    QModelIndex getNextPreviousUnreadItemIndex(int default_row) const;

    QModelIndex indexFromMessage(const Message& msg) const;

    // Maps list of indexes.
    QModelIndexList mapListToSource(const QModelIndexList& indexes) const;
    QModelIndexList mapListFromSource(const QModelIndexList& indexes, bool deep = false) const;

    void setMessageListFilter(MessageListFilter filter);

    // Fix for matching indexes with respect to specifics of the message model.
    virtual QModelIndexList match(const QModelIndex& start,
                                  int role,
                                  const QVariant& entered_value,
                                  int hits,
                                  Qt::MatchFlags flags) const;

    // Performs sort of items.
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    int additionalArticleId() const;
    void setAdditionalArticleId(int additional_article_id);

  private:
    void initializeFilters();

    QModelIndex getNextImportantItemIndex(int default_row, int max_row) const;
    QModelIndex getNextUnreadItemIndex(int default_row, int max_row) const;

    bool filterAcceptsMessage(int msg_row_index) const;

    virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const;
    virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

    // Source model pointer.
    MessagesModel* m_sourceModel;
    MessageListFilter m_filter;
    QMap<MessageListFilter, std::function<bool(int)>> m_filters;
    QList<MessageListFilter> m_filterKeys;
    int m_additionalArticleId;
};

Q_DECLARE_METATYPE(MessagesProxyModel::MessageListFilter)

#endif // MESSAGESPROXYMODEL_H
