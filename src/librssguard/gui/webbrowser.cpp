// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webbrowser.h"

#include "database/databasequeries.h"
#include "gui/messagebox.h"
#include "gui/reusable/discoverfeedsbutton.h"
#include "gui/reusable/locationlineedit.h"
#include "gui/reusable/searchtextwidget.h"
#include "gui/webviewer.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"
#include "services/abstract/serviceroot.h"

#include <QKeyEvent>
#include <QScrollBar>
#include <QTimer>
#include <QToolBar>
#include <QToolTip>
#include <QWebEngineSettings>
#include <QWidgetAction>

WebBrowser::WebBrowser(QWidget* parent) : TabContent(parent),
  m_layout(new QVBoxLayout(this)),
  m_toolBar(new QToolBar(tr("Navigation panel"), this)),
  m_webView(new WebViewer(this)),
  m_searchWidget(new SearchTextWidget(this)),
  m_txtLocation(new LocationLineEdit(this)),
  m_btnDiscoverFeeds(new DiscoverFeedsButton(this)),
  m_actionBack(m_webView->pageAction(QWebEnginePage::WebAction::Back)),
  m_actionForward(m_webView->pageAction(QWebEnginePage::WebAction::Forward)),
  m_actionReload(m_webView->pageAction(QWebEnginePage::WebAction::Reload)),
  m_actionStop(m_webView->pageAction(QWebEnginePage::WebAction::Stop)),
  m_actionOpenInSystemBrowser(new QAction(qApp->icons()->fromTheme(QSL("document-open")),
                                          tr("Open this website in system web browser"),
                                          this)) {
  // Initialize the components and layout.
  initializeLayout();
  setFocusProxy(m_txtLocation);
  setTabOrder(m_txtLocation, m_toolBar);
  setTabOrder(m_toolBar, m_webView);
  createConnections();
  reloadFontSettings();
}

void WebBrowser::createConnections() {
  installEventFilter(this);

  connect(m_searchWidget, &SearchTextWidget::searchCancelled, this, [this]() {
    m_webView->findText(QString());
  });
  connect(m_searchWidget, &SearchTextWidget::searchForText, this, [this](const QString& text, bool backwards) {
    if (backwards) {
      m_webView->findText(text, QWebEnginePage::FindFlag::FindBackward);
    }
    else {
      m_webView->findText(text);
    }

    m_searchWidget->setFocus();
  });

  connect(m_actionOpenInSystemBrowser, &QAction::triggered, this, &WebBrowser::openCurrentSiteInSystemBrowser);

  connect(m_txtLocation, &LocationLineEdit::submitted,
          this, static_cast<void (WebBrowser::*)(const QString&)>(&WebBrowser::loadUrl));
  connect(m_webView, &WebViewer::urlChanged, this, &WebBrowser::updateUrl);

  // Change location textbox status according to webpage status.
  connect(m_webView, &WebViewer::loadStarted, this, &WebBrowser::onLoadingStarted);
  connect(m_webView, &WebViewer::loadProgress, this, &WebBrowser::onLoadingProgress);
  connect(m_webView, &WebViewer::loadFinished, this, &WebBrowser::onLoadingFinished);

  // Forward title/icon changes.
  connect(m_webView, &WebViewer::titleChanged, this, &WebBrowser::onTitleChanged);
  connect(m_webView, &WebViewer::iconChanged, this, &WebBrowser::onIconChanged);

  connect(m_webView->page(), &WebPage::windowCloseRequested, this, &WebBrowser::closeRequested);
}

void WebBrowser::updateUrl(const QUrl& url) {
  m_txtLocation->setText(url.toString());
}

void WebBrowser::loadUrl(const QUrl& url) {
  if (url.isValid()) {
    m_webView->load(url);
  }
}

WebBrowser::~WebBrowser() {
  // Delete members. Do not use scoped pointers here.
  delete m_layout;
}

double WebBrowser::verticalScrollBarPosition() const {
  double position;
  QEventLoop loop;

  viewer()->page()->runJavaScript(QSL("window.pageYOffset;"), [&position, &loop](const QVariant& val) {
    position = val.toDouble();
    loop.exit();
  });
  loop.exec();

  return position;
}

void WebBrowser::setVerticalScrollBarPosition(double pos) {
  viewer()->page()->runJavaScript(QSL("window.scrollTo(0, %1);").arg(pos));
}

void WebBrowser::reloadFontSettings() {
  QFont fon;

  fon.fromString(qApp->settings()->value(GROUP(Messages),
                                         SETTING(Messages::PreviewerFontStandard)).toString());

  QWebEngineSettings::defaultSettings()->setFontFamily(QWebEngineSettings::FontFamily::StandardFont, fon.family());
  QWebEngineSettings::defaultSettings()->setFontFamily(QWebEngineSettings::FontFamily::SerifFont, fon.family());
  QWebEngineSettings::defaultSettings()->setFontFamily(QWebEngineSettings::FontFamily::SansSerifFont, fon.family());
  QWebEngineSettings::defaultSettings()->setFontSize(QWebEngineSettings::DefaultFontSize, fon.pointSize());
}

void WebBrowser::increaseZoom() {
  m_webView->increaseWebPageZoom();
}

void WebBrowser::decreaseZoom() {
  m_webView->decreaseWebPageZoom();
}

void WebBrowser::resetZoom() {
  m_webView->resetWebPageZoom(true);
}

void WebBrowser::clear(bool also_hide) {
  m_webView->clear();
  m_messages.clear();

  if (also_hide) {
    hide();
  }
}

void WebBrowser::loadUrl(const QString& url) {
  return loadUrl(QUrl::fromUserInput(url));
}

void WebBrowser::loadMessages(const QList<Message>& messages, RootItem* root) {
  m_messages = messages;
  m_root = root;

  if (!m_root.isNull()) {
    m_searchWidget->hide();
    m_webView->loadMessages(messages, root);
    show();
  }
}

void WebBrowser::loadMessage(const Message& message, RootItem* root) {
  loadMessages(QList<Message>() << message, root);
}

bool WebBrowser::eventFilter(QObject* watched, QEvent* event) {
  Q_UNUSED(watched)

  if (event->type() == QEvent::KeyPress) {
    QKeyEvent* key_event = static_cast<QKeyEvent*>(event);

    if (key_event->matches(QKeySequence::StandardKey::Find)) {
      m_searchWidget->clear();
      m_searchWidget->show();
      m_searchWidget->setFocus();
      return true;
    }
  }

  return false;
}

void WebBrowser::openCurrentSiteInSystemBrowser() {
  auto url = m_webView->url();

  if (!url.isValid() || url.host().contains(QSL(APP_LOW_NAME))) {
    return;
  }

  qApp->web()->openUrlInExternalBrowser(url.toString());
}

void WebBrowser::onTitleChanged(const QString& new_title) {
  if (new_title.isEmpty()) {
    //: Webbrowser tab title when no title is available.
    emit titleChanged(m_index, tr("No title"));
  }
  else {
    emit titleChanged(m_index, new_title);
  }
}

void WebBrowser::onIconChanged(const QIcon& icon) {
  emit iconChanged(m_index, icon);
}

void WebBrowser::initializeLayout() {
  m_toolBar->setFloatable(false);
  m_toolBar->setMovable(false);
  m_toolBar->setAllowedAreas(Qt::ToolBarArea::TopToolBarArea);

  // Modify action texts.
  m_actionBack->setText(tr("Back"));
  m_actionForward->setText(tr("Forward"));
  m_actionReload->setText(tr("Reload"));
  m_actionStop->setText(tr("Stop"));
  QWidgetAction* act_discover = new QWidgetAction(this);

  m_actionOpenInSystemBrowser->setEnabled(false);
  act_discover->setDefaultWidget(m_btnDiscoverFeeds);

  // Add needed actions into toolbar.
  m_toolBar->addAction(m_actionBack);
  m_toolBar->addAction(m_actionForward);
  m_toolBar->addAction(m_actionReload);
  m_toolBar->addAction(m_actionStop);
  m_toolBar->addAction(m_actionOpenInSystemBrowser);
  m_toolBar->addAction(act_discover);
  m_toolBar->addWidget(m_txtLocation);
  m_loadingProgress = new QProgressBar(this);
  m_loadingProgress->setFixedHeight(5);
  m_loadingProgress->setMinimum(0);
  m_loadingProgress->setTextVisible(false);
  m_loadingProgress->setMaximum(100);
  m_loadingProgress->setAttribute(Qt::WidgetAttribute::WA_TranslucentBackground);

  // Setup layout.
  m_layout->addWidget(m_toolBar);
  m_layout->addWidget(m_webView);
  m_layout->addWidget(m_loadingProgress);
  m_layout->addWidget(m_searchWidget);
  m_layout->setMargin(0);
  m_layout->setSpacing(0);

  m_searchWidget->hide();
}

void WebBrowser::onLoadingStarted() {
  m_btnDiscoverFeeds->clearFeedAddresses();
  m_loadingProgress->show();
  m_actionOpenInSystemBrowser->setEnabled(false);
}

void WebBrowser::onLoadingProgress(int progress) {
  m_loadingProgress->setValue(progress);
}

void WebBrowser::onLoadingFinished(bool success) {
  if (success) {
    auto url = m_webView->url();

    if (url.isValid() && !url.host().contains(QSL(APP_LOW_NAME))) {
      m_actionOpenInSystemBrowser->setEnabled(true);
    }

    // Let's check if there are any feeds defined on the web and eventually
    // display "Add feeds" button.
    m_webView->page()->toHtml([this](const QString& result) {
      this->m_btnDiscoverFeeds->setFeedAddresses(NetworkFactory::extractFeedLinksFromHtmlPage(m_webView->url(), result));
    });
  }
  else {
    m_btnDiscoverFeeds->clearFeedAddresses();
  }

  m_loadingProgress->hide();
  m_loadingProgress->setValue(0);
}

Message* WebBrowser::findMessage(int id) {
  for (int i = 0; i < m_messages.size(); i++) {
    if (m_messages.at(i).m_id == id) {
      return &m_messages[i];
    }
  }

  return nullptr;
}
