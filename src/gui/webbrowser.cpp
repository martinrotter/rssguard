#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QPointer>
#include <QApplication>
#include <QWebFrame>
#include <QWidgetAction>
#include <QSlider>

#include <QLabel>
#include <QMessageBox>
#include <QToolButton>

#include "core/defs.h"
#include "core/basenetworkaccessmanager.h"
#include "core/webbrowsernetworkaccessmanager.h"
#include "core/basewebpage.h"
#include "gui/skinfactory.h"
#include "gui/basewebview.h"
#include "gui/webbrowser.h"
#include "gui/formmain.h"
#include "gui/locationlineedit.h"
#include "gui/iconthemefactory.h"
#include "gui/tabwidget.h"


QPointer<WebBrowserNetworkAccessManager> WebBrowser::m_networkManager;
QList<WebBrowser*> WebBrowser::m_runningWebBrowsers;

WebBrowser::WebBrowser(QWidget *parent)
  : TabContent(parent),
    m_layout(new QVBoxLayout(this)),
    m_toolBar(new QToolBar(tr("Navigation panel"), this)),
    m_webView(new BaseWebView(this)),
    m_txtLocation(new LocationLineEdit(this)),
    m_actionBack(m_webView->pageAction(QWebPage::Back)),
    m_actionForward(m_webView->pageAction(QWebPage::Forward)),
    m_actionReload(m_webView->pageAction(QWebPage::Reload)),
    m_actionStop(m_webView->pageAction(QWebPage::Stop)) {

  m_index = -1;

  // Add this new instance to the global list of web browsers.
  // NOTE: This is used primarily for dynamic icon theme switching.
  m_runningWebBrowsers.append(this);

  // Set properties of some components.
  m_toolBar->setFloatable(false);
  m_toolBar->setMovable(false);
  m_toolBar->setAllowedAreas(Qt::TopToolBarArea);

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
  m_toolBar->addAction(m_actionReload);
  m_toolBar->addAction(m_actionStop);
  m_toolBar->addWidget(m_txtLocation);

  // Setup layout.
  m_layout->addWidget(m_toolBar);
  m_layout->addWidget(m_webView);
  m_layout->setMargin(0);
  m_layout->setSpacing(0);

  setTabOrder(m_txtLocation, m_toolBar);
  setTabOrder(m_toolBar, m_webView);

  createConnections();
  initializeZoomWidget();

  // NOTE: Not needed here probably.
  //setupIcons();
}

void WebBrowser::initializeZoomWidget() {
  // Initializations.
  m_zoomButtons = new QWidget(this);
  QLabel *zoom_label = new QLabel(tr("Zoom  "), m_zoomButtons);
  QHBoxLayout *layout = new QHBoxLayout(m_zoomButtons);
  QToolButton *button_decrease = new QToolButton(m_zoomButtons);
  m_btnResetZoom = new QToolButton(m_zoomButtons);
  QToolButton *button_increase = new QToolButton(m_zoomButtons);

  // Set texts.
  button_decrease->setText("-");
  button_decrease->setToolTip(tr("Decrease zoom."));
  m_btnResetZoom->setText("100%");
  m_btnResetZoom->setToolTip(tr("Reset zoom to default."));
  button_increase->setText("+");
  button_increase->setToolTip(tr("Increase zoom."));

  // Setup layout.
  layout->addWidget(zoom_label);
  layout->addWidget(button_decrease);
  layout->addWidget(m_btnResetZoom);
  layout->addWidget(button_increase);
  layout->setSpacing(2);
  layout->setMargin(3);
  m_zoomButtons->setLayout(layout);

  // Make connections..
  connect(button_increase, SIGNAL(clicked()), this, SLOT(increaseZoom()));
  connect(button_decrease, SIGNAL(clicked()), this, SLOT(decreaseZoom()));
  connect(m_btnResetZoom, SIGNAL(clicked()), this, SLOT(resetZoom()));

  m_actionZoom = new QWidgetAction(this);
  m_actionZoom->setDefaultWidget(m_zoomButtons);
}

void WebBrowser::createConnections() {
  // When user confirms new url, then redirect to it.
  connect(m_txtLocation,SIGNAL(submitted(QString)),
          this, SLOT(navigateToUrl(QString)));
  // If new page loads, then update current url.
  connect(m_webView, SIGNAL(urlChanged(QUrl)), this, SLOT(updateUrl(QUrl)));

  // Connect this WebBrowser to global TabWidget.
  TabWidget *tab_widget = FormMain::getInstance()->getTabWidget();
  connect(m_webView, SIGNAL(newTabRequested()), tab_widget, SLOT(addEmptyBrowser()));
  connect(m_webView, SIGNAL(linkMiddleClicked(QUrl)),
          tab_widget, SLOT(addLinkedBrowser(QUrl)));

  // Change location textbox status according to webpage status.
  connect(m_webView, SIGNAL(loadProgress(int)), m_txtLocation, SLOT(setProgress(int)));
  connect(m_webView, SIGNAL(loadFinished(bool)), m_txtLocation, SLOT(clearProgress()));

  // Forward title/icon changes.
  connect(m_webView, SIGNAL(titleChanged(QString)), this, SLOT(onTitleChanged(QString)));
  connect(m_webView, SIGNAL(iconChanged()), this, SLOT(onIconChanged()));

  // Misc connections.
  connect(m_webView, SIGNAL(zoomFactorChanged()), this, SLOT(updateZoomGui()));
}

void WebBrowser::onIconChanged() {
  emit iconChanged(m_index, m_webView->icon());
}

void WebBrowser::onTitleChanged(const QString &new_title) {
  if (new_title.isEmpty()) {
    emit titleChanged(m_index, tr("No title"));
  }
  else {
    emit titleChanged(m_index, new_title);
  }

  emit iconChanged(m_index, QIcon());
}

void WebBrowser::updateUrl(const QUrl &url) {
  m_txtLocation->setText(url.toString());
}

void WebBrowser::navigateToUrl(const QUrl &url) {
  if (url.isValid()) {
    m_webView->load(url);
  }
}

void WebBrowser::clear() {
  m_webView->load(QUrl());
}

void WebBrowser::navigateToMessage(const Message &message) {
  m_webView->setHtml(SkinFactory::getInstance()->getCurrentMarkup().arg(message.m_title,
                                                                        tr("Written by ") + message.m_author,
                                                                        message.m_url,
                                                                        message.m_contents,
                                                                        message.m_updated.toString(Qt::ISODate)));
}

void WebBrowser::updateZoomGui() {
  m_btnResetZoom->setText(QString("%1%").arg(QString::number(m_webView->zoomFactor() * 100,
                                                             'f',
                                                             0)));
}

void WebBrowser::increaseZoom() {
  m_webView->increaseWebPageZoom();
  updateZoomGui();
}

void WebBrowser::decreaseZoom() {
  m_webView->decreaseWebPageZoom();
  updateZoomGui();
}

void WebBrowser::resetZoom() {
  m_webView->resetWebPageZoom();
  updateZoomGui();
}

void WebBrowser::navigateToUrl(const QString &textual_url) {
  // Prepare input url.
  QString better_url = textual_url;
  better_url = better_url.replace('\\', '/');

  navigateToUrl(QUrl::fromUserInput(better_url));
}

WebBrowser::~WebBrowser() {
  qDebug("Destroying WebBrowser instance.");

  // Remove this instance from the global list of web browsers.
  m_runningWebBrowsers.removeAll(this);

  // Delete members.
  delete m_layout;
  delete m_zoomButtons;
  delete m_actionZoom;
}

WebBrowser *WebBrowser::webBrowser() {
  return this;
}

QList<QAction *> WebBrowser::globalMenu() {
  QList<QAction*> browser_menu;

  // Add needed actions into the menu.
  browser_menu.append(m_actionZoom);

  return browser_menu;
}

QIcon WebBrowser::icon() {
  return m_webView->icon();
}

void WebBrowser::setFocus(Qt::FocusReason reason) {
  m_txtLocation->setFocus(reason);
}

void WebBrowser::setupIcons() {
  m_actionZoom->setIcon(IconThemeFactory::getInstance()->fromTheme("zoom-fit-best"));
  m_actionBack->setIcon(IconThemeFactory::getInstance()->fromTheme("go-previous"));
  m_actionForward->setIcon(IconThemeFactory::getInstance()->fromTheme("go-next"));
  m_actionReload->setIcon(IconThemeFactory::getInstance()->fromTheme("view-refresh"));
  m_actionStop->setIcon(IconThemeFactory::getInstance()->fromTheme("process-stop"));
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
