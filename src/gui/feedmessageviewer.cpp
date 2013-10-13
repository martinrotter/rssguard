#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QApplication>

#include "gui/feedmessageviewer.h"
#include "gui/webbrowser.h"
#include "gui/messagesview.h"
#include "gui/feedsview.h"


FeedMessageViewer::FeedMessageViewer(QWidget *parent)
  : TabContent(parent),
    m_messagesView(new MessagesView(this)),
    m_feedsView(new FeedsView(this)),
    m_messagesBrowser(new WebBrowser(this))
{
  initializeViews();
}

void FeedMessageViewer::initializeViews() {
  // Instantiate needed components.
  QHBoxLayout *vertical_layout = new QHBoxLayout(this);
  QSplitter *vertical_splitter = new QSplitter(Qt::Horizontal, this);
  QSplitter *message_splitter = new QSplitter(Qt::Vertical, this);

  // Set layout properties.
  vertical_layout->setMargin(0);
  vertical_layout->setSpacing(0);

  // Set views.
  m_feedsView->setFrameStyle(QFrame::NoFrame);
  m_messagesView->setFrameStyle(QFrame::NoFrame);

  // setup splitters.
  message_splitter->setHandleWidth(1);
  message_splitter->setChildrenCollapsible(false);
  message_splitter->setStretchFactor(0, 1);
  message_splitter->addWidget(m_messagesView);
  message_splitter->addWidget(m_messagesBrowser);

  vertical_splitter->setHandleWidth(1);
  vertical_splitter->setChildrenCollapsible(false);
  vertical_splitter->setStretchFactor(0, 1);
  vertical_splitter->addWidget(m_feedsView);
  vertical_splitter->addWidget(message_splitter);
  vertical_layout->addWidget(vertical_splitter);

  // Set layout as active.
  setLayout(vertical_layout);
}

FeedMessageViewer::~FeedMessageViewer() {
  qDebug("Destroying FeedMessageViewer instance.");
}

WebBrowser *FeedMessageViewer::webBrowser() {
  return m_messagesBrowser;
}
