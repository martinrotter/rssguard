#include "gui/feedmessageviewer.h"

#include "core/settings.h"
#include "core/messagesproxymodel.h"
#include "core/feeddownloader.h"
#include "core/feedsmodelstandardfeed.h"
#include "core/systemfactory.h"
#include "gui/webbrowser.h"
#include "gui/formmain.h"
#include "gui/iconthemefactory.h"
#include "gui/messagesview.h"
#include "gui/feedsview.h"
#include "gui/statusbar.h"
#include "gui/systemtrayicon.h"
#include "gui/messagebox.h"

#include <QVBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QDebug>
#include <QApplication>
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
    m_toolBar(new QToolBar(tr("Toolbar for messages"), this)),
    m_messagesView(new MessagesView(this)),
    m_feedsView(new FeedsView(this)),
    m_messagesBrowser(new WebBrowser(this)),
    m_feedDownloaderThread(new QThread()),
    m_feedDownloader(new FeedDownloader())  {
  initialize();
  initializeViews();
  createConnections();

  // Start the feed downloader thread.
  m_feedDownloaderThread->start();
}

FeedMessageViewer::~FeedMessageViewer() {
  qDebug("Destroying FeedMessageViewer instance.");
}

void FeedMessageViewer::saveSize() {
  Settings *settings = Settings::instance();

  // Store offsets of splitters.
  settings->setValue(APP_CFG_GUI,
                     "splitter_feeds",
                     QString(m_feedSplitter->saveState().toBase64()));
  settings->setValue(APP_CFG_GUI,
                     "splitter_messages",
                     QString(m_messageSplitter->saveState().toBase64()));

  // States of splitters are stored, let's store
  // widths of columns.
  settings->setValue(APP_CFG_GUI,
                     KEY_MESSAGES_VIEW + QString::number(MSG_DB_AUTHOR_INDEX),
                     m_messagesView->columnWidth(MSG_DB_AUTHOR_INDEX));
  settings->setValue(APP_CFG_GUI,
                     KEY_MESSAGES_VIEW + QString::number(MSG_DB_DCREATED_INDEX),
                     m_messagesView->columnWidth(MSG_DB_DCREATED_INDEX));
}

void FeedMessageViewer::loadSize() {
  Settings *settings = Settings::instance();
  int default_msg_section_size = m_messagesView->header()->defaultSectionSize();

  // Restore offsets of splitters.
  m_feedSplitter->restoreState(QByteArray::fromBase64(settings->value(APP_CFG_GUI, "splitter_feeds").toString().toLocal8Bit()));
  m_messageSplitter->restoreState(QByteArray::fromBase64(settings->value(APP_CFG_GUI, "splitter_messages").toString().toLocal8Bit()));

  // Splitters are restored, now, restore widths of columns.
  m_messagesView->setColumnWidth(MSG_DB_AUTHOR_INDEX,
                                 settings->value(APP_CFG_GUI,
                                                 KEY_MESSAGES_VIEW + QString::number(MSG_DB_AUTHOR_INDEX),
                                                 default_msg_section_size).toInt());
  m_messagesView->setColumnWidth(MSG_DB_DCREATED_INDEX,
                                 settings->value(APP_CFG_GUI,
                                                 KEY_MESSAGES_VIEW + QString::number(MSG_DB_DCREATED_INDEX),
                                                 default_msg_section_size).toInt());
}

void FeedMessageViewer::quitDownloader() {
  qDebug("Quitting feed downloader thread.");
  m_feedDownloaderThread->quit();

  qDebug("Feed downloader thread aborted. Deleting it from memory.");
  m_feedDownloader->deleteLater();
}

void FeedMessageViewer::updateTrayIconStatus(int unread_messages,
                                               int total_messages) {
  Q_UNUSED(total_messages)

  // TODO: Optimize the call isSystemTrayActivated()
  // because it opens settings (use member variable)?.
  if (SystemTrayIcon::isSystemTrayActivated()) {
    SystemTrayIcon::instance()->setNumber(unread_messages);
  }
}

void FeedMessageViewer::onFeedUpdatesStarted() {
  FormMain::instance()->statusBar()->showProgress(0, tr("Feed update started"));
}

void FeedMessageViewer::onFeedUpdatesProgress(FeedsModelFeed *feed,
                                              int current,
                                              int total) {
  // Some feed got updated.
  m_feedsView->updateCountsOfParticularFeed(feed, true);
  FormMain::instance()->statusBar()->showProgress((current * 100.0) / total,
                                                     tr("Updated feed '%1'").arg(feed->title()));
}

void FeedMessageViewer::onFeedUpdatesFinished() {
  // Updates of some feeds finished, unlock the lock.
  SystemFactory::instance()->applicationCloseLock()->unlock();
  FormMain::instance()->statusBar()->clearProgress();
}

void FeedMessageViewer::createConnections() {
  FormMain *form_main = FormMain::instance();

  // Message changers.
  connect(m_messagesView, SIGNAL(currentMessagesRemoved()),
          m_messagesBrowser, SLOT(clear()));
  connect(m_messagesView, SIGNAL(currentMessagesChanged(QList<Message>)),
          m_messagesBrowser, SLOT(navigateToMessages(QList<Message>)));

  // If user selects feeds, load their messages.
  connect(m_feedsView, SIGNAL(feedsSelected(QList<int>)),
          m_messagesView, SLOT(loadFeeds(QList<int>)));

  // If user changes status of some messages, recalculate message counts.
  connect(m_messagesView, SIGNAL(feedCountsChanged()),
          m_feedsView, SLOT(updateCountsOfSelectedFeeds()));

  // State of many messages is changed, then we need
  // to reload selections.
  connect(m_feedsView, SIGNAL(feedsNeedToBeReloaded(int)),
          m_messagesView, SLOT(reloadSelections(int)));

  // If counts of unread/all messages change, update the tray icon.
  connect(m_feedsView, SIGNAL(feedCountsChanged(int,int)),
          this, SLOT(updateTrayIconStatus(int,int)));

  // Message openers.
  connect(m_messagesView, SIGNAL(openMessagesInNewspaperView(QList<Message>)),
          form_main->m_ui->m_tabWidget,
          SLOT(addBrowserWithMessages(QList<Message>)));
  connect(m_messagesView, SIGNAL(openLinkNewTab(QString)),
          form_main->m_ui->m_tabWidget,
          SLOT(addLinkedBrowser(QString)));
  connect(m_feedsView, SIGNAL(openMessagesInNewspaperView(QList<Message>)),
          form_main->m_ui->m_tabWidget,
          SLOT(addBrowserWithMessages(QList<Message>)));

  // Downloader connections.
  connect(m_feedDownloaderThread, SIGNAL(finished()),
          m_feedDownloaderThread, SLOT(deleteLater()));
  connect(m_feedsView, SIGNAL(feedsUpdateRequested(QList<FeedsModelFeed*>)),
          m_feedDownloader, SLOT(updateFeeds(QList<FeedsModelFeed*>)));
  connect(m_feedDownloader, SIGNAL(finished()),
          this, SLOT(onFeedUpdatesFinished()));
  connect(m_feedDownloader, SIGNAL(started()),
          this, SLOT(onFeedUpdatesStarted()));
  connect(m_feedDownloader, SIGNAL(progress(FeedsModelFeed*,int,int)),
          this, SLOT(onFeedUpdatesProgress(FeedsModelFeed*,int,int)));

  // Toolbar forwardings.
  connect(form_main->m_ui->m_actionSwitchImportanceOfSelectedMessages,
          SIGNAL(triggered()), m_messagesView, SLOT(switchSelectedMessagesImportance()));
  connect(form_main->m_ui->m_actionDeleteSelectedMessages,
          SIGNAL(triggered()), m_messagesView, SLOT(deleteSelectedMessages()));
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
  connect(form_main->m_ui->m_actionClearFeeds,
          SIGNAL(triggered()), m_feedsView, SLOT(clearSelectedFeeds()));
  connect(form_main->m_ui->m_actionUpdateSelectedFeedsCategories,
          SIGNAL(triggered()), m_feedsView, SLOT(updateSelectedFeeds()));
  connect(form_main->m_ui->m_actionUpdateAllFeeds,
          SIGNAL(triggered()), m_feedsView, SLOT(updateAllFeeds()));
  connect(form_main->m_ui->m_actionAddStandardCategory,
          SIGNAL(triggered()), m_feedsView, SLOT(addNewStandardCategory()));
  connect(form_main->m_ui->m_actionAddStandardFeed,
          SIGNAL(triggered()), m_feedsView, SLOT(addNewStandardFeed()));
  connect(form_main->m_ui->m_actionEditSelectedFeedCategory,
          SIGNAL(triggered()), m_feedsView, SLOT(editSelectedItem()));
  connect(form_main->m_ui->m_actionViewSelectedItemsNewspaperMode,
          SIGNAL(triggered()), m_feedsView, SLOT(openSelectedFeedsInNewspaperMode()));
  connect(form_main->m_ui->m_actionDeleteSelectedFeedCategory,
          SIGNAL(triggered()), m_feedsView, SLOT(deleteSelectedItem()));
}

void FeedMessageViewer::initialize() {
  // Initialize/populate toolbar.
  m_toolBar->setFloatable(false);
  m_toolBar->setMovable(false);
  m_toolBar->setAllowedAreas(Qt::TopToolBarArea);
  m_toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

  // Add everything to toolbar.
  m_toolBar->addAction(FormMain::instance()->m_ui->m_actionUpdateAllFeeds);
  m_toolBar->addAction(FormMain::instance()->m_ui->m_actionMarkAllFeedsRead);
  m_toolBar->addSeparator();
  m_toolBar->addAction(FormMain::instance()->m_ui->m_actionUpdateSelectedFeedsCategories);
  m_toolBar->addAction(FormMain::instance()->m_ui->m_actionEditSelectedFeedCategory);
  m_toolBar->addAction(FormMain::instance()->m_ui->m_actionDeleteSelectedFeedCategory);
  m_toolBar->addSeparator();
  m_toolBar->addAction(FormMain::instance()->m_ui->m_actionMarkSelectedFeedsAsRead);
  m_toolBar->addAction(FormMain::instance()->m_ui->m_actionMarkSelectedFeedsAsUnread);
  m_toolBar->addAction(FormMain::instance()->m_ui->m_actionClearFeeds);

  // Finish web/message browser setup.
  m_messagesBrowser->setNavigationBarVisible(false);

  // Downloader setup.
  qRegisterMetaType<QList<FeedsModelFeed*> >("QList<FeedsModelFeed*>");
  m_feedDownloader->moveToThread(m_feedDownloaderThread);
}

void FeedMessageViewer::initializeViews() {
  // Instantiate needed components.
  QVBoxLayout *central_layout = new QVBoxLayout(this);
  m_feedSplitter = new QSplitter(Qt::Horizontal, this);
  m_messageSplitter = new QSplitter(Qt::Vertical, this);

  // Set layout properties.
  central_layout->setMargin(0);
  central_layout->setSpacing(0);

  // Set views.
  m_feedsView->setFrameStyle(QFrame::NoFrame);
  m_messagesView->setFrameStyle(QFrame::NoFrame);

  // Setup splitters.
  m_messageSplitter->setHandleWidth(1);
  m_messageSplitter->setOpaqueResize(false);
  m_messageSplitter->setChildrenCollapsible(false);
  m_messageSplitter->addWidget(m_messagesView);
  m_messageSplitter->addWidget(m_messagesBrowser);

  m_feedSplitter->setHandleWidth(1);
  m_feedSplitter->setOpaqueResize(false);
  m_feedSplitter->setChildrenCollapsible(false);
  m_feedSplitter->addWidget(m_feedsView);
  m_feedSplitter->addWidget(m_messageSplitter);

  // Add toolbar and main feeds/messages widget to main layout.
  central_layout->addWidget(m_toolBar);
  central_layout->addWidget(m_feedSplitter);

  // Set layout as active.
  setLayout(central_layout);
}
