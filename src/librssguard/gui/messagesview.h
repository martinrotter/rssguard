// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESVIEW_H
#define MESSAGESVIEW_H

#include "core/feedsmodel.h"
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

    virtual void keyboardSearch(const QString& search);

    MessagesProxyModel* model() const;
    MessagesModel* sourceModel() const;

    void reloadFontSettings();
    void setupArticleMarkingPolicy();

    void adjustSort(int column, Qt::SortOrder order, bool emit_changed_from_header, bool ignore_multicolumn_sorting);

    QByteArray saveHeaderState() const;
    void restoreHeaderState(const QByteArray& dta);

  public slots:
    void goToMotherFeed();

    void copyUrlOfSelectedArticles() const;

    // Called after data got changed externally
    // and it needs to be reloaded to the view.
    void reactOnExternalDataChange(RootItem* item, FeedsModel::ExternalDataChange cause);

    // Loads messages from selected item.
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

  protected:
    virtual void focusInEvent(QFocusEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

  private slots:
    void onArticleLabelIdsChanged(const QList<Message>& msgs);
    void openSelectedMessagesWithExternalTool();
    void adjustColumns();
    void markSelectedMessagesReadDelayed();
    void onSortIndicatorChanged(int column, Qt::SortOrder order);

  signals:
#if defined(ENABLE_MEDIAPLAYER)
    void playLinkInMediaPlayer(const QString& link);
#endif

    void selectInFeedsView(RootItem* item);
    void openSingleMessageInNewTab(RootItem* root, const Message& message);

    // Notify others about message selections.
    void currentMessageChanged(Message message, RootItem* root);
    void currentMessageRemoved(RootItem* root);
    void reachedEndOfList();

  private:
    void reselectIndexes(const QModelIndexList& indexes);
    void reselectArticle(bool ensure_article_reviewed, bool do_not_modify_selection, int article_id);
    void createConnections();
    void initializeContextMenu();
    void setupAppearance();
    void requestArticleDisplay(const Message& msg);
    void requestArticleHiding();

  private:
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
