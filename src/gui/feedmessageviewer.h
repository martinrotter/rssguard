// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#include <QTextBrowser>


class WebBrowser;
class MessagesView;
class MessagesToolBar;
class FeedsToolBar;
class WebBrowser;
class FeedsView;
class StandardFeed;
class QToolBar;
class QSplitter;
class QProgressBar;

class FeedMessageViewer : public TabContent {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedMessageViewer(QWidget *parent = 0);
    virtual ~FeedMessageViewer();

    WebBrowser *webBrowser() const;
    FeedsView *feedsView() const;
    MessagesView *messagesView() const;
    MessagesToolBar *messagesToolBar() const;
    FeedsToolBar *feedsToolBar() const;

    bool areToolBarsEnabled() const;
    bool areListHeadersEnabled() const;

  public slots:
    // Loads/saves sizes and states of ALL
    // underlying widgets, this contains primarily
    // splitters, toolbar and views.
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

  private slots:
    // Called when feed update finishes.
    void onFeedsUpdateFinished();
    void onFeedsUpdateStarted();

    // Switches visibility of feed list and related
    // toolbar.
    void switchFeedComponentVisibility();

    // Toggles displayed feeds.
    void toggleShowOnlyUnreadFeeds();

    void updateMessageButtonsAvailability();
    void updateFeedButtonsAvailability();

  protected:
    // Initializes some properties of the widget.
    void initialize();

    // Initializes both messages/feeds views.
    void initializeViews();

    // Sets up connections.
    void createConnections();

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
};

#endif // FEEDMESSAGEVIEWER_H
