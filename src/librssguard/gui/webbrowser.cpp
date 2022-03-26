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
#include "network-web/readability.h"
#include "network-web/webfactory.h"
#include "services/abstract/serviceroot.h"

#include <QKeyEvent>
#include <QScrollBar>
#include <QTimer>
#include <QToolBar>
#include <QToolTip>
#include <QWidgetAction>

#if defined(USE_WEBENGINE)
#include "gui/webengine/webengineviewer.h" // WebEngine-based web browsing.
#else
#include "gui/litehtml/litehtmlviewer.h" // QLiteHtml-based web browsing.
#endif

WebBrowser::WebBrowser(QWidget* parent) : TabContent(parent),
  m_layout(new QVBoxLayout(this)),
  m_toolBar(new QToolBar(tr("Navigation panel"), this)),
#if defined(USE_WEBENGINE)
  m_webView(new WebEngineViewer(this)),
#else
  m_webView(new LiteHtmlViewer(this)),
#endif
  m_searchWidget(new SearchTextWidget(this)),
  m_txtLocation(new LocationLineEdit(this)),
  m_btnDiscoverFeeds(new DiscoverFeedsButton(this)),
  m_actionOpenInSystemBrowser(new QAction(qApp->icons()->fromTheme(QSL("document-open")),
                                          tr("Open this website in system web browser"),
                                          this)),
  m_actionReadabilePage(new QAction(qApp->icons()->fromTheme(QSL("text-html")),
                                    tr("View website in reader mode"),
                                    this)) {
  // Initialize the components and layout.
  m_webView->bindToBrowser(this);
  m_webView->setZoomFactor(qApp->settings()->value(GROUP(Messages), SETTING(Messages::Zoom)).toDouble());

  initializeLayout();

  setFocusProxy(m_txtLocation);
  setTabOrder(m_txtLocation, m_toolBar);
  setTabOrder(m_toolBar, dynamic_cast<QWidget*>(m_webView));

  createConnections();
  reloadFontSettings();
}

void WebBrowser::createConnections() {
  installEventFilter(this);

  connect(m_searchWidget, &SearchTextWidget::searchCancelled, this, [this]() {
    m_webView->findText(QString(), {});
  });
  connect(m_searchWidget, &SearchTextWidget::searchForText, this, [this](const QString& text, bool backwards) {
    m_webView->findText(text, backwards);
    m_searchWidget->setFocus();
  });

  connect(m_actionOpenInSystemBrowser, &QAction::triggered, this, &WebBrowser::openCurrentSiteInSystemBrowser);
  connect(m_actionReadabilePage, &QAction::triggered, this, &WebBrowser::readabilePage);

  connect(m_txtLocation, &LocationLineEdit::submitted,
          this, static_cast<void (WebBrowser::*)(const QString&)>(&WebBrowser::loadUrl));

  connect(qApp->web()->readability(), &Readability::htmlReadabled, this, &WebBrowser::setReadabledHtml);
  connect(qApp->web()->readability(), &Readability::errorOnHtmlReadabiliting, this, &WebBrowser::readabilityFailed);
}

void WebBrowser::updateUrl(const QUrl& url) {
  m_txtLocation->setText(url.toString());
}

void WebBrowser::loadUrl(const QUrl& url) {
  if (url.isValid()) {
    m_webView->setUrl(url);
  }
}

WebBrowser::~WebBrowser() {
  // Delete members. Do not use scoped pointers here.
  delete m_layout;
}

double WebBrowser::verticalScrollBarPosition() const {
  return m_webView->verticalScrollBarPosition();
}

void WebBrowser::setVerticalScrollBarPosition(double pos) {
  m_webView->setVerticalScrollBarPosition(pos);
}

void WebBrowser::reloadFontSettings() {
  QFont fon;

  fon.fromString(qApp->settings()->value(GROUP(Messages),
                                         SETTING(Messages::PreviewerFontStandard)).toString());

  m_webView->reloadFontSettings(fon);
}

void WebBrowser::onZoomFactorChanged() {
  qApp->settings()->setValue(GROUP(Messages), Messages::Zoom, m_webView->zoomFactor());
}

void WebBrowser::increaseZoom() {
  if (m_webView->canZoomIn()) {
    m_webView->zoomIn();

    qApp->settings()->setValue(GROUP(Messages), Messages::Zoom, m_webView->zoomFactor());
  }
}

void WebBrowser::decreaseZoom() {
  if (m_webView->canZoomOut()) {
    m_webView->zoomOut();

    qApp->settings()->setValue(GROUP(Messages), Messages::Zoom, m_webView->zoomFactor());
  }
}

void WebBrowser::resetZoom() {
  m_webView->setZoomFactor(1.0f);

  qApp->settings()->setValue(GROUP(Messages), Messages::Zoom, m_webView->zoomFactor());
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

  setNavigationBarVisible(m_toolBar->isVisible() && m_messages.size() <= 1);

  if (!m_root.isNull()) {
    m_searchWidget->hide();
    m_webView->loadMessages(messages, root);
    show();
  }
}

void WebBrowser::loadMessage(const Message& message, RootItem* root) {
  loadMessages({ message }, root);
}

void WebBrowser::readabilePage() {
  m_actionReadabilePage->setEnabled(false);
  qApp->web()->readability()->makeHtmlReadable(m_webView->html(), m_webView->url().toString());
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

void WebBrowser::onLinkHovered(const QString& url) {
  qDebugNN << LOGSEC_GUI << "Hovered link:" << QUOTE_W_SPACE_DOT(url);

  qApp->showGuiMessage(Notification::Event::GeneralEvent,
                       { url, url, QSystemTrayIcon::MessageIcon::NoIcon },
                       { false, false, true });
}

void WebBrowser::setReadabledHtml(const QString& better_html) {
  if (!better_html.isEmpty()) {
    m_webView->setHtml(better_html, m_webView->url());
  }
}

void WebBrowser::readabilityFailed(const QString& error) {
  MsgBox::show({}, QMessageBox::Icon::Critical,
               tr("Reader mode failed for this website"),
               tr("Reader mode cannot be applied to current page."),
               {},
               error);
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

  m_actionBack->setIcon(qApp->icons()->fromTheme(QSL("go-previous")));
  m_actionForward->setIcon(qApp->icons()->fromTheme(QSL("go-next")));
  m_actionReload->setIcon(qApp->icons()->fromTheme(QSL("reload"), QSL("view-refresh")));
  m_actionStop->setIcon(qApp->icons()->fromTheme(QSL("process-stop")));

  QWidgetAction* act_discover = new QWidgetAction(this);

  m_actionOpenInSystemBrowser->setEnabled(false);
  m_actionReadabilePage->setEnabled(false);

  act_discover->setDefaultWidget(m_btnDiscoverFeeds);

  // Add needed actions into toolbar.
  m_toolBar->addAction(m_actionBack);
  m_toolBar->addAction(m_actionForward);
  m_toolBar->addAction(m_actionReload);
  m_toolBar->addAction(m_actionStop);
  m_toolBar->addAction(m_actionOpenInSystemBrowser);
  m_toolBar->addAction(m_actionReadabilePage);
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
  m_layout->addWidget(dynamic_cast<QWidget*>(m_webView));
  m_layout->addWidget(m_loadingProgress);
  m_layout->addWidget(m_searchWidget);
  m_layout->setContentsMargins({ 0, 0, 0, 0 });
  m_layout->setSpacing(0);

  m_searchWidget->hide();
}

void WebBrowser::onLoadingStarted() {
  m_btnDiscoverFeeds->clearFeedAddresses();
  m_loadingProgress->show();
  m_actionOpenInSystemBrowser->setEnabled(false);
  m_actionReadabilePage->setEnabled(false);
}

void WebBrowser::onLoadingProgress(int progress) {
  m_loadingProgress->setValue(progress);
}

void WebBrowser::onLoadingFinished(bool success) {
  if (success) {
    auto url = m_webView->url();

    if (url.isValid() && !url.host().isEmpty()) {
      m_actionOpenInSystemBrowser->setEnabled(true);
      m_actionReadabilePage->setEnabled(true);
    }
    else {
      m_actionOpenInSystemBrowser->setEnabled(false);
      m_actionReadabilePage->setEnabled(false);
    }

    // Let's check if there are any feeds defined on the web and eventually
    // display "Add feeds" button.
    m_btnDiscoverFeeds->setFeedAddresses(NetworkFactory::extractFeedLinksFromHtmlPage(m_webView->url(), m_webView->html()));
  }
  else {
    m_btnDiscoverFeeds->clearFeedAddresses();
  }

  m_loadingProgress->hide();
  m_loadingProgress->setValue(0);
}
