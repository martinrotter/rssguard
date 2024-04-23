// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webbrowser.h"

#include "definitions/globals.h"
#include "gui/dialogs/formmain.h"
#include "gui/messagebox.h"
#include "gui/reusable/locationlineedit.h"
#include "gui/reusable/searchtextwidget.h"
#include "gui/tabwidget.h"
#include "gui/webviewers/webviewer.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "network-web/articleparse.h"
#include "network-web/networkfactory.h"
#include "network-web/readability.h"
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
    m_webView(viewer), m_searchWidget(new SearchTextWidget(this)), m_txtLocation(new LocationLineEdit(this)),
    m_actionOpenInSystemBrowser(new QAction(qApp->icons()->fromTheme(QSL("document-open")),
                                            tr("Open this website in system web browser"),
                                            this)),
#if defined(ENABLE_MEDIAPLAYER)
    m_actionPlayPageInMediaPlayer(new QAction(qApp->icons()->fromTheme(QSL("player_play"), QSL("media-playback-start")),
                                              tr("Play in media player"),
                                              this)),
#endif
    m_actionReadabilePage(new QAction(qApp->icons()->fromTheme(QSL("text-html")),
                                      tr("View website in reader mode"),
                                      this)),
    m_actionGetFullArticle(new QAction(qApp->icons()->fromTheme(QSL("applications-office")),
                                       tr("Load full source article"),
                                       this)) {
  if (m_webView == nullptr) {
    m_webView = qApp->createWebView();
    dynamic_cast<QWidget*>(m_webView)->setParent(this);
  }

  // Initialize the components and layout.
  bindWebView();

  m_webView->setZoomFactor(qApp->settings()->value(GROUP(Messages), SETTING(Messages::Zoom)).toDouble());

  initializeLayout();

  setFocusProxy(m_txtLocation);
  setTabOrder(m_txtLocation, m_toolBar);
  setTabOrder(m_toolBar, dynamic_cast<QWidget*>(m_webView));

  createConnections();
  reloadFontSettings();
}

void WebBrowser::bindWebView() {
  m_webView->bindToBrowser(this);

  auto* qobj_viewer = dynamic_cast<QObject*>(m_webView);

  connect(qobj_viewer, SIGNAL(linkMouseHighlighted(QUrl)), this, SLOT(onLinkHovered(QUrl)));
  connect(qobj_viewer, SIGNAL(pageTitleChanged(QString)), this, SLOT(onTitleChanged(QString)));
  connect(qobj_viewer, SIGNAL(pageUrlChanged(QUrl)), this, SLOT(updateUrl(QUrl)));
  connect(qobj_viewer, SIGNAL(pageIconChanged(QIcon)), this, SLOT(onIconChanged(QIcon)));
  connect(qobj_viewer, SIGNAL(loadingStarted()), this, SLOT(onLoadingStarted()));
  connect(qobj_viewer, SIGNAL(loadingProgress(int)), this, SLOT(onLoadingProgress(int)));
  connect(qobj_viewer, SIGNAL(loadingFinished(bool)), this, SLOT(onLoadingFinished(bool)));
  connect(qobj_viewer, SIGNAL(newWindowRequested(WebViewer*)), this, SLOT(newWindowRequested(WebViewer*)));
  connect(qobj_viewer, SIGNAL(closeWindowRequested()), this, SIGNAL(windowCloseRequested()));
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
  connect(m_actionGetFullArticle, &QAction::triggered, this, &WebBrowser::getFullArticle);

#if defined(ENABLE_MEDIAPLAYER)
  connect(m_actionPlayPageInMediaPlayer, &QAction::triggered, this, &WebBrowser::playCurrentSiteInMediaPlayer);
#endif

  connect(m_txtLocation,
          &LocationLineEdit::submitted,
          this,
          static_cast<void (WebBrowser::*)(const QString&)>(&WebBrowser::loadUrl));

  connect(qApp->web()->readability(), &Readability::htmlReadabled, this, &WebBrowser::setReadabledHtml);
  connect(qApp->web()->readability(), &Readability::errorOnHtmlReadabiliting, this, &WebBrowser::readabilityFailed);

  connect(qApp->web()->articleParse(), &ArticleParse::articleParsed, this, &WebBrowser::setFullArticleHtml);
  connect(qApp->web()->articleParse(), &ArticleParse::errorOnArticlePArsing, this, &WebBrowser::fullArticleFailed);
}

void WebBrowser::updateUrl(const QUrl& url) {
  m_txtLocation->setText(url.toString());
}

void WebBrowser::loadUrl(const QUrl& url) {
  if (url.isValid()) {
    m_webView->setUrl(url);
  }
}

WebBrowser::~WebBrowser() {}

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

void WebBrowser::reloadFontSettings() {
  QFont fon;

  fon.fromString(qApp->settings()->value(GROUP(Messages), SETTING(Messages::PreviewerFontStandard)).toString());

  m_webView->applyFont(fon);
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
  m_messages.clear();

  if (also_hide) {
    hide();
  }
}

void WebBrowser::loadUrl(const QString& url) {
  return loadUrl(QUrl::fromUserInput(url));
}

void WebBrowser::setHtml(const QString& html, const QUrl& base_url) {
  m_webView->setHtml(html, base_url);
}

void WebBrowser::loadMessages(const QList<Message>& messages, RootItem* root) {
  m_messages = messages;
  m_root = root;

  setNavigationBarVisible(m_toolBar->isVisible() && m_messages.size() <= 1);

  if (!m_root.isNull()) {
    m_searchWidget->hide();
    m_webView->loadMessages(messages, root);
  }
}

void WebBrowser::readabilePage() {
  m_actionReadabilePage->setEnabled(false);
  qApp->web()->readability()->makeHtmlReadable(this, m_webView->html(), m_webView->url().toString());
}

void WebBrowser::getFullArticle() {
  QString url;

  if (!m_messages.isEmpty() && !m_messages.first().m_url.isEmpty()) {
    url = m_messages.first().m_url;
  }
  else if (m_webView->url().isValid()) {
    url = m_webView->url().toString();
  }
  else {
    return;
  }

  m_actionGetFullArticle->setEnabled(false);
  qApp->web()->articleParse()->parseArticle(this, url);
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

void WebBrowser::onLinkHovered(const QUrl& url) {
  qDebugNN << LOGSEC_GUI << "Hovered link:" << QUOTE_W_SPACE_DOT(url);

  qApp->showGuiMessage(Notification::Event::GeneralEvent,
                       {url.toString(), url.toString(), QSystemTrayIcon::MessageIcon::NoIcon},
                       {false, false, true});
}

void WebBrowser::newWindowRequested(WebViewer* viewer) {
  WebBrowser* browser = new WebBrowser(viewer, this);

  qApp->mainForm()->tabWidget()->addBrowser(false, false, browser);
}

void WebBrowser::setReadabledHtml(QObject* sndr, const QString& better_html) {
  if (sndr == this && !better_html.isEmpty()) {
    m_webView->setReadabledHtml(better_html, m_webView->url());
  }
}

void WebBrowser::readabilityFailed(QObject* sndr, const QString& error) {
  if (sndr == this && !error.isEmpty()) {
    m_webView->setReadabledHtml(error, m_webView->url());
  }
}

void WebBrowser::setFullArticleHtml(QObject* sndr, const QString& json_answer) {
  if (sndr == this && !json_answer.isEmpty()) {
    QJsonDocument json_doc = QJsonDocument::fromJson(json_answer.toUtf8());
    QString better_html = json_doc["content"].toString();

    m_webView->setReadabledHtml(better_html, m_webView->url());
  }
}

void WebBrowser::fullArticleFailed(QObject* sndr, const QString& error) {
  if (sndr == this && !error.isEmpty()) {
    m_webView->setReadabledHtml(error, m_webView->url());
  }
}

void WebBrowser::initializeLayout() {
  m_toolBar->setFloatable(false);
  m_toolBar->setMovable(false);
  m_toolBar->setAllowedAreas(Qt::ToolBarArea::TopToolBarArea);

  // Modify action texts.
  if (m_actionBack != nullptr) {
    m_actionBack->setText(tr("Back"));
    m_actionBack->setIcon(qApp->icons()->fromTheme(QSL("go-previous")));
    m_toolBar->addAction(m_actionBack);
  }

  if (m_actionForward != nullptr) {
    m_actionForward->setText(tr("Forward"));
    m_actionForward->setIcon(qApp->icons()->fromTheme(QSL("go-next")));
    m_toolBar->addAction(m_actionForward);
  }

  if (m_actionReload != nullptr) {
    m_actionReload->setText(tr("Reload"));
    m_actionReload->setIcon(qApp->icons()->fromTheme(QSL("reload"), QSL("view-refresh")));
    m_toolBar->addAction(m_actionReload);
  }

  if (m_actionStop != nullptr) {
    m_actionStop->setText(tr("Stop"));
    m_actionStop->setIcon(qApp->icons()->fromTheme(QSL("process-stop")));
    m_toolBar->addAction(m_actionStop);
  }

  m_actionOpenInSystemBrowser->setEnabled(false);
  m_actionReadabilePage->setEnabled(false);
  m_actionGetFullArticle->setEnabled(false);

  // Add needed actions into toolbar.
  m_toolBar->addAction(m_actionOpenInSystemBrowser);
  m_toolBar->addAction(m_actionGetFullArticle);
  m_toolBar->addAction(m_actionReadabilePage);

#if defined(ENABLE_MEDIAPLAYER)
  m_actionPlayPageInMediaPlayer->setEnabled(false);
  m_toolBar->addAction(m_actionPlayPageInMediaPlayer);
#endif

  m_txtLocationAction = m_toolBar->addWidget(m_txtLocation);

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

  m_searchWidget->hide();
}

void WebBrowser::onLoadingStarted() {
  m_loadingProgress->show();
  m_actionOpenInSystemBrowser->setEnabled(false);
  m_actionReadabilePage->setEnabled(false);
  m_actionGetFullArticle->setEnabled(false);

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
      m_actionGetFullArticle->setEnabled(true);
      m_actionReadabilePage->setEnabled(true);

#if defined(ENABLE_MEDIAPLAYER)
      m_actionPlayPageInMediaPlayer->setEnabled(true);
#endif
    }
    else {
      m_actionOpenInSystemBrowser->setEnabled(false);
      m_actionReadabilePage->setEnabled(false);
      m_actionGetFullArticle->setEnabled(false);

#if defined(ENABLE_MEDIAPLAYER)
      m_actionPlayPageInMediaPlayer->setEnabled(false);
#endif
    }
  }

  m_loadingProgress->hide();
  m_loadingProgress->setValue(0);
}
