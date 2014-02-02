#ifndef MESSAGESVIEW_H
#define MESSAGESVIEW_H

#include "core/messagesmodel.h"

#include <QTreeView>
#include <QHeaderView>


class MessagesProxyModel;

class MessagesView : public QTreeView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesView(QWidget *parent = 0);
    virtual ~MessagesView();

    inline void setSortingEnabled(bool enable) {
      QTreeView::setSortingEnabled(enable);
      header()->setSortIndicatorShown(false);
    }

    // Model accessors.
    inline MessagesProxyModel *model() {
      return m_proxyModel;
    }

    inline MessagesModel *sourceModel() {
      return m_sourceModel;
    }

    // Creates needed connections.
    void createConnections();

  public slots:
    // Called after data got changed externally
    // and it needs to be reloaded to the view.
    // If "mark_current_index_read" is 0, then message with
    // "current" index is not marked as read.
    void reloadSelections(int mark_current_index_read);

    // Loads un-deleted messages from selected feeds.
    void loadFeeds(const QList<int> &feed_ids);

    // Message manipulators.
    void openSelectedSourceArticlesExternally();
    void openSelectedSourceMessagesInternally();
    void openSelectedMessagesInternally();

    // Works with SELECTED messages only.
    void setSelectedMessagesReadStatus(int read);
    void markSelectedMessagesRead();
    void markSelectedMessagesUnread();
    void deleteSelectedMessages();
    void switchSelectedMessagesImportance();

  protected slots:
    // Marks given indexes as selected.
    void reselectIndexes(const QModelIndexList &indexes);

    // Changes resize mode for all columns.
    void adjustColumns();

  protected:
    // Initializes context menu.
    void initializeContextMenu();

    // Sets up appearance.
    void setupAppearance();

    // Event reimplementations.
    void contextMenuEvent(QContextMenuEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void currentChanged(const QModelIndex &current,
                        const QModelIndex &previous);

  signals:
    // Link/message openers.
    void openLinkNewTab(const QString &link);
    void openMessagesInNewspaperView(const QList<Message> &messages);

    // Notify others about message selections.
    void currentMessagesChanged(const QList<Message> &messages);
    void currentMessagesRemoved();

    // Emitted if counts of unread/total messages has changed
    // because of user interaction with list of messages.
    void feedCountsChanged();

  private:
    QMenu *m_contextMenu;

    MessagesProxyModel *m_proxyModel;
    MessagesModel *m_sourceModel;

    bool m_columnsAdjusted;
    bool m_batchUnreadSwitch;
};

#endif // MESSAGESVIEW_H
