#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QPointer>
#include <QLineEdit>

#include "core/basenetworkaccessmanager.h"
#include "gui/basewebview.h"
#include "gui/webbrowser.h"
#include "gui/themefactory.h"


QPointer<BaseNetworkAccessManager> WebBrowser::m_networkManager;
QList<WebBrowser*> WebBrowser::m_runningWebBrowsers;

WebBrowser::WebBrowser(QWidget *parent)
  : QWidget(parent), m_layout(new QVBoxLayout(this)),
    m_toolBar(new QToolBar(tr("Navigation panel"), this)),
    m_webView(new BaseWebView(this)),
    m_actionBack(m_webView->pageAction(QWebPage::Back)),
    m_actionForward(m_webView->pageAction(QWebPage::Forward)),
    m_actionReload(m_webView->pageAction(QWebPage::Reload)),
    m_actionStop(m_webView->pageAction(QWebPage::Stop)) {

  // Add this new instance to the global list of web browsers.
  // NOTE: This is used primarily for dynamic icon theme switching.
  m_runningWebBrowsers.append(this);

  // TODO: Make this better, add location box, search box, better icons for buttons,
  // note that icons must be loaded via separate method,
  // and main window will be responsible for reloading
  // icons on all web browsers.

  // Set toolbar properties.
  m_toolBar->layout()->setMargin(0);
  m_toolBar->setFloatable(false);
  m_toolBar->setMovable(false);

  // Add needed actions into toolbar.
  m_toolBar->addAction(m_actionBack);
  m_toolBar->addAction(m_actionForward);
  m_toolBar->addSeparator();
  m_toolBar->addAction(m_actionReload);
  m_toolBar->addAction(m_actionStop);

  QLineEdit *ed = new QLineEdit(this);
  m_toolBar->addWidget(ed);

  // Setup layout.
  m_layout->addWidget(m_toolBar);
  m_layout->addWidget(m_webView);
  m_layout->setMargin(0);

  m_webView->load(QUrl("http://www.seznam.cz"));
}

WebBrowser::~WebBrowser() {
  qDebug("Erasing WebBrowser instance.");

  // Remove this instance from the global list of web browsers.
  m_runningWebBrowsers.removeAll(this);

  // Delete members.
  delete m_layout;
}

void WebBrowser::setupIcons() {
  m_actionBack->setIcon(ThemeFactory::fromTheme("go-previous"));
  m_actionForward->setIcon(ThemeFactory::fromTheme("go-next"));
  m_actionReload->setIcon(ThemeFactory::fromTheme("view-refresh"));
  m_actionStop->setIcon(ThemeFactory::fromTheme("process-stop"));
}

QList<WebBrowser *> WebBrowser::runningWebBrowsers() {
  return m_runningWebBrowsers;
}

BaseNetworkAccessManager *WebBrowser::globalNetworkManager() {
  if (m_networkManager.isNull()) {
    m_networkManager = new BaseNetworkAccessManager();
  }

  return m_networkManager;
}
