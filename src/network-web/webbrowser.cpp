// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "network-web/webbrowser.h"

#include "definitions/definitions.h"
#include "network-web/webview.h"
#include "network-web/networkfactory.h"
#include "miscellaneous/skinfactory.h"
#include "gui/dialogs/formmain.h"
#include "gui/tabwidget.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "services/standard/standardserviceroot.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QWebEnginePage>
#include <QWidgetAction>
#include <QSlider>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include <QClipboard>


QList<WebBrowser*> WebBrowser::m_runningWebBrowsers;

WebBrowser::WebBrowser(QWidget *parent)
  : TabContent(parent),
    m_layout(new QVBoxLayout(this)),
    m_toolBar(new QToolBar(tr("Navigation panel"), this)),
    m_webView(new WebView(this)),
    m_txtLocation(new LocationLineEdit(this)),
    m_actionBack(m_webView->pageAction(QWebEnginePage::Back)),
    m_actionForward(m_webView->pageAction(QWebEnginePage::Forward)),
    m_actionReload(m_webView->pageAction(QWebEnginePage::Reload)),
    m_actionStop(m_webView->pageAction(QWebEnginePage::Stop)) {
  // Add this new instance to the global list of web browsers.
  // NOTE: This is used primarily for dynamic icon theme switching.
  m_runningWebBrowsers.append(this);

  // Initialize the components and layout.
  initializeLayout();

  setTabOrder(m_txtLocation, m_toolBar);
  setTabOrder(m_toolBar, m_webView);

  createConnections();
  initializeZoomWidget();
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
  button_decrease->setText(QSL("-"));
  button_decrease->setToolTip(tr("Decrease zoom."));
  m_btnResetZoom->setText(QSL("100%"));
  m_btnResetZoom->setToolTip(tr("Reset zoom to default."));
  button_increase->setText(QSL("+"));
  button_increase->setToolTip(tr("Increase zoom."));

  // Setup layout.
  layout->addWidget(zoom_label);
  layout->addWidget(button_decrease);
  layout->addWidget(m_btnResetZoom);
  layout->addWidget(button_increase);
  layout->setSpacing(2);
  layout->setMargin(3);
  m_zoomButtons->setLayout(layout);

  // Make connections.
  connect(button_increase, SIGNAL(clicked()), this, SLOT(increaseZoom()));
  connect(button_decrease, SIGNAL(clicked()), this, SLOT(decreaseZoom()));
  connect(m_btnResetZoom, SIGNAL(clicked()), this, SLOT(resetZoom()));

  m_actionZoom = new QWidgetAction(this);
  m_actionZoom->setDefaultWidget(m_zoomButtons);
}

void WebBrowser::initializeLayout() {
  m_toolBar->setFloatable(false);
  m_toolBar->setMovable(false);
  m_toolBar->setAllowedAreas(Qt::TopToolBarArea);

  // Modify action texts.
  m_actionBack->setText(tr("Back"));
  m_actionBack->setToolTip(tr("Go back."));
  m_actionForward->setText(tr("Forward"));
  m_actionForward->setToolTip(tr("Go forward."));
  m_actionReload->setText(tr("Reload"));
  m_actionReload->setToolTip(tr("Reload current web page."));
  m_actionStop->setText(tr("Stop"));
  m_actionStop->setToolTip(tr("Stop web page loading."));

  m_btnDiscoverFeeds = new DiscoverFeedsButton(this);

  QWidgetAction *act_discover = new QWidgetAction(this);

  act_discover->setDefaultWidget(m_btnDiscoverFeeds);

  // Add needed actions into toolbar.
  m_toolBar->addAction(m_actionBack);
  m_toolBar->addAction(m_actionForward);
  m_toolBar->addAction(m_actionReload);
  m_toolBar->addAction(m_actionStop);
  m_toolBar->addAction(act_discover);
  m_toolBar->addWidget(m_txtLocation);

  m_loadingWidget = new QWidget(this);
  m_loadingWidget->setFixedHeight(15);

  // Initialize dynamic progress bar which will be displayed
  // at the bottom of web browser.
  m_lblProgress = new QLabel(this);
  m_loadingProgress = new QProgressBar(this);
  m_loadingProgress->setFixedHeight(15);
  m_loadingProgress->setMinimum(0);
  m_loadingProgress->setTextVisible(false);
  m_loadingProgress->setMaximum(100);
  m_loadingProgress->setAttribute(Qt::WA_TranslucentBackground);

  m_loadingLayout = new QHBoxLayout();
  m_loadingLayout->setMargin(0);
  m_loadingLayout->addWidget(m_lblProgress, 0, Qt::AlignVCenter);
  m_loadingLayout->addWidget(m_loadingProgress, 1, Qt::AlignVCenter);
  m_loadingWidget->setLayout(m_loadingLayout);

  // Setup layout.
  m_layout->addWidget(m_toolBar);
  m_layout->addWidget(m_webView);
  m_layout->addWidget(m_loadingWidget);
  m_layout->setMargin(0);
  m_layout->setSpacing(0);

  m_loadingWidget->hide();
}

void WebBrowser::onLoadingStarted() {
  m_loadingProgress->setValue(0);
  m_loadingWidget->show();
  m_btnDiscoverFeeds->setEnabled(false);
}

void WebBrowser::onLoadingProgress(int progress) {
  m_loadingProgress->setValue(progress);
}

void WebBrowser::onLoadingFinished(bool success) {
  if (success) {
    m_webView->page()->toHtml([this](const QString &html) {
      this->m_btnDiscoverFeeds->setFeedAddresses(NetworkFactory::extractFeedLinksFromHtmlPage(m_webView->url(), html));
    });
  }
  else {
    m_btnDiscoverFeeds->clearFeedAddresses();
  }

  m_loadingWidget->hide();
}

void WebBrowser::createConnections() {
  // When user confirms new url, then redirect to it.
  connect(m_txtLocation,SIGNAL(submitted(QString)), this, SLOT(navigateToUrl(QString)));
  // If new page loads, then update current url.
  connect(m_webView, SIGNAL(urlChanged(QUrl)), this, SLOT(updateUrl(QUrl)));

  // Connect this WebBrowser to global TabWidget.
  TabWidget *tab_widget = qApp->mainForm()->tabWidget();

  // Change location textbox status according to webpage status.
  connect(m_webView, SIGNAL(loadStarted()), this, SLOT(onLoadingStarted()));
  connect(m_webView, SIGNAL(loadProgress(int)), this, SLOT(onLoadingProgress(int)));
  connect(m_webView, SIGNAL(loadFinished(bool)), this, SLOT(onLoadingFinished(bool)));

  // Forward title/icon changes.
  connect(m_webView, SIGNAL(titleChanged(QString)), this, SLOT(onTitleChanged(QString)));
  connect(m_webView, SIGNAL(iconUrlChanged(QUrl)), this, SLOT(onIconChanged()));

  // Misc connections.
  connect(m_webView, SIGNAL(zoomFactorChanged()), this, SLOT(updateZoomGui()));
}

void WebBrowser::onIconChanged() {
  emit iconChanged(m_index, icon());
}

void WebBrowser::onTitleChanged(const QString &new_title) {
  if (new_title.isEmpty()) {
    //: Webbrowser tab title when no title is available.
    emit titleChanged(m_index, tr("No title"));
  }
  else {
    emit titleChanged(m_index, new_title);
  }
}

void WebBrowser::updateUrl(const QUrl &url) {
  QString url_string = url.toString();

  m_txtLocation->setText(url_string);
  setNavigationBarVisible(url_string != INTERNAL_URL_EMPTY && url_string != INTERNAL_URL_NEWSPAPER);
}

void WebBrowser::navigateToUrl(const QUrl &url) {
  if (url.isValid()) {
    m_webView->load(url);
  }
}

void WebBrowser::navigateToMessages(const QList<Message> &messages) {
  Skin skin = qApp->skins()->currentSkin();
  QString messages_layout;
  QString single_message_layout = skin.m_layoutMarkup;

  foreach (const Message &message, messages) {
    QString enclosures;

    foreach (const Enclosure &enclosure, message.m_enclosures) {
      enclosures += skin.m_enclosureMarkup.arg(enclosure.m_url);

      if (!enclosure.m_mimeType.isEmpty()) {
        enclosures += QL1S(" [") + enclosure.m_mimeType + QL1S("]");
      }

      enclosures += QL1S("<br>");
    }

    if (!enclosures.isEmpty()) {
      enclosures = enclosures.prepend(QSL("<br>"));
    }

    messages_layout.append(single_message_layout.arg(message.m_title,
                                                     tr("Written by ") + (message.m_author.isEmpty() ?
                                                                            tr("unknown author") :
                                                                            message.m_author),
                                                     message.m_url,
                                                     message.m_contents,
                                                     message.m_created.toString(Qt::DefaultLocaleShortDate),
                                                     enclosures));
  }

  QString layout_wrapper = skin.m_layoutMarkupWrapper.arg(messages.size() == 1 ? messages.at(0).m_title : tr("Newspaper view"), messages_layout);

  m_webView->setHtml(layout_wrapper, QUrl(INTERNAL_URL_NEWSPAPER));
  emit iconChanged(m_index, qApp->icons()->fromTheme(QSL("item-newspaper")));
}

void WebBrowser::updateZoomGui() {
  m_btnResetZoom->setText(QString(QSL("%1%")).arg(QString::number(m_webView->zoomFactor() * 100, 'f', 0)));
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
  better_url = better_url.replace(QL1C('\\'), QL1C('/'));

  navigateToUrl(QUrl::fromUserInput(better_url));
}

WebBrowser::~WebBrowser() {
  qDebug("Destroying WebBrowser instance.");

  // Remove this instance from the global list of web browsers.
  m_runningWebBrowsers.removeAll(this);

  // Delete members. Do not use scoped pointers here.
  delete m_layout;
  delete m_zoomButtons;
  delete m_actionZoom;
}

void WebBrowser::setupIcons() {
  m_actionBack->setIcon(qApp->icons()->fromTheme(QSL("go-previous")));
  m_actionForward->setIcon(qApp->icons()->fromTheme(QSL("go-next")));
  m_actionReload->setIcon(qApp->icons()->fromTheme(QSL("go-refresh")));
  m_actionStop->setIcon(qApp->icons()->fromTheme(QSL("go-stop")));
}

QIcon WebBrowser::icon() const {
  QUrl url = m_webView->iconUrl();

  if (url.isValid()) {
    QByteArray output;
    if (NetworkFactory::downloadFile(url.toString(), DOWNLOAD_TIMEOUT, output).first == QNetworkReply::NoError) {
      QPixmap icon_pixmap;
      icon_pixmap.loadFromData(output);
      return QIcon(icon_pixmap);
    }
  }

  return QIcon();
}
