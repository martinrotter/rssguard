#include "gui/feedmessageviewer.h"

#include "core/settings.h"
#include "core/messagesproxymodel.h"
#include "core/feeddownloader.h"
#include "gui/webbrowser.h"
#include "gui/formmain.h"
#include "gui/iconthemefactory.h"
#include "gui/messagesview.h"
#include "gui/feedsview.h"

#include <QVBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QApplication>
#include <QLineEdit>
#include <QAction>
#include <QToolButton>
#include <QMenu>
#include <QWidgetAction>


FeedMessageViewer::FeedMessageViewer(QWidget *parent)
  : TabContent(parent),
    m_toolBar(new QToolBar(tr("Toolbar for messages"), this)),
    m_messagesView(new MessagesView(this)),
    m_feedsView(new FeedsView(this)),
    m_messagesBrowser(new WebBrowser(this)) {
  initialize();
  initializeViews();
  createConnections();
}

FeedMessageViewer::~FeedMessageViewer() {
  qDebug("Destroying FeedMessageViewer instance.");
}

void FeedMessageViewer::saveSize() {
  Settings *settings = Settings::getInstance();

  // Store offsets of splitters.
  settings->setValue(APP_CFG_GUI,
                     "splitter_feeds",
                     m_feedSplitter->saveState());
  settings->setValue(APP_CFG_GUI,
                     "splitter_messages",
                     m_messageSplitter->saveState());

  // States of splitters are stored, let's store
  // widths of columns.
  settings->setValue(APP_CFG_GUI,
                     KEY_MESSAGES_VIEW + QString::number(MSG_DB_AUTHOR_INDEX),
                     m_messagesView->columnWidth(MSG_DB_AUTHOR_INDEX));
  settings->setValue(APP_CFG_GUI,
                     KEY_MESSAGES_VIEW + QString::number(MSG_DB_DUPDATED_INDEX),
                     m_messagesView->columnWidth(MSG_DB_DUPDATED_INDEX));
}

void FeedMessageViewer::loadSize() {
  Settings *settings = Settings::getInstance();
  int default_msg_section_size = m_messagesView->header()->defaultSectionSize();

  // Restore offsets of splitters.
  m_feedSplitter->restoreState(settings->value(APP_CFG_GUI, "splitter_feeds").toByteArray());
  m_messageSplitter->restoreState(settings->value(APP_CFG_GUI, "splitter_messages").toByteArray());

  // Splitters are restored, now, restore widths of columns.
  m_messagesView->setColumnWidth(MSG_DB_AUTHOR_INDEX,
                                 settings->value(APP_CFG_GUI,
                                                 KEY_MESSAGES_VIEW + QString::number(MSG_DB_AUTHOR_INDEX),
                                                 default_msg_section_size).toInt());
  m_messagesView->setColumnWidth(MSG_DB_DUPDATED_INDEX,
                                 settings->value(APP_CFG_GUI,
                                                 KEY_MESSAGES_VIEW + QString::number(MSG_DB_DUPDATED_INDEX),
                                                 default_msg_section_size).toInt());
}

void FeedMessageViewer::createConnections() {
  // General connections.
  connect(m_messagesView, SIGNAL(currentMessageRemoved()),
          m_messagesBrowser, SLOT(clear()));
  connect(m_messagesView, SIGNAL(currentMessageChanged(Message)),
          m_messagesBrowser, SLOT(navigateToMessage(Message)));
  connect(m_messagesView, SIGNAL(openMessageNewTabRequested(Message)),
          FormMain::getInstance()->m_ui->m_tabWidget,
          SLOT(addBrowserWithMessage(Message)));
  connect(m_messagesView, SIGNAL(openLinkMessageNewTabRequested(QString)),
          FormMain::getInstance()->m_ui->m_tabWidget,
          SLOT(addLinkedBrowser(QString)));
  connect(m_feedsView, SIGNAL(feedsSelected(QList<int>)),
          m_messagesView, SLOT(loadFeeds(QList<int>)));
  connect(m_messagesView, SIGNAL(feedCountsChanged()),
          m_feedsView, SLOT(updateCountsOfSelectedFeeds()));

  // Toolbar forwardings.
  connect(FormMain::getInstance()->m_ui->m_actionSwitchImportanceOfSelectedMessages,
          SIGNAL(triggered()), m_messagesView, SLOT(switchSelectedMessagesImportance()));
  connect(FormMain::getInstance()->m_ui->m_actionDeleteSelectedMessages,
          SIGNAL(triggered()), m_messagesView, SLOT(deleteSelectedMessages()));
  connect(FormMain::getInstance()->m_ui->m_actionMarkSelectedMessagesAsRead,
          SIGNAL(triggered()), m_messagesView, SLOT(markSelectedMessagesRead()));
  connect(FormMain::getInstance()->m_ui->m_actionMarkSelectedMessagesAsUnread,
          SIGNAL(triggered()), m_messagesView, SLOT(markSelectedMessagesUnread()));
  connect(FormMain::getInstance()->m_ui->m_actionOpenSelectedSourceArticlesExternally,
          SIGNAL(triggered()), m_messagesView, SLOT(openSelectedSourceArticlesExternally()));
  connect(FormMain::getInstance()->m_ui->m_actionOpenSelectedSourceArticlesInternally,
          SIGNAL(triggered()), m_messagesView, SLOT(openSelectedSourceMessagesInternally()));
  connect(FormMain::getInstance()->m_ui->m_actionOpenSelectedMessagesInternally,
          SIGNAL(triggered()), m_messagesView, SLOT(openSelectedMessagesInternally()));
  connect(FormMain::getInstance()->m_ui->m_actionMarkAllMessagesAsRead,
          SIGNAL(triggered()), m_messagesView, SLOT(setAllMessagesRead()));
  connect(FormMain::getInstance()->m_ui->m_actionMarkAllMessagesAsUnread,
          SIGNAL(triggered()), m_messagesView, SLOT(setAllMessagesUnread()));
  connect(FormMain::getInstance()->m_ui->m_actionDeleteAllMessages,
          SIGNAL(triggered()), m_messagesView, SLOT(setAllMessagesDeleted()));
}

void FeedMessageViewer::initialize() {
  // Initialize/populate toolbar.
  m_toolBar->setFloatable(false);
  m_toolBar->setMovable(false);
  m_toolBar->setAllowedAreas(Qt::TopToolBarArea);
  m_toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

  // Add everything to toolbar.
  m_toolBar->addAction(FormMain::getInstance()->m_ui->m_actionUpdateAllFeeds);
  m_toolBar->addAction(FormMain::getInstance()->m_ui->m_actionUpdateSelectedFeeds);
  m_toolBar->addAction(FormMain::getInstance()->m_ui->m_actionAddNewFeed);
  m_toolBar->addAction(FormMain::getInstance()->m_ui->m_actionEditSelectedFeed);
  m_toolBar->addAction(FormMain::getInstance()->m_ui->m_actionDeleteSelectedFeeds);
  m_toolBar->addSeparator();
  m_toolBar->addAction(FormMain::getInstance()->m_ui->m_actionMarkAllMessagesAsRead);
  m_toolBar->addAction(FormMain::getInstance()->m_ui->m_actionMarkAllMessagesAsUnread);
  m_toolBar->addAction(FormMain::getInstance()->m_ui->m_actionDeleteAllMessages);

  // Finish web/message browser setup.
  m_messagesBrowser->setNavigationBarVisible(false);
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
  m_messageSplitter->setChildrenCollapsible(false);
  m_messageSplitter->setStretchFactor(0, 1);
  m_messageSplitter->addWidget(m_messagesView);
  m_messageSplitter->addWidget(m_messagesBrowser);

  m_feedSplitter->setHandleWidth(1);
  m_feedSplitter->setChildrenCollapsible(false);
  m_feedSplitter->setStretchFactor(0, 1);
  m_feedSplitter->addWidget(m_feedsView);
  m_feedSplitter->addWidget(m_messageSplitter);

  // Add toolbar and main feeds/messages widget to main layout.
  central_layout->addWidget(m_toolBar);
  central_layout->addWidget(m_feedSplitter);

  // Set layout as active.
  setLayout(central_layout);
}

WebBrowser *FeedMessageViewer::webBrowser() {
  return m_messagesBrowser;
}
