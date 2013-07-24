#include <QStyleOptionFrameV3>
#include <QAction>
#include <QMenu>
#include <QWebFrame>
#include <QContextMenuEvent>

#include "core/defs.h"
#include "core/settings.h"
#include "core/basewebpage.h"
#include "gui/basewebview.h"
#include "gui/themefactory.h"


BaseWebView::BaseWebView(QWidget *parent)
  : QWebView(parent), m_page(new BaseWebPage(this)) {
  setPage(m_page);
  setContextMenuPolicy(Qt::NoContextMenu);
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

void BaseWebView::createConnections() {
  connect(this, &BaseWebView::loadFinished,
          this, &BaseWebView::onLoadFinished);
  connect(this, &BaseWebView::customContextMenuRequested,
          this, &BaseWebView::popupContextMenu);
}

void BaseWebView::setupIcons() {
  m_actionReload->setIcon(ThemeFactory::fromTheme("view-refresh"));
  m_actionCopyLink->setIcon(ThemeFactory::fromTheme("edit-copy"));
  m_actionCopyImage->setIcon(ThemeFactory::fromTheme("insert-image"));
  m_actionCopyImageUrl->setIcon(ThemeFactory::fromTheme("edit-copy"));
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


  m_actionCopyImageUrl = pageAction(QWebPage::CopyImageUrlToClipboard);
  m_actionCopyImageUrl->setParent(this);
  m_actionCopyImageUrl->setText(tr("Copy image url"));
  m_actionCopyImageUrl->setToolTip(tr("Copy image url to clipboard"));
}

void BaseWebView::displayErrorPage() {
  // TODO: Add better custom error page.
  setHtml("error", url());
}

void BaseWebView::popupContextMenu(const QPoint &pos) {
  QMenu context_menu(tr("Web browser"), this);
  QMenu image_submenu(tr("Image"), &context_menu);
  QWebHitTestResult hit_result = page()->mainFrame()->hitTestContent(pos);

  image_submenu.setIcon(ThemeFactory::fromTheme("image-x-generic"));

  // Assemble the menu from actions.
  context_menu.addAction(m_actionReload);

  if (hit_result.linkUrl().isValid()) {
    context_menu.addAction(m_actionCopyLink);
  }

  if (!hit_result.pixmap().isNull()) {
    // Add 'Image' menu, because if user clicked image it needs to be visible.
    context_menu.addMenu(&image_submenu);

    image_submenu.addAction(m_actionCopyImage);
  }

  if (hit_result.imageUrl().isValid()) {
    image_submenu.addAction(m_actionCopyImageUrl);
  }

  // Display the menu.
  context_menu.exec(mapToGlobal(pos));
}

void BaseWebView::mousePressEvent(QMouseEvent *event) {
  if (event->button() & Qt::MiddleButton) {
    QWebHitTestResult hit_result = page()->mainFrame()->hitTestContent(event->pos());

    // Check if user clicked with middle mouse button on some
    // hyperlink.
    if (hit_result.linkUrl().isValid()) {
      emit linkMiddleClicked(hit_result.linkUrl());

      // No more handling of event is now needed. Return.
      return;
    }
  }
  else if (event->button() & Qt::RightButton) {
    m_gestureOrigin = event->pos();
  }

  // TODO: Add mouse gestures (from quite-rss).
  QWebView::mousePressEvent(event);
}

void BaseWebView::mouseReleaseEvent(QMouseEvent *event) {
  QWebView::mouseReleaseEvent(event);

  if (event->button() & Qt::RightButton) {
    bool are_gestures_enabled = Settings::getInstance()->value(APP_CFG_BROWSER,
                                                               "gestures_enabled",
                                                               true).toBool();
    if (are_gestures_enabled) {
      QPoint release_point = event->pos();
      int left_move = m_gestureOrigin.x() - release_point.x();
      int right_move = release_point.x() - m_gestureOrigin.x();
      int top_move = m_gestureOrigin.y() - release_point.y();
      int bottom_move = release_point.y() - m_gestureOrigin.y();
      int total_max = qMax(qMax(qMax(left_move, right_move),
                                qMax(top_move, bottom_move)),
                           40);

      if (total_max == left_move && are_gestures_enabled) {
        back();
      }
      else if (total_max == right_move && are_gestures_enabled) {
        forward();
      }
      else if (total_max == top_move && are_gestures_enabled) {
        reload();
      }
      else if (total_max == bottom_move && are_gestures_enabled) {
        emit newTabRequested();
      }
      else {
        emit customContextMenuRequested(event->pos());
      }
    }
    else {
      emit customContextMenuRequested(event->pos());
    }
  }
}

void BaseWebView::paintEvent(QPaintEvent *event) {
  QWebView::paintEvent(event);

  // Draw additional frame.
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
}
