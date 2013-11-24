#include <QStyleOptionFrameV3>
#include <QAction>
#include <QMenu>
#include <QDir>
#include <QFile>
#include <QWebFrame>
#include <QContextMenuEvent>
#include <QDateTime>

#include "core/defs.h"
#include "core/settings.h"
#include "core/basewebpage.h"
#include "gui/basewebview.h"
#include "gui/skinfactory.h"
#include "gui/iconthemefactory.h"


BaseWebView::BaseWebView(QWidget *parent)
  : QWebView(parent), m_page(new BaseWebPage(this)) {
  setPage(m_page);
  setContextMenuPolicy(Qt::CustomContextMenu);
  initializeActions();
  createConnections();
}

BaseWebView::~BaseWebView() {
  qDebug("Destroying BaseWebView.");
}

void BaseWebView::onLoadFinished(bool ok) {
  // If page was not loaded, then display custom error page.
  if (!ok) {
    displayErrorPage();
  }
}

void BaseWebView::openLinkInNewTab() {
  emit linkMiddleClicked(m_contextLinkUrl);
}

void BaseWebView::openImageInNewTab() {
  emit linkMiddleClicked(m_contextImageUrl);
}

void BaseWebView::createConnections() {
  connect(this, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));
  connect(this, SIGNAL(customContextMenuRequested(QPoint)),
          this, SLOT(popupContextMenu(QPoint)));

  connect(m_actionOpenLinkNewTab,SIGNAL(triggered()), this, SLOT(openLinkInNewTab()));
  connect(m_actionOpenImageNewTab, SIGNAL(triggered()), this, SLOT(openImageInNewTab()));
}

void BaseWebView::setupIcons() {
  m_actionReload->setIcon(IconThemeFactory::getInstance()->fromTheme("view-refresh"));
  m_actionCopyLink->setIcon(IconThemeFactory::getInstance()->fromTheme("edit-copy"));
  m_actionCopyImage->setIcon(IconThemeFactory::getInstance()->fromTheme("insert-image"));

#if QT_VERSION >= 0x040800
  m_actionCopyImageUrl->setIcon(IconThemeFactory::getInstance()->fromTheme("edit-copy"));
#endif

  m_actionOpenLinkThisTab->setIcon(IconThemeFactory::getInstance()->fromTheme("text-html"));
  m_actionOpenLinkNewTab->setIcon(IconThemeFactory::getInstance()->fromTheme("text-html"));
  m_actionOpenImageNewTab->setIcon(IconThemeFactory::getInstance()->fromTheme("insert-image"));
}

void BaseWebView::initializeActions() {
  // Create needed actions.
  m_actionReload = pageAction(QWebPage::Reload);
  m_actionReload->setParent(this);
  m_actionReload->setText(tr("Reload web page"));
  m_actionReload->setToolTip(tr("Reload current web page"));

  m_actionCopyLink = pageAction(QWebPage::CopyLinkToClipboard);
  m_actionCopyLink->setParent(this);
  m_actionCopyLink->setText(tr("Copy link url"));
  m_actionCopyLink->setToolTip(tr("Copy link url to clipboard"));

  m_actionCopyImage = pageAction(QWebPage::CopyImageToClipboard);
  m_actionCopyImage->setParent(this);
  m_actionCopyImage->setText(tr("Copy image"));
  m_actionCopyImage->setToolTip(tr("Copy image to clipboard"));

#if QT_VERSION >= 0x040800
  m_actionCopyImageUrl = pageAction(QWebPage::CopyImageUrlToClipboard);
  m_actionCopyImageUrl->setParent(this);
  m_actionCopyImageUrl->setText(tr("Copy image url"));
  m_actionCopyImageUrl->setToolTip(tr("Copy image url to clipboard"));
#endif

  m_actionOpenLinkNewTab = pageAction(QWebPage::OpenLinkInNewWindow);
  m_actionOpenLinkNewTab->setParent(this);
  m_actionOpenLinkNewTab->setText(tr("Open link in new tab"));
  m_actionOpenLinkNewTab->setToolTip(tr("Open this hyperlink in new tab"));

  m_actionOpenLinkThisTab = pageAction(QWebPage::OpenLink);
  m_actionOpenLinkThisTab->setParent(this);
  m_actionOpenLinkThisTab->setText(tr("Follow link"));
  m_actionOpenLinkThisTab->setToolTip(tr("Open the hyperlink in this tab"));

  m_actionOpenImageNewTab = pageAction(QWebPage::OpenImageInNewWindow);
  m_actionOpenImageNewTab->setParent(this);
  m_actionOpenImageNewTab->setText(tr("Open image in new tab"));
  m_actionOpenImageNewTab->setToolTip(tr("Open this image in this tab"));
}

void BaseWebView::displayErrorPage() {
  setHtml(SkinFactory::getInstance()->getCurrentMarkup().arg(tr("Page not found"),
                                                             tr("Check your internet connection or website address"),
                                                             QString(),
                                                             tr("This failure can be caused by:<br><ul>"
                                                                "<li>non-functional internet connection,</li>"
                                                                "<li>incorrect website address,</li>"
                                                                "<li>bad proxy server settings,</li>"
                                                                "<li>target destination outage,</li>"
                                                                "<li>many other things.</li>"
                                                                "</ul>"),
                                                             QDateTime::currentDateTime().toString(Qt::DefaultLocaleLongDate)));
}

void BaseWebView::popupContextMenu(const QPoint &pos) {
  QMenu context_menu(tr("Web browser"), this);
  QMenu image_submenu(tr("Image"), &context_menu);
  QMenu link_submenu(tr("Hyperlink"), this);
  QWebHitTestResult hit_result = page()->mainFrame()->hitTestContent(pos);

  image_submenu.setIcon(IconThemeFactory::getInstance()->fromTheme("image-x-generic"));
  link_submenu.setIcon(IconThemeFactory::getInstance()->fromTheme("text-html"));

  // Assemble the menu from actions.
  context_menu.addAction(m_actionReload);

  QUrl hit_url = hit_result.linkUrl();
  QUrl hit_image_url = hit_result.imageUrl();

  if (hit_url.isValid()) {
    m_contextLinkUrl = hit_url;

    context_menu.addMenu(&link_submenu);
    link_submenu.addAction(m_actionOpenLinkThisTab);
    link_submenu.addAction(m_actionOpenLinkNewTab);
    link_submenu.addAction(m_actionCopyLink);
  }

  if (!hit_result.pixmap().isNull()) {
    // Add 'Image' menu, because if user clicked image it needs to be visible.
    context_menu.addMenu(&image_submenu);

    if (hit_image_url.isValid()) {
      m_contextImageUrl = hit_image_url;
      image_submenu.addAction(m_actionOpenImageNewTab);

#if QT_VERSION >= 0x040800
      image_submenu.addAction(m_actionCopyImageUrl);
#endif
    }
    image_submenu.addAction(m_actionCopyImage);
  }

  // Display the menu.
  context_menu.exec(mapToGlobal(pos));
}

void BaseWebView::mousePressEvent(QMouseEvent *event) {
  if (event->button() & Qt::LeftButton && event->modifiers() & Qt::ControlModifier) {
    QWebHitTestResult hit_result = page()->mainFrame()->hitTestContent(event->pos());

    // Check if user clicked with middle mouse button on some
    // hyperlink.
    QUrl link_url = hit_result.linkUrl();
    QUrl image_url = hit_result.imageUrl();

    if (link_url.isValid()) {
      emit linkMiddleClicked(link_url);
      // No more handling of event is now needed. Return.
      return;
    }
    else if (image_url.isValid()) {
      emit linkMiddleClicked(image_url);
      return;
    }
  }
  else if (event->button() & Qt::MiddleButton) {
    m_gestureOrigin = event->pos();
    return;
  }

  QWebView::mousePressEvent(event);
}

void BaseWebView::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() & Qt::MiddleButton) {
    bool are_gestures_enabled = Settings::getInstance()->value(APP_CFG_BROWSER,
                                                               "gestures_enabled",
                                                               true).toBool();
    if (are_gestures_enabled) {
      QPoint release_point = event->pos();
      int left_move = m_gestureOrigin.x() - release_point.x();
      int right_move = -left_move;
      int top_move = m_gestureOrigin.y() - release_point.y();
      int bottom_move = -top_move;
      int total_max = qMax(qMax(qMax(left_move, right_move),
                                qMax(top_move, bottom_move)),
                           40);

      if (total_max == left_move) {
        back();
      }
      else if (total_max == right_move) {
        forward();
      }
      else if (total_max == top_move) {
        reload();
      }
      else if (total_max == bottom_move) {
        emit newTabRequested();
      }
    }
  }

  QWebView::mouseReleaseEvent(event);
}

void BaseWebView::wheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::ControlModifier) {
    if (event->delta() > 0) {
      increaseWebPageZoom();
      emit zoomFactorChanged();
      return;
    }
    else if (event->delta() < 0) {
      decreaseWebPageZoom();
      emit zoomFactorChanged();
      return;
    }
  }

  QWebView::wheelEvent(event);
}

void BaseWebView::paintEvent(QPaintEvent *event) {
  QWebView::paintEvent(event);

  // Draw additional frame.
  /*
  QPainter painter(this);
  QStyleOptionFrameV3 style_option;
  int frame_shape = QFrame::Sunken & QFrame::Shape_Mask;

  style_option.init(this);
  style_option.frameShape = QFrame::Shape(int(style_option.frameShape) |
                                          QFrame::StyledPanel |
                                          frame_shape);
  style_option.rect = rect();
  style_option.lineWidth = 1;
  style_option.midLineWidth = 0;

  style()->drawControl(QStyle::CE_ShapedFrame, &style_option, &painter, this);
  */
}

bool BaseWebView::increaseWebPageZoom() {
  qreal new_factor = zoomFactor() + 0.1;

  if (new_factor >= 0.0 && new_factor <= MAX_ZOOM_FACTOR) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}

bool BaseWebView::decreaseWebPageZoom() {
  qreal new_factor = zoomFactor() - 0.1;

  if (new_factor >= 0.0 && new_factor <= MAX_ZOOM_FACTOR) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}

bool BaseWebView::resetWebPageZoom() {
  qreal new_factor = 1.0;

  if (new_factor != zoomFactor()) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}
