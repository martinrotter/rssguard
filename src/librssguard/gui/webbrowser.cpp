// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webbrowser.h"

#include "definitions/globals.h"
#include "gui/dialogs/formmain.h"
#include "gui/reusable/baselineedit.h"
#include "gui/reusable/searchtextwidget.h"
#include "gui/tabwidget.h"
#include "gui/webviewers/webviewer.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "network-web/webfactory.h"

#include <QJsonObject>
#include <QKeyEvent>
#include <QProgressBar>
#include <QScrollBar>
#include <QTimer>
#include <QToolBar>
#include <QToolTip>
#include <QWidgetAction>

WebBrowser::WebBrowser(WebViewer* viewer, QWidget* parent)
  : TabContent(parent), m_layout(new QVBoxLayout(this)), m_toolBar(new QToolBar(tr("Navigation panel"), this)),
    m_webView(viewer), m_searchWidget(new SearchTextWidget(this)),
    m_actionOpenInSystemBrowser(new QAction(qApp->icons()->fromTheme(QSL("document-open")),
                                            tr("Open in system web browser"),
                                            this)),
    m_actionReload(new QAction(qApp->icons()->fromTheme(QSL("view-refresh")), tr("Reload"), this)),
    m_actionGoBack(new QAction(qApp->icons()->fromTheme(QSL("draw-arrow-back")), tr("Go back"), this)),
    m_actionGoForward(new QAction(qApp->icons()->fromTheme(QSL("draw-arrow-forward")), tr("Go forward"), this)),
    m_txtLocation(new BaseLineEdit(this))
#if defined(ENABLE_MEDIAPLAYER)
    ,
    m_actionPlayPageInMediaPlayer(new QAction(qApp->icons()->fromTheme(QSL("player_play"), QSL("media-playback-start")),
                                              tr("Play in media player"),
                                              this))
#endif
{
  if (m_webView == nullptr) {
    m_webView = qApp->createWebView();
    dynamic_cast<QWidget*>(m_webView)->setParent(this);
  }

  // Initialize the components and layout.
  bindWebView();

  initializeLayout();

  setFocusProxy(m_txtLocation);
  setTabOrder(m_txtLocation, m_toolBar);
  setTabOrder(m_toolBar, dynamic_cast<QWidget*>(m_webView));

  createConnections();
  reloadFontSettings();
  reloadZoomFactor();
  setNavigationVisible(true);
}

void WebBrowser::bindWebView() {
  m_webView->bindToBrowser(this);

  auto* qobj_viewer = dynamic_cast<QObject*>(m_webView);

  connect(qobj_viewer, SIGNAL(goBackEnabledChanged(bool)), m_actionGoBack, SLOT(setEnabled(bool)));
  connect(qobj_viewer, SIGNAL(goForwardEnabledChanged(bool)), m_actionGoForward, SLOT(setEnabled(bool)));
  connect(qobj_viewer, SIGNAL(reloadPageEnabledChanged(bool)), m_actionReload, SLOT(setEnabled(bool)));

  connect(qobj_viewer, SIGNAL(linkMouseHighlighted(QUrl)), this, SLOT(onLinkMouseHighlighted(QUrl)));
  connect(qobj_viewer, SIGNAL(linkMouseClicked(QUrl)), this, SLOT(onLinkMouseClicked(QUrl)));
  connect(qobj_viewer, SIGNAL(pageTitleChanged(QString)), this, SLOT(onTitleChanged(QString)));
  connect(qobj_viewer, SIGNAL(pageUrlChanged(QUrl)), this, SLOT(updateUrl(QUrl)));
  connect(qobj_viewer, SIGNAL(pageIconChanged(QIcon)), this, SLOT(onIconChanged(QIcon)));
  connect(qobj_viewer, SIGNAL(loadingStarted()), this, SLOT(onLoadingStarted()));
  connect(qobj_viewer, SIGNAL(loadingProgress(int)), this, SLOT(onLoadingProgress(int)));
  connect(qobj_viewer, SIGNAL(loadingFinished(bool)), this, SLOT(onLoadingFinished(bool)));
  connect(qobj_viewer, SIGNAL(openUrlInNewTab(bool, QUrl)), this, SLOT(onOpenUrlInNewTab(bool, QUrl)));
  connect(qobj_viewer, SIGNAL(openViewerInNewTab(WebViewer*)), this, SLOT(openViewerInNewTab(WebViewer*)));
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

  connect(m_actionReload, &QAction::triggered, this, &WebBrowser::reloadPage);
  connect(m_actionGoBack, &QAction::triggered, this, &WebBrowser::goBack);
  connect(m_actionGoForward, &QAction::triggered, this, &WebBrowser::goForward);

  connect(m_txtLocation,
          &BaseLineEdit::submitted,
          this,
          static_cast<void (WebBrowser::*)(const QString&)>(&WebBrowser::loadUrlOrSearchPhrase));

  connect(m_actionOpenInSystemBrowser, &QAction::triggered, this, &WebBrowser::openCurrentSiteInSystemBrowser);

#if defined(ENABLE_MEDIAPLAYER)
  connect(m_actionPlayPageInMediaPlayer, &QAction::triggered, this, &WebBrowser::playCurrentSiteInMediaPlayer);
#endif
}

WebBrowser::~WebBrowser() {}

void WebBrowser::updateUrl(const QUrl& url) {
  m_txtLocation->setText(url.toString());
}

void WebBrowser::loadUrl(const QUrl& url) {
  if (url.isValid()) {
    m_webView->loadUrl(url);
  }
}

void WebBrowser::reloadPage() {
  if (m_webView->url().isValid()) {
    m_webView->reloadPage();
  }
}

void WebBrowser::goForward() {
  m_webView->goForward();
}

void WebBrowser::goBack() {
  m_webView->goBack();
}

void WebBrowser::loadUrl(const QString& url) {
  return loadUrl(QUrl::fromUserInput(url));
}

double WebBrowser::verticalScrollBarPosition() const {
  return m_webView->verticalScrollBarPosition();
}

void WebBrowser::setVerticalScrollBarPosition(double pos) {
  m_webView->setVerticalScrollBarPosition(pos);
}

void WebBrowser::scrollUp() {
  setVerticalScrollBarPosition(verticalScrollBarPosition() - WEB_BROWSER_SCROLL_STEP);
}

void WebBrowser::scrollDown() {
  setVerticalScrollBarPosition(verticalScrollBarPosition() + WEB_BROWSER_SCROLL_STEP);
}

void WebBrowser::cleanupCache() {
  m_webView->cleanupCache();
}

void WebBrowser::reloadFontSettings() {
  QFont fon;
  fon.fromString(qApp->settings()->value(GROUP(Messages), SETTING(Messages::PreviewerFontStandard)).toString());
  m_webView->applyFont(fon);
}

void WebBrowser::reloadZoomFactor() {
  m_webView->setZoomFactor(qApp->settings()->value(GROUP(Messages), SETTING(Messages::Zoom)).toDouble());
}

void WebBrowser::setNavigationVisible(bool visible) {
  const bool is_supported = m_webView->supportsNavigation();

  m_actionGoBack->setVisible(is_supported && visible);
  m_actionGoForward->setVisible(is_supported && visible);
  m_actionReload->setVisible(is_supported && visible);

  m_actionTxtLocation->setVisible(visible);
  m_txtLocation->blockSignals(!visible);
}

void WebBrowser::onZoomFactorChanged() {
  auto fact = m_webView->zoomFactor();
  qApp->settings()->setValue(GROUP(Messages), Messages::Zoom, fact);
}

#if defined(ENABLE_MEDIAPLAYER)
void WebBrowser::playCurrentSiteInMediaPlayer() {
  qApp->mainForm()->tabWidget()->addMediaPlayer(m_webView->url().toString(), true);
}
#endif

void WebBrowser::clear(bool also_hide) {
  m_webView->clear();

  if (also_hide) {
    hide();
  }
}

void WebBrowser::loadUrlOrSearchPhrase(const QString& text) {
  auto url = QUrl::fromUserInput(text.trimmed());

  if (!text.startsWith(QSL("chrom")) && !text.startsWith(QSL("qrc:")) &&
      (!url.isValid() || url.host().isEmpty() || !text.contains(QL1C('.')))) {
    url = QUrl::fromUserInput(QSL("https://www.google.com/search?q=%1")
                                .arg(QString::fromUtf8(QUrl::toPercentEncoding(text.trimmed()))));
  }

  loadUrl(url);
}

void WebBrowser::setHtml(const QString& html, const QUrl& url, RootItem* root) {
  reloadZoomFactor();

  m_searchWidget->hide();
  m_webView->setHtml(html, url, root);
}

void WebBrowser::loadMessage(const Message& message, RootItem* root) {
  reloadZoomFactor();

  m_searchWidget->hide();
  m_webView->loadMessage(message, root);
}

bool WebBrowser::eventFilter(QObject* watched, QEvent* event) {
  Q_UNUSED(watched)

  if (event->type() == QEvent::Type::Wheel) {
    QWheelEvent* wh_event = static_cast<QWheelEvent*>(event);

    // Zoom with mouse.
    if (Globals::hasFlag(wh_event->modifiers(), Qt::KeyboardModifier::ControlModifier)) {
      if (wh_event->angleDelta().y() > 0 && m_webView->canZoomIn()) {
        m_webView->zoomIn();
        onZoomFactorChanged();
        return true;
      }
      else if (wh_event->angleDelta().y() < 0 && m_webView->canZoomOut()) {
        m_webView->zoomOut();
        onZoomFactorChanged();
        return true;
      }
    }
  }
  else if (event->type() == QEvent::KeyPress) {
    QKeyEvent* key_event = static_cast<QKeyEvent*>(event);

    // Find text.
    if (key_event->matches(QKeySequence::StandardKey::Find)) {
      m_searchWidget->clear();
      m_searchWidget->show();
      m_searchWidget->setFocus();
      return true;
    }

    // Hide visible search box.
    if (key_event->key() == Qt::Key::Key_Escape && m_searchWidget->isVisible()) {
      m_searchWidget->hide();
      return true;
    }

    // Zoom with keyboard.
    if ((key_event->modifiers() & Qt::KeyboardModifier::ControlModifier) > 0) {
      if (key_event->key() == Qt::Key::Key_Plus && m_webView->canZoomIn()) {
        m_webView->zoomIn();
        onZoomFactorChanged();
        return true;
      }
      else if (key_event->key() == Qt::Key::Key_Minus && m_webView->canZoomOut()) {
        m_webView->zoomOut();
        onZoomFactorChanged();
        return true;
      }
      else if (key_event->key() == Qt::Key::Key_0) {
        m_webView->setZoomFactor(1.0f);
        onZoomFactorChanged();
        return true;
      }
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
    emit titleChanged(m_index, new_title.simplified());
  }
}

void WebBrowser::onIconChanged(const QIcon& icon) {
  emit iconChanged(m_index, icon);
}

void WebBrowser::onLinkMouseHighlighted(const QUrl& url) {
  qDebugNN << LOGSEC_GUI << "Hovered link:" << QUOTE_W_SPACE_DOT(url);

  qApp->showGuiMessage(Notification::Event::GeneralEvent,
                       {url.toString(), url.toString(), QSystemTrayIcon::MessageIcon::NoIcon},
                       {false, false, true});
}

void WebBrowser::onLinkMouseClicked(const QUrl& url) {
  if (qApp->settings()->value(GROUP(Web), SETTING(Web::FollowLinks)).toBool()) {
    loadUrl(url);
  }
  else {
    qApp->web()->openUrlInExternalBrowser(url.toString(), true);

    if (qApp->settings()
          ->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally))
          .toBool()) {
      QTimer::singleShot(1000, qApp, []() {
        qApp->mainForm()->display();
      });
    }
  }
}

void WebBrowser::initializeLayout() {
  m_toolBar->setFloatable(false);
  m_toolBar->setMovable(false);
  m_toolBar->setAllowedAreas(Qt::ToolBarArea::TopToolBarArea);
  m_actionOpenInSystemBrowser->setEnabled(false);

  // Add needed actions into toolbar.
  m_toolBar->addAction(m_actionOpenInSystemBrowser);

#if defined(ENABLE_MEDIAPLAYER)
  m_actionPlayPageInMediaPlayer->setEnabled(false);
  m_toolBar->addAction(m_actionPlayPageInMediaPlayer);
#endif

  m_toolBar->addAction(m_actionGoBack);
  m_toolBar->addAction(m_actionGoForward);
  m_toolBar->addAction(m_actionReload);
  m_actionTxtLocation = m_toolBar->addWidget(m_txtLocation);
  m_txtLocation->setPlaceholderText(tr("Enter URL or search phrase here"));

  m_loadingProgress = new QProgressBar(this);
  m_loadingProgress->setFixedHeight(10);
  m_loadingProgress->setMinimum(0);
  m_loadingProgress->setTextVisible(false);
  m_loadingProgress->setMaximum(100);
  m_loadingProgress->setAttribute(Qt::WidgetAttribute::WA_TranslucentBackground);

  // Setup layout.
  m_layout->addWidget(m_toolBar);
  m_layout->addWidget(dynamic_cast<QWidget*>(m_webView));
  m_layout->addWidget(m_loadingProgress);
  m_layout->addWidget(m_searchWidget);
  m_layout->setContentsMargins({0, 0, 0, 0});
  m_layout->setSpacing(0);

  m_actionGoBack->setEnabled(false);
  m_actionGoForward->setEnabled(false);
  m_actionReload->setEnabled(false);

  m_searchWidget->hide();
  m_loadingProgress->hide();
}

void WebBrowser::onLoadingStarted() {
  m_loadingProgress->show();
  m_actionOpenInSystemBrowser->setEnabled(false);

#if defined(ENABLE_MEDIAPLAYER)
  m_actionPlayPageInMediaPlayer->setEnabled(false);
#endif
}

void WebBrowser::onLoadingProgress(int progress) {
  m_loadingProgress->setMaximum(progress < 0 ? 0 : 100);
  m_loadingProgress->setValue(progress < 0 ? 0 : progress);
}

void WebBrowser::onLoadingFinished(bool success) {
  if (success) {
    auto url = m_webView->url();

    if (url.isValid() && !url.host().isEmpty()) {
      m_actionOpenInSystemBrowser->setEnabled(true);

#if defined(ENABLE_MEDIAPLAYER)
      m_actionPlayPageInMediaPlayer->setEnabled(true);
#endif
    }
    else {
      m_actionOpenInSystemBrowser->setEnabled(false);

#if defined(ENABLE_MEDIAPLAYER)
      m_actionPlayPageInMediaPlayer->setEnabled(false);
#endif
    }
  }

  m_loadingProgress->hide();
  m_loadingProgress->setValue(0);
}

void WebBrowser::onOpenUrlInNewTab(bool open_externally, const QUrl& url) {
  if (open_externally) {
    qApp->web()->openUrlInExternalBrowser(url, true);
  }
  else {
    qApp->mainForm()->tabWidget()->addLinkedBrowser(url);
  }
}

void WebBrowser::openViewerInNewTab(WebViewer* viewer) {
  qApp->mainForm()->tabWidget()->addBrowser(false, false, new WebBrowser(viewer, this));
}
