#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QPointer>
#include <QApplication>

#include <QMessageBox>

#include "core/basenetworkaccessmanager.h"
#include "core/webbrowsernetworkaccessmanager.h"
#include "core/basewebpage.h"
#include "gui/basewebview.h"
#include "gui/webbrowser.h"
#include "gui/locationlineedit.h"
#include "gui/themefactory.h"
#include "gui/tabwidget.h"


QPointer<WebBrowserNetworkAccessManager> WebBrowser::m_networkManager;
QList<WebBrowser*> WebBrowser::m_runningWebBrowsers;

WebBrowser::WebBrowser(TabWidget *parent)
  : TabContent(parent), m_layout(new QVBoxLayout(this)),
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
  m_layout->setContentsMargins(0, -1, 0, 0);

  setTabOrder(m_txtLocation, m_toolBar);
  setTabOrder(m_toolBar, m_webView);

  createConnections();
  setupIcons();
}

void WebBrowser::createConnections() {
  // When user confirms new url, then redirect to it.
  connect(m_txtLocation,
          &LocationLineEdit::submitted,
          this,
          static_cast<void (WebBrowser::*)(const QString &url)>(&WebBrowser::navigateToUrl));
  // If new page loads, then update current url.
  connect(m_webView, &BaseWebView::urlChanged,
          this, &WebBrowser::updateUrl);

  // Connect this WebBrowser to global TabWidget.
  // TODO: Přemýšlet nad tím, zda toto navázání spojení
  // nepředělat například do TabWidget::AddBrowser a ty signály
  // z m_webView tedy neforwardovat z WebBrowseru.
  TabWidget *parent_widget = static_cast<TabWidget*>(parent());
  connect(m_webView, &BaseWebView::newTabRequested,
          parent_widget, &TabWidget::addEmptyBrowser);
  connect(m_webView, &BaseWebView::linkMiddleClicked,
          parent_widget, &TabWidget::addLinkedBrowser);


  // Change location textbox status according to webpage status.
  connect(m_webView, &BaseWebView::loadProgress,
          m_txtLocation, &LocationLineEdit::setProgress);
  connect(m_webView, &BaseWebView::loadFinished,
          m_txtLocation, &LocationLineEdit::clearProgress);
}

void WebBrowser::updateUrl(const QUrl &url) {
  m_txtLocation->setText(url.toString());
}

void WebBrowser::navigateToUrl(const QUrl &url) {
  if (url.isValid()) {
    m_webView->load(url);
  }
}

void WebBrowser::navigateToUrl(const QString &textual_url) {
  navigateToUrl(QUrl::fromUserInput(textual_url));
}

WebBrowser::~WebBrowser() {
  qDebug("Erasing WebBrowser instance.");

  // Remove this instance from the global list of web browsers.
  m_runningWebBrowsers.removeAll(this);

  // Delete members.
  delete m_layout;
}

WebBrowser *WebBrowser::webBrowser() {
  return this;
}

QMenu *WebBrowser::globalMenu() {
  return nullptr;
}

QIcon WebBrowser::icon() {
  return m_webView->icon();
}

void WebBrowser::setFocus(Qt::FocusReason reason) {
  m_txtLocation->setFocus(reason);
}

void WebBrowser::setupIcons() {
  m_actionBack->setIcon(ThemeFactory::getInstance()->fromTheme("go-previous"));
  m_actionForward->setIcon(ThemeFactory::getInstance()->fromTheme("go-next"));
  m_actionReload->setIcon(ThemeFactory::getInstance()->fromTheme("view-refresh"));
  m_actionStop->setIcon(ThemeFactory::getInstance()->fromTheme("process-stop"));
  m_webView->setupIcons();
}

QList<WebBrowser *> WebBrowser::runningWebBrowsers() {
  return m_runningWebBrowsers;
}

void WebBrowser::setNavigationBarVisible(bool visible) {
  m_toolBar->setVisible(visible);
}

WebBrowserNetworkAccessManager *WebBrowser::globalNetworkManager() {
  if (m_networkManager.isNull()) {
    m_networkManager = new WebBrowserNetworkAccessManager(qApp);
  }

  return m_networkManager;
}
