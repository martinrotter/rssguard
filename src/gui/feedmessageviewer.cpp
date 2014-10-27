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

#include "gui/feedmessageviewer.h"

#include "miscellaneous/settings.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/systemfactory.h"
#include "miscellaneous/iconfactory.h"
#include "core/messagesproxymodel.h"
#include "core/feeddownloader.h"
#include "core/feedsmodelfeed.h"
#include "network-web/webbrowser.h"
#include "gui/formmain.h"
#include "gui/messagesview.h"
#include "gui/feedsview.h"
#include "gui/statusbar.h"
#include "gui/systemtrayicon.h"
#include "gui/messagebox.h"
#include "gui/messagestoolbar.h"
#include "gui/feedstoolbar.h"

#include <QVBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QDebug>
#include <QLineEdit>
#include <QAction>
#include <QToolButton>
#include <QMenu>
#include <QWidgetAction>
#include <QThread>
#include <QProgressBar>
#include <QStatusBar>


FeedMessageViewer::FeedMessageViewer(QWidget *parent)
  : TabContent(parent),
    m_toolBarsEnabled(true),
    m_listHeadersEnabled(true),
    m_toolBarFeeds(new FeedsToolBar(tr("Toolbar for feeds"), this)),
    m_toolBarMessages(new MessagesToolBar(tr("Toolbar for messages"), this)),
    m_messagesView(new MessagesView(this)),
    m_feedsView(new FeedsView(this)),
    m_messagesBrowser(new WebBrowser(this)),
    m_feedDownloaderThread(new QThread()),
    m_feedDownloader(new FeedDownloader()) {
  initialize();
  initializeViews();
  createConnections();

  // Start the feed downloader thread.
  m_feedDownloaderThread->start();

  // Now, update all feeds if user has set it.
  m_feedsView->updateAllFeedsOnStartup();
}

FeedMessageViewer::~FeedMessageViewer() {
  qDebug("Destroying FeedMessageViewer instance.");
}

void FeedMessageViewer::saveSize() {
  Settings *settings = qApp->settings();

  m_feedsView->saveExpandedStates();

  // Store offsets of splitters.
  settings->setValue(APP_CFG_GUI, "splitter_feeds", QString(m_feedSplitter->saveState().toBase64()));
  settings->setValue(APP_CFG_GUI, "splitter_messages", QString(m_messageSplitter->saveState().toBase64()));

  // States of splitters are stored, let's store
  // widths of columns.
  int width_column_author = m_messagesView->columnWidth(MSG_DB_AUTHOR_INDEX);
  int width_column_date = m_messagesView->columnWidth(MSG_DB_DCREATED_INDEX);

  if (width_column_author != 0 && width_column_date != 0) {
    settings->setValue(APP_CFG_GUI, KEY_MESSAGES_VIEW + QString::number(MSG_DB_AUTHOR_INDEX), width_column_author);
    settings->setValue(APP_CFG_GUI, KEY_MESSAGES_VIEW + QString::number(MSG_DB_DCREATED_INDEX), width_column_date);
  }

  // Store "visibility" of toolbars and list headers.
  settings->setValue(APP_CFG_GUI, "enable_toolbars", m_toolBarsEnabled);
  settings->setValue(APP_CFG_GUI, "enable_list_headers", m_listHeadersEnabled);
}

void FeedMessageViewer::loadSize() {
  Settings *settings = qApp->settings();
  int default_msg_section_size = m_messagesView->header()->defaultSectionSize();

  m_feedsView->loadExpandedStates();

  // Restore offsets of splitters.
  m_feedSplitter->restoreState(QByteArray::fromBase64(settings->value(APP_CFG_GUI, "splitter_feeds").toString().toLocal8Bit()));
  m_messageSplitter->restoreState(QByteArray::fromBase64(settings->value(APP_CFG_GUI, "splitter_messages").toString().toLocal8Bit()));

  // Splitters are restored, now, restore widths of columns.
  m_messagesView->setColumnWidth(MSG_DB_AUTHOR_INDEX, settings->value(APP_CFG_GUI,
                                                                      KEY_MESSAGES_VIEW + QString::number(MSG_DB_AUTHOR_INDEX),
                                                                      default_msg_section_size).toInt());
  m_messagesView->setColumnWidth(MSG_DB_DCREATED_INDEX, settings->value(APP_CFG_GUI,
                                                                        KEY_MESSAGES_VIEW + QString::number(MSG_DB_DCREATED_INDEX),
                                                                        default_msg_section_size).toInt());
}

void FeedMessageViewer::quit() {
  // Quit the feeds view (stops auto-update timer etc.).
  m_feedsView->quit();

  qDebug("Quitting feed downloader thread.");
  m_feedDownloaderThread->quit();
  m_feedDownloaderThread->wait();

  qDebug("Feed downloader thread aborted. Deleting it from memory.");
  m_feedDownloader->deleteLater();

  if (qApp->settings()->value(APP_CFG_MESSAGES, "clear_read_on_exit", false).toBool()) {
    m_feedsView->clearAllReadMessages();
  }
}

void FeedMessageViewer::setToolBarsEnabled(bool enable) {
  m_toolBarsEnabled = enable;
  m_toolBarFeeds->setVisible(enable);
  m_toolBarMessages->setVisible(enable);
}

void FeedMessageViewer::setListHeadersEnabled(bool enable) {
  m_listHeadersEnabled = enable;
  m_feedsView->header()->setVisible(enable);
  m_messagesView->header()->setVisible(enable);
}

void FeedMessageViewer::updateTrayIconStatus(int unread_messages, int total_messages, bool any_unread_messages) {
  Q_UNUSED(total_messages)

  if (SystemTrayIcon::isSystemTrayActivated()) {
    qApp->trayIcon()->setNumber(unread_messages, any_unread_messages);
  }
}

void FeedMessageViewer::onFeedUpdatesStarted() {
  //: Text display in status bar when feed update is started.
  qApp->mainForm()->statusBar()->showProgress(0, tr("Feed update started"));
}

void FeedMessageViewer::onFeedUpdatesProgress(FeedsModelFeed *feed, int current, int total) {
  // Some feed got updated.
  m_feedsView->updateCountsOfParticularFeed(feed, true);
  qApp->mainForm()->statusBar()->showProgress((current * 100.0) / total,
                                              //: Text display in status bar when particular feed is updated.
                                              tr("Updated feed '%1'").arg(feed->title()));
}

void FeedMessageViewer::onFeedUpdatesFinished() {
  qApp->closeLock()->unlock();
  qApp->mainForm()->statusBar()->clearProgress();
  m_messagesView->reloadSelections(1);
}

void FeedMessageViewer::switchFeedComponentVisibility() {
  m_feedsWidget->setVisible(!m_feedsWidget->isVisible());
}

void FeedMessageViewer::createConnections() {
  FormMain *form_main = qApp->mainForm();

  // Filtering & searching.
  connect(m_toolBarMessages, SIGNAL(messageSearchPatternChanged(QString)), m_messagesView, SLOT(searchMessages(QString)));
  connect(m_toolBarMessages, SIGNAL(messageFilterChanged(MessagesModel::MessageFilter)), m_messagesView, SLOT(filterMessages(MessagesModel::MessageFilter)));

  // Message changers.
  connect(m_messagesView, SIGNAL(currentMessagesRemoved()), m_messagesBrowser, SLOT(clear()));
  connect(m_messagesView, SIGNAL(currentMessagesChanged(QList<Message>)), m_messagesBrowser, SLOT(navigateToMessages(QList<Message>)));

  // If user selects feeds, load their messages.
  connect(m_feedsView, SIGNAL(feedsSelected(QList<int>)), m_messagesView, SLOT(loadFeeds(QList<int>)));

  // If user changes status of some messages, recalculate message counts.
  connect(m_messagesView->sourceModel(), SIGNAL(messageCountsChanged(MessagesModel::MessageMode,bool,bool)),
          m_feedsView, SLOT(receiveMessageCountsChange(MessagesModel::MessageMode,bool,bool)));

  // State of many messages is changed, then we need
  // to reload selections.
  connect(m_feedsView, SIGNAL(feedsNeedToBeReloaded(int)), m_messagesView, SLOT(reloadSelections(int)));

  // If counts of unread/all messages change, update the tray icon.
  connect(m_feedsView, SIGNAL(messageCountsChanged(int,int,bool)), this, SLOT(updateTrayIconStatus(int,int,bool)));

  // Message openers.
  connect(m_messagesView, SIGNAL(openMessagesInNewspaperView(QList<Message>)),
          form_main->m_ui->m_tabWidget, SLOT(addBrowserWithMessages(QList<Message>)));
  connect(m_messagesView, SIGNAL(openLinkNewTab(QString)),
          form_main->m_ui->m_tabWidget, SLOT(addLinkedBrowser(QString)));
  connect(m_feedsView, SIGNAL(openMessagesInNewspaperView(QList<Message>)),
          form_main->m_ui->m_tabWidget, SLOT(addBrowserWithMessages(QList<Message>)));

  // Downloader connections.
  connect(m_feedDownloaderThread, SIGNAL(finished()), m_feedDownloaderThread, SLOT(deleteLater()));
  connect(m_feedsView, SIGNAL(feedsUpdateRequested(QList<FeedsModelFeed*>)), m_feedDownloader, SLOT(updateFeeds(QList<FeedsModelFeed*>)));
  connect(m_feedDownloader, SIGNAL(finished()), this, SLOT(onFeedUpdatesFinished()));
  connect(m_feedDownloader, SIGNAL(started()), this, SLOT(onFeedUpdatesStarted()));
  connect(m_feedDownloader, SIGNAL(progress(FeedsModelFeed*,int,int)), this, SLOT(onFeedUpdatesProgress(FeedsModelFeed*,int,int)));

  // Toolbar forwardings.
  connect(form_main->m_ui->m_actionSwitchImportanceOfSelectedMessages,
          SIGNAL(triggered()), m_messagesView, SLOT(switchSelectedMessagesImportance()));
  connect(form_main->m_ui->m_actionDeleteSelectedMessages,
          SIGNAL(triggered()), m_messagesView, SLOT(deleteSelectedMessages()));
  connect(form_main->m_ui->m_actionRestoreSelectedMessagesFromRecycleBin,
          SIGNAL(triggered()), m_messagesView, SLOT(restoreSelectedMessages()));
  connect(form_main->m_ui->m_actionMarkSelectedMessagesAsRead,
          SIGNAL(triggered()), m_messagesView, SLOT(markSelectedMessagesRead()));
  connect(form_main->m_ui->m_actionMarkSelectedMessagesAsUnread,
          SIGNAL(triggered()), m_messagesView, SLOT(markSelectedMessagesUnread()));
  connect(form_main->m_ui->m_actionOpenSelectedSourceArticlesExternally,
          SIGNAL(triggered()), m_messagesView, SLOT(openSelectedSourceArticlesExternally()));
  connect(form_main->m_ui->m_actionOpenSelectedSourceArticlesInternally,
          SIGNAL(triggered()), m_messagesView, SLOT(openSelectedSourceMessagesInternally()));
  connect(form_main->m_ui->m_actionOpenSelectedMessagesInternally,
          SIGNAL(triggered()), m_messagesView, SLOT(openSelectedMessagesInternally()));
  connect(form_main->m_ui->m_actionMarkAllFeedsRead,
          SIGNAL(triggered()), m_feedsView, SLOT(markAllFeedsRead()));
  connect(form_main->m_ui->m_actionMarkSelectedFeedsAsRead,
          SIGNAL(triggered()), m_feedsView, SLOT(markSelectedFeedsRead()));
  connect(form_main->m_ui->m_actionMarkSelectedFeedsAsUnread,
          SIGNAL(triggered()), m_feedsView, SLOT(markSelectedFeedsUnread()));
  connect(form_main->m_ui->m_actionClearSelectedFeeds,
          SIGNAL(triggered()), m_feedsView, SLOT(clearSelectedFeeds()));
  connect(form_main->m_ui->m_actionClearAllFeeds,
          SIGNAL(triggered()), m_feedsView, SLOT(clearAllFeeds()));
  connect(form_main->m_ui->m_actionUpdateSelectedFeedsCategories,
          SIGNAL(triggered()), m_feedsView, SLOT(updateSelectedFeeds()));
  connect(form_main->m_ui->m_actionUpdateAllFeeds,
          SIGNAL(triggered()), m_feedsView, SLOT(updateAllFeeds()));
  connect(form_main->m_ui->m_actionAddCategory,
          SIGNAL(triggered()), m_feedsView, SLOT(addNewCategory()));
  connect(form_main->m_ui->m_actionAddFeed,
          SIGNAL(triggered()), m_feedsView, SLOT(addNewFeed()));
  connect(form_main->m_ui->m_actionEditSelectedFeedCategory,
          SIGNAL(triggered()), m_feedsView, SLOT(editSelectedItem()));
  connect(form_main->m_ui->m_actionViewSelectedItemsNewspaperMode,
          SIGNAL(triggered()), m_feedsView, SLOT(openSelectedFeedsInNewspaperMode()));
  connect(form_main->m_ui->m_actionEmptyRecycleBin,
          SIGNAL(triggered()), m_feedsView, SLOT(emptyRecycleBin()));
  connect(form_main->m_ui->m_actionRestoreRecycleBin,
          SIGNAL(triggered()), m_feedsView, SLOT(restoreRecycleBin()));
  connect(form_main->m_ui->m_actionDeleteSelectedFeedCategory,
          SIGNAL(triggered()), m_feedsView, SLOT(deleteSelectedItem()));
  connect(form_main->m_ui->m_actionSwitchFeedsList,
          SIGNAL(triggered()), this, SLOT(switchFeedComponentVisibility()));
  connect(form_main->m_ui->m_actionSelectNextFeedCategory,
          SIGNAL(triggered()), m_feedsView, SLOT(selectNextItem()));
  connect(form_main->m_ui->m_actionSwitchToolBars,
          SIGNAL(toggled(bool)), this, SLOT(setToolBarsEnabled(bool)));
  connect(form_main->m_ui->m_actionSwitchListHeaders,
          SIGNAL(toggled(bool)), this, SLOT(setListHeadersEnabled(bool)));
  connect(form_main->m_ui->m_actionSelectPreviousFeedCategory,
          SIGNAL(triggered()), m_feedsView, SLOT(selectPreviousItem()));
  connect(form_main->m_ui->m_actionSelectNextMessage,
          SIGNAL(triggered()), m_messagesView, SLOT(selectNextItem()));
  connect(form_main->m_ui->m_actionSelectPreviousMessage,
          SIGNAL(triggered()), m_messagesView, SLOT(selectPreviousItem()));
  connect(form_main->m_ui->m_actionDefragmentDatabase,
          SIGNAL(triggered()), this, SLOT(vacuumDatabase()));
}

void FeedMessageViewer::initialize() {
  // Initialize/populate toolbars.
  m_toolBarFeeds->setFloatable(false);
  m_toolBarFeeds->setMovable(false);
  m_toolBarFeeds->setAllowedAreas(Qt::TopToolBarArea);
  m_toolBarFeeds->loadChangeableActions();

  m_toolBarMessages->setFloatable(false);
  m_toolBarMessages->setMovable(false);
  m_toolBarMessages->setAllowedAreas(Qt::TopToolBarArea);
  m_toolBarMessages->loadChangeableActions();

  // Finish web/message browser setup.
  m_messagesBrowser->setNavigationBarVisible(false);

  // Downloader setup.
  qRegisterMetaType<QList<FeedsModelFeed*> >("QList<FeedsModelFeed*>");
  m_feedDownloader->moveToThread(m_feedDownloaderThread);

  refreshVisualProperties();
}

void FeedMessageViewer::initializeViews() {
  m_feedsWidget = new QWidget(this);
  m_messagesWidget = new QWidget(this);
  m_feedSplitter = new QSplitter(Qt::Horizontal, this);
  m_messageSplitter = new QSplitter(Qt::Vertical, this);

  // Instantiate needed components.
  QVBoxLayout *central_layout = new QVBoxLayout(this);
  QVBoxLayout *feed_layout = new QVBoxLayout(m_feedsWidget);
  QVBoxLayout *message_layout = new QVBoxLayout(m_messagesWidget);

  // Set layout properties.
  central_layout->setMargin(0);
  central_layout->setSpacing(0);
  feed_layout->setMargin(0);
  feed_layout->setSpacing(0);
  message_layout->setMargin(0);
  message_layout->setSpacing(0);

  // Set views.
  m_feedsView->setFrameStyle(QFrame::NoFrame);
  m_messagesView->setFrameStyle(QFrame::NoFrame);

  // Setup message splitter.
  m_messageSplitter->setObjectName("MessageSplitter");
  m_messageSplitter->setHandleWidth(1);
  m_messageSplitter->setOpaqueResize(false);
  m_messageSplitter->setChildrenCollapsible(false);
  m_messageSplitter->addWidget(m_messagesView);
  m_messageSplitter->addWidget(m_messagesBrowser);

  // Assemble message-related components to single widget.
  message_layout->addWidget(m_toolBarMessages);
  message_layout->addWidget(m_messageSplitter);

  // Assemble feed-related components to another widget.
  feed_layout->addWidget(m_toolBarFeeds);
  feed_layout->addWidget(m_feedsView);

  // Assembler everything together.
  m_feedSplitter->setHandleWidth(1);
  m_feedSplitter->setOpaqueResize(false);
  m_feedSplitter->setChildrenCollapsible(false);
  m_feedSplitter->addWidget(m_feedsWidget);
  m_feedSplitter->addWidget(m_messagesWidget);

  // Add toolbar and main feeds/messages widget to main layout.
  central_layout->addWidget(m_feedSplitter);

  setTabOrder(m_feedsView, m_messagesView);
  setTabOrder(m_messagesView, m_toolBarFeeds);
  setTabOrder(m_toolBarFeeds, m_toolBarMessages);
  setTabOrder(m_toolBarMessages, m_messagesBrowser);

}

void FeedMessageViewer::vacuumDatabase() {
  if (!qApp->closeLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(tr("Cannot defragment database"),
                         tr("Database cannot be defragmented because feed update is ongoing."),
                         QSystemTrayIcon::Warning,
                         this);
    return;
  }

  if (qApp->database()->vacuumDatabase()) {
    qApp->showGuiMessage(tr("Database defragmented"),
                         tr("Database was successfully defragmented."),
                         QSystemTrayIcon::Information,
                         this);
  }
  else {
    qApp->showGuiMessage(tr("Database was not defragmented"),
                         tr("Database was not defragmented. This database backend does not support it or it cannot be defragmented now."),
                         QSystemTrayIcon::Warning,
                         this);
  }

  qApp->closeLock()->unlock();
}

void FeedMessageViewer::refreshVisualProperties() {
  Qt::ToolButtonStyle button_style = static_cast<Qt::ToolButtonStyle>(qApp->settings()->value(APP_CFG_GUI,
                                                                                              "toolbar_style",
                                                                                              Qt::ToolButtonIconOnly).toInt());

  m_toolBarFeeds->setToolButtonStyle(button_style);
  m_toolBarMessages->setToolButtonStyle(button_style);
}
