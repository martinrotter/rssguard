#ifndef FEEDMESSAGEVIEWER_H
#define FEEDMESSAGEVIEWER_H

#include "gui/tabcontent.h"

#include "core/messagesmodel.h"


class WebBrowser;
class FeedsView;
class MessagesView;
class FeedDownloader;
class QToolBar;
class QSplitter;
class QProgressBar;
class FeedsModelFeed;

class FeedMessageViewer : public TabContent {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedMessageViewer(QWidget *parent = 0);
    virtual ~FeedMessageViewer();

    // WebBrowser getter from TabContent interface.
    inline WebBrowser *webBrowser() {
      return m_messagesBrowser;
    }

    // FeedsView getter.
    inline FeedsView *feedsView() {
      return m_feedsView;
    }

    // Loads/saves sizes and states of ALL
    // underlying widgets, this contains primarily
    // splitters, toolbar and views.
    void saveSize();
    void loadSize();

    // Destroys worker/feed downloader thread.
    void quitDownloader();

  public slots:
    // Feed updates.
    void updateSelectedFeeds();
    void updateAllFeeds();

  protected slots:
    // Updates counts of messages for example in tray icon.
    void updateCountsOfMessages(int unread_messages, int total_messages);

    // Reacts on feed updates.
    void onFeedUpdatesStarted();
    void onFeedUpdatesProgress(FeedsModelFeed *feed, int current, int total);
    void onFeedUpdatesFinished();

  protected:
    // Initializes some properties of the widget.
    void initialize();

    // Initializes both messages/feeds views.
    void initializeViews();

    // Sets up connections.
    void createConnections();

  signals:
    void feedsUpdateRequested(QList<FeedsModelFeed*>);

  private:
    QToolBar *m_toolBar;

    QSplitter *m_feedSplitter;
    QSplitter *m_messageSplitter;

    MessagesView *m_messagesView;
    FeedsView *m_feedsView;
    WebBrowser *m_messagesBrowser;

    QThread *m_feedDownloaderThread;
    FeedDownloader *m_feedDownloader;
};

#endif // FEEDMESSAGEVIEWER_H
