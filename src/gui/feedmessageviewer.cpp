#include <QVBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QApplication>
#include <QLineEdit>
#include <QAction>
#include <QToolButton>
#include <QMenu>
#include <QWidgetAction>

#include "gui/feedmessageviewer.h"
#include "gui/webbrowser.h"
#include "gui/formmain.h"
#include "gui/iconthemefactory.h"
#include "gui/messagesview.h"
#include "gui/feedsview.h"
#include "core/messagesproxymodel.h"


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
}

void FeedMessageViewer::initialize() {
  // Initialize/populate toolbar.
  m_toolBar->setFloatable(false);
  m_toolBar->setMovable(false);
  m_toolBar->setAllowedAreas(Qt::TopToolBarArea);
  m_toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

  QToolButton *update_button = new QToolButton(m_toolBar);
  update_button->setPopupMode(QToolButton::InstantPopup);
  update_button->setIcon(IconThemeFactory::getInstance()->fromTheme("view-refresh"));
  update_button->setText(tr("Update selected/all feeds"));
  update_button->setToolTip(tr("Select which feeds you want to update."));

  QMenu *update_menu = new QMenu(tr("Feed update menu"), update_button);
  update_menu->addAction(FormMain::getInstance()->m_ui->m_actionUpdateAllFeeds);
  update_menu->addAction(FormMain::getInstance()->m_ui->m_actionUpdateSelectedFeeds);

  update_button->setMenu(update_menu);

  QWidgetAction *update_action = new QWidgetAction(m_toolBar);
  update_action->setDefaultWidget(update_button);

  // Add everything to toolbar.
  m_toolBar->addAction(update_action);
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
  QSplitter *feed_splitter = new QSplitter(Qt::Horizontal, this);
  QSplitter *message_splitter = new QSplitter(Qt::Vertical, this);

  // Set layout properties.
  central_layout->setMargin(0);
  central_layout->setSpacing(0);

  // Set views.
  m_feedsView->setFrameStyle(QFrame::NoFrame);
  m_messagesView->setFrameStyle(QFrame::NoFrame);

  // Setup splitters.
  message_splitter->setHandleWidth(1);
  message_splitter->setChildrenCollapsible(false);
  message_splitter->setStretchFactor(0, 1);
  message_splitter->addWidget(m_messagesView);
  message_splitter->addWidget(m_messagesBrowser);

  feed_splitter->setHandleWidth(1);
  feed_splitter->setChildrenCollapsible(false);
  feed_splitter->setStretchFactor(0, 1);
  feed_splitter->addWidget(m_feedsView);
  feed_splitter->addWidget(message_splitter);

  // Add toolbar and main feeds/messages widget to main layout.
  central_layout->addWidget(m_toolBar);
  central_layout->addWidget(feed_splitter);

  // Set layout as active.
  setLayout(central_layout);
}

FeedMessageViewer::~FeedMessageViewer() {
  qDebug("Destroying FeedMessageViewer instance.");
}

WebBrowser *FeedMessageViewer::webBrowser() {
  return m_messagesBrowser;
}
