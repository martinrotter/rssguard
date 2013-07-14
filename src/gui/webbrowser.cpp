#include <QVBoxLayout>
#include <QToolBar>
#include <QPointer>

#include "core/basenetworkaccessmanager.h"
#include "gui/basewebview.h"
#include "gui/webbrowser.h"
#include "gui/themefactory.h"


QPointer<BaseNetworkAccessManager> WebBrowser::m_networkManager;

WebBrowser::WebBrowser(QWidget *parent)
  : QWidget(parent), m_layout(new QVBoxLayout(this)) {

  // TODO: Make this better, add location box, search box, better icons for buttons,
  // note that icons must be loaded via separate method,
  // and main window will be responsible for reloading
  // icons on all web browsers.
  QToolBar *bar = new QToolBar(tr("Navigation panel"), this);
  BaseWebView *view = new BaseWebView(this);

  bar->addAction(view->pageAction(QWebPage::Back));
  bar->addAction(view->pageAction(QWebPage::Forward));
  bar->addAction(view->pageAction(QWebPage::Reload));
  bar->addAction(view->pageAction(QWebPage::Stop));

  m_layout->addWidget(bar);
  m_layout->addWidget(view);
  m_layout->setMargin(0);

  view->load(QUrl("http://www.seznam.cz"));
}

WebBrowser::~WebBrowser() {
  qDebug("Erasing WebBrowser instance.");

  delete m_layout;
}

BaseNetworkAccessManager *WebBrowser::getNetworkManager() {
  if (m_networkManager.isNull()) {
    m_networkManager = new BaseNetworkAccessManager();
  }

  return m_networkManager;
}
