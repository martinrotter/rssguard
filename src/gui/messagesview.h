#ifndef MESSAGESVIEW_H
#define MESSAGESVIEW_H

#include "core/messagesmodel.h"

#include <QTreeView>


class MessagesProxyModel;

class MessagesView : public QTreeView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesView(QWidget *parent = 0);
    virtual ~MessagesView();

    void setSortingEnabled(bool enable);

    // Model accessors.
    MessagesProxyModel *model();
    MessagesModel *sourceModel();

    void createConnections();

  public slots:
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

    // Sets ALL (unfiltered) messages read.
    void setAllMessagesReadStatus(int read);
    void setAllMessagesRead();
    void setAllMessagesUnread();

    // Sets ALL (unfiltered) messages deleted.
    void setAllMessagesDeleteStatus(int deleted);
    void setAllMessagesDeleted();

  protected slots:
    // Marks given indexes as selected.
    void reselectIndexes(const QModelIndexList &indexes);

    // Changes resize mode for all columns.
    void adjustColumns();

  protected:
    void initializeContextMenu();
    void setupAppearance();

    void contextMenuEvent(QContextMenuEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void currentChanged(const QModelIndex &current,
                        const QModelIndex &previous);

  signals:
    void openLinkMessageNewTabRequested(const QString &link);
    void openMessageNewTabRequested(const Message &message);
    void currentMessageChanged(const Message &message);
    void currentMessageRemoved();

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
