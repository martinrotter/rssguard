// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#ifndef FEEDMESSAGEVIEWER_H
#define FEEDMESSAGEVIEWER_H

#include "gui/tabcontent.h"

#include "core/messagesmodel.h"
#include "core/feeddownloader.h"


class WebBrowser;
class MessagesView;
class MessagesToolBar;
class FeedsToolBar;
class FeedsView;
class DatabaseCleaner;
class FeedsModelFeed;
class QToolBar;
class QSplitter;
class QProgressBar;

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

    inline MessagesView *messagesView() {
      return m_messagesView;
    }

    inline MessagesToolBar *messagesToolBar() {
      return m_toolBarMessages;
    }

    inline FeedsToolBar *feedsToolBar() {
      return m_toolBarFeeds;
    }

    DatabaseCleaner *databaseCleaner();

    // Loads/saves sizes and states of ALL
    // underlying widgets, this contains primarily
    // splitters, toolbar and views.
    void saveSize();
    void loadSize();

    // Destroys worker/feed downloader thread and
    // stops any child widgets/workers.
    void quit();

    inline bool areToolBarsEnabled() const {
      return m_toolBarsEnabled;
    }

    inline bool areListHeadersEnabled() const {
      return m_listHeadersEnabled;
    }

  public slots:
    void loadInitialFeeds();

    // Switches orientation horizontal/vertical.
    void switchMessageSplitterOrientation();

    // Enables/disables main toolbars or list headers.
    void setToolBarsEnabled(bool enable);
    void setListHeadersEnabled(bool enable);

    // Runs "cleanup" of the database.
    void showDbCleanupAssistant();

    // Reloads some changeable visual settings.
    void refreshVisualProperties();

    void updateFeeds(QList<FeedsModelFeed*> feeds);

  private slots:
    // Updates counts of messages for example in tray icon.
    void updateTrayIconStatus(int unread_messages, int total_messages, bool any_unread_messages);

    // Reacts on feed updates.
    void onFeedUpdatesStarted();
    void onFeedUpdatesProgress(FeedsModelFeed *feed, int current, int total);
    void onFeedUpdatesFinished(FeedDownloadResults results);

    // Switches visibility of feed list and related
    // toolbar.
    void switchFeedComponentVisibility();

    void updateMessageButtonsAvailability();
    void updateFeedButtonsAvailability();

  protected:
    // Initializes some properties of the widget.
    void initialize();

    // Initializes both messages/feeds views.
    void initializeViews();

    // Sets up connections.
    void createConnections();

  signals:
    // Emitted if user/application requested updating of some feeds.
    void feedsUpdateRequested(const QList<FeedsModelFeed*> feeds);

  private:
    bool m_toolBarsEnabled;
    bool m_listHeadersEnabled;
    FeedsToolBar *m_toolBarFeeds;
    MessagesToolBar *m_toolBarMessages;

    QSplitter *m_feedSplitter;
    QSplitter *m_messageSplitter;

    MessagesView *m_messagesView;
    FeedsView *m_feedsView;
    QWidget *m_feedsWidget;
    QWidget *m_messagesWidget;
    WebBrowser *m_messagesBrowser;

    QThread *m_feedDownloaderThread;
    QThread *m_dbCleanerThread;
    FeedDownloader *m_feedDownloader;
    DatabaseCleaner *m_dbCleaner;
};

#endif // FEEDMESSAGEVIEWER_H
