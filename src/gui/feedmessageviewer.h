// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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


class WebBrowser;
class MessagesView;
class FeedsView;
class FeedDownloader;
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

  public slots:
    // Enables/disables main toolbars.
    void setToolBarsEnabled(bool enable);

    // Runs "cleanup" of the database.
    void vacuumDatabase();

    // Reloads some changeable visual settings.
    void refreshVisualProperties();

  protected slots:
    // Updates counts of messages for example in tray icon.
    void updateTrayIconStatus(int unread_messages, int total_messages);

    // Reacts on feed updates.
    void onFeedUpdatesStarted();
    void onFeedUpdatesProgress(FeedsModelFeed *feed, int current, int total);
    void onFeedUpdatesFinished();

    // Switches visibility of feed list and related
    // toolbar.
    void switchFeedComponentVisibility();

  protected:
    // Initializes some properties of the widget.
    void initialize();

    // Initializes both messages/feeds views.
    void initializeViews();

    // Sets up connections.
    void createConnections();

  private:
    bool m_toolBarsEnabled;
    QToolBar *m_toolBarFeeds;
    QToolBar *m_toolBarMessages;

    QSplitter *m_feedSplitter;
    QSplitter *m_messageSplitter;

    MessagesView *m_messagesView;
    FeedsView *m_feedsView;
    QWidget *m_feedsWidget;
    QWidget *m_messagesWidget;
    WebBrowser *m_messagesBrowser;

    QThread *m_feedDownloaderThread;
    FeedDownloader *m_feedDownloader;
};

#endif // FEEDMESSAGEVIEWER_H
