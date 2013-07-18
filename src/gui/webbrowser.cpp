#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QPointer>
#include <QApplication>

#include <QMessageBox>

#include "core/basenetworkaccessmanager.h"
#include "core/webbrowsernetworkaccessmanager.h"
#include "gui/basewebview.h"
#include "gui/basewebpage.h"
#include "gui/webbrowser.h"
#include "gui/locationlineedit.h"
#include "gui/themefactory.h"


QPointer<WebBrowserNetworkAccessManager> WebBrowser::m_networkManager;
QList<WebBrowser*> WebBrowser::m_runningWebBrowsers;

WebBrowser::WebBrowser(QWidget *parent)
  : QWidget(parent), m_layout(new QVBoxLayout(this)),
    m_toolBar(new QToolBar(tr("Navigation panel"), this)),
    m_webView(new BaseWebView(this)),
    m_txtLocation(new LocationLineEdit(this)),
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

  // Set properties of some components.
  m_toolBar->layout()->setMargin(0);
  m_toolBar->setFloatable(false);
  m_toolBar->setMovable(false);

  // Modify action texts.
  m_actionBack->setText(tr("Back"));
  m_actionBack->setToolTip(tr("Go back"));
  m_actionForward->setText(tr("Forward"));
  m_actionForward->setToolTip(tr("Go forward"));
  m_actionReload->setText(tr("Reload"));
  m_actionReload->setToolTip(tr("Reload current web page"));
  m_actionStop->setText(tr("Stop"));
  m_actionStop->setToolTip(tr("Stop web page loading"));

  // Add needed actions into toolbar.
  m_toolBar->addAction(m_actionBack);
  m_toolBar->addAction(m_actionForward);
  m_toolBar->addSeparator();
  m_toolBar->addAction(m_actionReload);
  m_toolBar->addAction(m_actionStop);
  m_toolBar->addWidget(m_txtLocation);

  // Setup layout.
  m_layout->addWidget(m_toolBar);
  m_layout->addWidget(m_webView);
  m_layout->setMargin(0);

  createConnections();
}

void WebBrowser::createConnections() {
  // When user confirms new url, then redirect to it.
  connect(m_txtLocation, &LocationLineEdit::submitted,
          this, &WebBrowser::navigateToUrl);
  // If new page loads, then update current url.
  connect(m_webView, &BaseWebView::urlChanged,
          this, &WebBrowser::updateUrl);

  // Change location textbox status according to webpage status.
  connect(m_webView->page(), &BaseWebPage::loadProgress,
          m_txtLocation, &LocationLineEdit::setProgress);
}

void WebBrowser::updateUrl(const QUrl &url) {
  m_txtLocation->setText(url.toString());
}

void WebBrowser::navigateToUrl(const QString &textual_url) {
  QUrl extracted_url = QUrl::fromUserInput(textual_url);

  if (extracted_url.isValid()) {
    m_webView->setUrl(extracted_url);
  }
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

WebBrowserNetworkAccessManager *WebBrowser::globalNetworkManager() {
  if (m_networkManager.isNull()) {
    // TODO: Not sure if qApp is needed here.
    m_networkManager = new WebBrowserNetworkAccessManager(qApp);
  }

  return m_networkManager;
}
