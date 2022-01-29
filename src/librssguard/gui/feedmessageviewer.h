// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDMESSAGEVIEWER_H
#define FEEDMESSAGEVIEWER_H

#include "gui/tabcontent.h"

#include "core/messagesmodel.h"

#include <QTextBrowser>

#if defined(USE_WEBENGINE)
class WebBrowser;
#endif

class MessagePreviewer;
class MessagesView;
class MessagesToolBar;
class FeedsToolBar;
class WebBrowser;
class FeedsView;
class StandardFeed;
class QToolBar;
class QSplitter;
class QProgressBar;

class RSSGUARD_DLLSPEC FeedMessageViewer : public TabContent {
  Q_OBJECT

  public:
    explicit FeedMessageViewer(QWidget* parent = nullptr);
    virtual ~FeedMessageViewer();

#if defined(USE_WEBENGINE)
    virtual WebBrowser* webBrowser() const;
#endif

    FeedsView* feedsView() const;
    MessagesView* messagesView() const;
    MessagesToolBar* messagesToolBar() const;
    FeedsToolBar* feedsToolBar() const;
    MessagePreviewer* messagesBrowser() const;

    bool areToolBarsEnabled() const;
    bool areListHeadersEnabled() const;

  public slots:
    void saveSize();
    void loadSize();

    void loadMessageViewerFonts();

    // Switches orientation horizontal/vertical.
    void switchMessageSplitterOrientation();

    // Enables/disables main toolbars or list headers.
    void setToolBarsEnabled(bool enable);
    void setListHeadersEnabled(bool enable);

    // Reloads some changeable visual settings.
    void refreshVisualProperties();

    // Switches visibility of feed list and related
    // toolbar.
    void switchFeedComponentVisibility();

    void toggleShowOnlyUnreadMessages();
    void toggleShowOnlyUnreadFeeds();
    void toggleShowFeedTreeBranches();
    void toggleItemsAutoExpandingOnSelection();
    void alternateRowColorsInLists();
    void respondToMainWindowResizes();

  private slots:
    void onFeedSplitterResized();
    void onMessageSplitterResized();
    void displayMessage(const Message& message, RootItem* root);

  protected:
    void initialize();

    // Initializes both messages/feeds views.
    void initializeViews();

    // Sets up connections.
    void createConnections();

  private:
    bool m_toolBarsEnabled;
    bool m_listHeadersEnabled;
    FeedsToolBar* m_toolBarFeeds;
    MessagesToolBar* m_toolBarMessages;
    QSplitter* m_feedSplitter;
    QSplitter* m_messageSplitter;
    MessagesView* m_messagesView;
    FeedsView* m_feedsView;
    QWidget* m_feedsWidget;
    QWidget* m_messagesWidget;
    MessagePreviewer* m_messagesBrowser;
};

#endif // FEEDMESSAGEVIEWER_H
