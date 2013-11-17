#include <QVBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QApplication>
#include <QLineEdit>

#include "gui/feedmessageviewer.h"
#include "gui/webbrowser.h"
#include "gui/messagesview.h"
#include "gui/feedsview.h"


FeedMessageViewer::FeedMessageViewer(QWidget *parent)
  : TabContent(parent),
    m_toolBar(new QToolBar(tr("Toolbar for messages"), this)),
    m_messagesView(new MessagesView(this)),
    m_feedsView(new FeedsView(this)),
    m_messagesBrowser(new WebBrowser(this)) {
  initialize();
  initializeViews();

  // TODO: oddělit do createConnections();
  connect(m_messagesView, SIGNAL(currentMessageChanged(Message)),
          m_messagesBrowser, SLOT(navigateToMessage(Message)));

}

void FeedMessageViewer::initialize() {
  // Initialize/populate toolbar.
  m_toolBar->setFloatable(false);
  m_toolBar->setMovable(false);
  m_toolBar->setAllowedAreas(Qt::TopToolBarArea);

  m_toolBar->addAction(QIcon::fromTheme("application-exit"), "aaa");

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
