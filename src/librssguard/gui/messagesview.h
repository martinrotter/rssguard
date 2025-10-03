// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESVIEW_H
#define MESSAGESVIEW_H

#include "core/messagesmodel.h"
#include "core/messagesproxymodel.h"
#include "gui/reusable/basetreeview.h"
#include "gui/reusable/searchlineedit.h"
#include "services/abstract/rootitem.h"

#include <QHeaderView>
#include <QTimer>

class MessagesProxyModel;

class MessagesView : public BaseTreeView {
    Q_OBJECT

  public:
    enum class ArticleMarkingPolicy {
      // Article is marked as read right when selected.
      MarkImmediately = 0,

      // Article is marked as read with some configured delay.
      MarkWithDelay = 1,

      // Article is never marked as read on selection.
      MarkOnlyManually = 2
    };

    explicit MessagesView(QWidget* parent = nullptr);
    virtual ~MessagesView();

    MessagesProxyModel* model() const;
    MessagesModel* sourceModel() const;

    void reloadFontSettings();
    void setupArticleMarkingPolicy();

    QByteArray saveHeaderState() const;
    void restoreHeaderState(const QByteArray& dta);

  public slots:
    void copyUrlOfSelectedArticles() const;

    void keyboardSearch(const QString& search);

    // Called after data got changed externally
    // and it needs to be reloaded to the view.
    void reloadSelections();

    // Loads un-deleted messages from selected feeds.
    void loadItem(RootItem* item);

// Message manipulators.
#if defined(ENABLE_MEDIAPLAYER)
    void playSelectedArticleInMediaPlayer();
#endif

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
    void selectItemWithCursorAction(QAbstractItemView::CursorAction act);

    void selectNextUnreadItem();

    // Searchs the visible message according to given pattern.
    void searchMessages(SearchLineEdit::SearchMode mode,
                        Qt::CaseSensitivity sensitivity,
                        int custom_criteria,
                        const QString& phrase);

    void highlightMessages(MessagesModel::MessageHighlighter highlighter);
    void changeFilter(MessagesProxyModel::MessageListFilter filter);

  protected slots:
    virtual void verticalScrollbarValueChanged(int value);

  private slots:
    void openSelectedMessagesWithExternalTool();

    // Marks given indexes as selected.
    void reselectIndexes(const QModelIndexList& indexes);

    // Changes resize mode for all columns.
    void adjustColumns();

    void markSelectedMessagesReadDelayed();

    // Saves current sort state.
    void onSortIndicatorChanged(int column, Qt::SortOrder order);

  signals:
#if defined(ENABLE_MEDIAPLAYER)
    void playLinkInMediaPlayer(const QString& link);
#endif

    void openSingleMessageInNewTab(RootItem* root, const Message& message);

    // Notify others about message selections.
    void currentMessageChanged(Message message, RootItem* root);
    void currentMessageRemoved(RootItem* root);
    void willReselectSameMessage();
    void reachedEndOfList();

  private:
    void sort(int column,
              Qt::SortOrder order,
              bool repopulate_data,
              bool change_header,
              bool emit_changed_from_header,
              bool ignore_multicolumn_sorting,
              int additional_article_id = 0);

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
    bool m_processingAnyMouseButton;
    bool m_processingRightMouseButton;
    ArticleMarkingPolicy m_articleMarkingPolicy;
    int m_articleMarkingDelay;
    QTimer m_delayedArticleMarker;
    QModelIndex m_delayedArticleIndex;
};

inline MessagesProxyModel* MessagesView::model() const {
  return m_proxyModel;
}

inline MessagesModel* MessagesView::sourceModel() const {
  return m_sourceModel;
}

#endif // MESSAGESVIEW_H
