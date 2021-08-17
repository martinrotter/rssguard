// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESVIEW_H
#define MESSAGESVIEW_H

#include "core/messagesmodel.h"

#include "services/abstract/rootitem.h"

#include <QHeaderView>
#include <QTreeView>

class MessagesProxyModel;

class MessagesView : public QTreeView {
  Q_OBJECT

  public:
    explicit MessagesView(QWidget* parent = nullptr);
    virtual ~MessagesView();

    MessagesProxyModel* model() const;
    MessagesModel* sourceModel() const;

    void reloadFontSettings();

    QByteArray saveHeaderState() const;
    void restoreHeaderState(const QByteArray& dta);

  public slots:
    void keyboardSearch(const QString& search);

    // Called after data got changed externally
    // and it needs to be reloaded to the view.
    void reloadSelections();

    // Loads un-deleted messages from selected feeds.
    void loadItem(RootItem* item);

    // Message manipulators.
    void openSelectedSourceMessagesExternally();
    void openSelectedMessagesInternally();
    void sendSelectedMessageViaEmail();

    // Works with SELECTED messages only.
    void setSelectedMessagesReadStatus(RootItem::ReadStatus read);
    void markSelectedMessagesRead();
    void markSelectedMessagesUnread();
    void switchSelectedMessagesImportance();
    void deleteSelectedMessages();
    void restoreSelectedMessages();

    void selectNextItem();
    void selectPreviousItem();
    void selectNextUnreadItem();

    // Searchs the visible message according to given pattern.
    void searchMessages(const QString& pattern);
    void filterMessages(MessagesModel::MessageHighlighter filter);

    void switchShowUnreadOnly(bool set_new_value = false, bool show_unread_only = false);

  private slots:
    void openSelectedMessagesWithExternalTool();

    // Marks given indexes as selected.
    void reselectIndexes(const QModelIndexList& indexes);

    // Changes resize mode for all columns.
    void adjustColumns();

    // Saves current sort state.
    void onSortIndicatorChanged(int column, Qt::SortOrder order);

  signals:
    void openLinkNewTab(const QString& link);
    void openLinkMiniBrowser(const QString& link);
    void openMessagesInNewspaperView(RootItem* root, const QList<Message>& messages);

    // Notify others about message selections.
    void currentMessageChanged(const Message& message, RootItem* root);
    void currentMessageRemoved();
    void willReselectSameMessage();

  private:
    void sort(int column, Qt::SortOrder order, bool repopulate_data,
              bool change_header, bool emit_changed_from_header, bool ignore_multicolumn_sorting);

    // Creates needed connections.
    void createConnections();

    // Initializes context menu.
    void initializeContextMenu();

    // Sets up appearance.
    void setupAppearance();

    // Event reimplementations.
    void focusInEvent(QFocusEvent* event);
    void contextMenuEvent(QContextMenuEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* event);
    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    QMenu* m_contextMenu;
    MessagesProxyModel* m_proxyModel;
    MessagesModel* m_sourceModel;
    bool m_columnsAdjusted;
    bool m_processingMouse;
};

inline MessagesProxyModel* MessagesView::model() const {
  return m_proxyModel;
}

inline MessagesModel* MessagesView::sourceModel() const {
  return m_sourceModel;
}

#endif // MESSAGESVIEW_H
